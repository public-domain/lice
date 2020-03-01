/*
 * This file implements a small state machine for handling all the forms
 * of initialization C offers. It's called from the parser just like
 * declaration specification. It's a sub portion of the core parser,
 * seperated from all that logic due to the nature of initializer
 * complexity.
 */
#include "parse.h"
#include "init.h"
#include "lice.h"
#include "lexer.h"
#include "conv.h"

#include <string.h>
#include <stdlib.h>

static void init_element   (list_t *, data_type_t *, int, bool);
static void init_structure (list_t *, data_type_t *, int, bool);
static void init_array     (list_t *, data_type_t *, int, bool);
static void init_list      (list_t *, data_type_t *, int, bool);

/*
 * Initializer elements need to be sorted by semantic order, instead
 * of lexical order since designated initializers are allowed to
 * overwrite existingly assigned fields lexically, but the order needs
 * to stay dependent on semantics. It's also generally more efficent for
 * initialization to stay sorted.
 */
static int init_sort_predicate(const void *p, const void *q) {
    const ast_t *const *restrict a = p;
    const ast_t *const *restrict b = q;

    return (*a)->init.offset <  (*b)->init.offset ? -1 :
           (*a)->init.offset == (*b)->init.offset ?  0 : 1;
}

static void init_sort(list_t *inits) {
    size_t           length = list_length(inits);
    size_t           index  = 0;
    ast_t          **temp   = memory_allocate(sizeof(ast_t *) * length);
    list_iterator_t *it     = list_iterator(inits);

    while (!list_iterator_end(it))
        temp[index++] = list_iterator_next(it);

    qsort(temp, length, sizeof(ast_t *), &init_sort_predicate);

    list_empty(inits);
    for (index = 0; index < length; index++)
        list_push(inits, temp[index]);
}

static bool init_earlyout(lexer_token_t *token, bool brace, bool designated) {
    if ((lexer_ispunct(token, '.') || lexer_ispunct(token, '[')) && !brace && !designated) {
        lexer_unget(token);
        return true;
    }
    return false;
}

/*
 * Utility routines to determine and skip to braces for initialization
 * involving aggregates.
 */
static bool init_skip_brace_maybe(void) {
    lexer_token_t *token = lexer_next();
    if (lexer_ispunct(token, '{'))
        return true;
    lexer_unget(token);
    return false;
}

static void init_skip_comma_maybe(void) {
    lexer_token_t *token = lexer_next();

    if (!lexer_ispunct(token, ','))
        lexer_unget(token);
}

static void init_skip_brace(void) {
    for (;;) {
        /*
         * Potentially infinite look a head, got to love C's grammar for
         * this sort of crap.
         */
        lexer_token_t *token = lexer_next();
        if (lexer_ispunct(token, '}'))
            return;

        if (lexer_ispunct(token, '.')) {
            lexer_next();
            parse_expect('=');
        } else {
            lexer_unget(token);
        }

        ast_t *ignore = parse_expression_assignment();
        if (!ignore)
            return;

        compile_warn("excess elements in initializer");
        init_skip_comma_maybe();
    }
}

/*
 * Structure and array initialization routines:
 *  deals with standard initialization via aggregate initializer, as well
 *  as designated initialization, and nested aggreate + designation. In
 *  the case of array designated initialization array subscripting is
 *  handled, where as in the case of structure designated initialization
 *  field members are indexed by .fieldname. The GCC style of designated
 *  initializers isn't supported yet, neither is range initialization.
 */
static void init_structure_intermediate(list_t *init, data_type_t *type, int offset, bool designated) {
    bool             brace = init_skip_brace_maybe();
    list_iterator_t *it    = list_iterator(table_keys(type->fields));

    for (;;) {
        lexer_token_t *token = lexer_next();
        if (lexer_ispunct(token, '}')) {
            if (!brace)
                lexer_unget(token);
            return;
        }

        char        *fieldname;
        data_type_t *fieldtype;

        if (init_earlyout(token, brace, designated))
            return;

        if (lexer_ispunct(token, '.')) {
            if (!(token = lexer_next()) || token->type != LEXER_TOKEN_IDENTIFIER)
                compile_error("invalid designated initializer");
            fieldname = token->string;
            if (!(fieldtype = table_find(type->fields, fieldname)))
                compile_error("field `%s' doesn't exist in designated initializer", fieldname);

            it = list_iterator(table_keys(type->fields));
            while (!list_iterator_end(it))
                if (!strcmp(fieldname, list_iterator_next(it)))
                    break;
            designated = true;
        } else {
            lexer_unget(token);
            if (list_iterator_end(it))
                break;

            fieldname = list_iterator_next(it);
            fieldtype = table_find(type->fields, fieldname);
        }
        init_element(init, fieldtype, offset + fieldtype->offset, designated);
        init_skip_comma_maybe();
        designated = false;

        if (!type->isstruct)
            break;
    }
    if (brace)
        init_skip_brace();
}

static void init_array_intermediate(list_t *init, data_type_t *type, int offset, bool designated) {
    bool brace    = init_skip_brace_maybe();
    bool flexible = (type->length <= 0);
    int  size     = type->pointer->size;
    int  i;

    for (i = 0; flexible || i < type->length; i++) {
        lexer_token_t *token = lexer_next();
        if (lexer_ispunct(token, '}')) {
            if (!brace)
                lexer_unget(token);
            goto complete;
        }

        if (init_earlyout(token, brace, designated))
            return;

        if (lexer_ispunct(token, '[')) {
            /* designated array initializer */
            int index = parse_expression_evaluate();
            if (index < 0 || (!flexible && type->length <= index))
                compile_error("out of bounds");
            i = index;
            parse_expect(']');
            designated = true;
        } else {
            lexer_unget(token);
        }
        init_element(init, type->pointer, offset + size * i, designated);
        init_skip_comma_maybe();
        designated = false;
    }
    if (brace)
        init_skip_brace();

complete:
    if (type->length < 0) {
        type->length = i;
        type->size   = size * i;
    }
}

/*
 * Intermediate stages deal with all the logic, these functions are
 * just tail calls (hopefully optimized) to the intermediate stages followed
 * by a sorting of the elements to honor semantic ordering of initialization.
 */
static void init_structure(list_t *init, data_type_t *type, int offset, bool designated) {
    init_structure_intermediate(init, type, offset, designated);
    init_sort(init);
}

static void init_array(list_t *init, data_type_t *type, int offset, bool designated) {
    init_array_intermediate(init, type, offset, designated);
    init_sort(init);
}

/*
 * The entry points to the initializers, single element initialization
 * and initializer list initialization will dispatch into the appropriate
 * initialization parsing routines as defined above.
 */
static void init_element(list_t *init, data_type_t *type, int offset, bool designated) {
    parse_next('=');
    if (type->type == TYPE_ARRAY || type->type == TYPE_STRUCTURE)
        init_list(init, type, offset, designated);
    else if (parse_next('{')) {
        init_element(init, type, offset, designated);
        parse_expect('}');
    } else {
        ast_t *expression = parse_expression_assignment();
        parse_semantic_assignable(type, expression->ctype);
        list_push(init, ast_initializer(expression, type, offset));
    }
}

static void init_string(list_t *init, data_type_t *type, char *p, int offset) {
    if (type->length == -1)
        type->length = type->size = strlen(p) + 1;

    int i = 0;
    for (; i < type->length && *p; i++) {
        list_push(init, ast_initializer(
            ast_new_integer(ast_data_table[AST_DATA_CHAR], *p++),
            ast_data_table[AST_DATA_CHAR], offset + i
        ));
    }
    for (; i < type->length; i++) {
        list_push(init, ast_initializer(
            ast_new_integer(ast_data_table[AST_DATA_CHAR], 0),
            ast_data_table[AST_DATA_CHAR], offset + i
        ));
    }
}

static void init_list(list_t *init, data_type_t *type, int offset, bool designated) {
    lexer_token_t *token = lexer_next();
    if (ast_type_isstring(type)) {
        if (token->type == LEXER_TOKEN_STRING) {
            init_string(init, type, token->string, offset);
            return;
        }

        if (lexer_ispunct(token, '{') && lexer_peek()->type == LEXER_TOKEN_STRING) {
            token = lexer_next();
            init_string(init, type, token->string, offset);
            parse_expect('}');
            return;
        }
    }
    lexer_unget(token);

    if (type->type == TYPE_ARRAY)
        init_array(init, type, offset, designated);
    else if (type->type == TYPE_STRUCTURE)
        init_structure(init, type, offset, designated);
    else
        init_array(init, ast_array(type, 1), offset, designated);
}

/*
 * Actual entry point of the parser, parses an initializer list, while
 * also dispatching into the appropriate parser routines depending on
 * certain state like, array/structure, designated or not.
 */
list_t *init_entry(data_type_t *type) {
    list_t *list = list_create();
    if (lexer_ispunct(lexer_peek(), '{') || ast_type_isstring(type)) {
        init_list(list, type, 0, false);
        return list;
    }

    ast_t *init = parse_expression_assignment();
    if (conv_capable(init->ctype) && init->ctype->type != type->type)
        init = ast_type_convert(type, init);
    list_push(list, ast_initializer(init, type, 0));

    return list;
}
