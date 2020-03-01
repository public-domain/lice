#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "lice.h"
#include "lexer.h"
#include "conv.h"
#include "init.h"
#include "parse.h"
#include "opt.h"

static ast_t       *parse_expression(void);
static ast_t       *parse_expression_postfix(void);
static ast_t       *parse_expression_multiplicative(void);
static ast_t       *parse_expression_conditional(void);
static ast_t       *parse_expression_cast(void);
static data_type_t *parse_expression_cast_type(void);
static ast_t       *parse_expression_unary(void);
static ast_t       *parse_expression_comma(void);

static ast_t       *parse_statement_compound(void);
static void         parse_statement_declaration(list_t *);
static ast_t       *parse_statement(void);


static data_type_t *parse_declaration_specification(storage_t *);
static data_type_t *parse_declarator(char **, data_type_t *, list_t *, cdecl_t);
static void         parse_declaration(list_t *, ast_t *(*)(data_type_t *, char *));

static data_type_t *parse_function_parameter(char **, bool);
static data_type_t *parse_function_parameters(list_t *, data_type_t *);


table_t *parse_typedefs = &SENTINEL_TABLE;

static bool parse_type_check(lexer_token_t *token);

static void parse_semantic_lvalue(ast_t *ast) {
    switch (ast->type) {
        case AST_TYPE_VAR_LOCAL:
        case AST_TYPE_VAR_GLOBAL:
        case AST_TYPE_DEREFERENCE:
        case AST_TYPE_STRUCT:
            return;
    }
    compile_error("expected lvalue, `%s' isn't a valid lvalue", ast_string(ast));
}

static void parse_semantic_notvoid(data_type_t *type) {
    if (type->type == TYPE_VOID)
        compile_error("void not allowed in expression");
}

void parse_expect(char punct) {
    lexer_token_t *token = lexer_next();
    if (!lexer_ispunct(token, punct))
        compile_error("expected `%c`, got %s instead", punct, lexer_token_string(token));
}

void parse_semantic_assignable(data_type_t *to, data_type_t *from) {
    from = ast_array_convert(from);
    if ((conv_capable(to)   || to->type   == TYPE_POINTER) &&
        (conv_capable(from) || from->type == TYPE_POINTER)) {

        return;
    }

    if (ast_struct_compare(to, from))
        return;

    compile_error("incompatible types '%s' and '%s' in assignment", ast_type_string(to), ast_type_string(from));
}

static bool parse_identifer_check(lexer_token_t *token, const char *identifier) {
    return token->type == LEXER_TOKEN_IDENTIFIER && !strcmp(token->string, identifier);
}

long parse_evaluate(ast_t *ast);
int parse_evaluate_offsetof(ast_t *ast, int offset) {
    if (ast->type == AST_TYPE_STRUCT)
        return parse_evaluate_offsetof(ast->structure, ast->ctype->offset + offset);
    return parse_evaluate(ast) + offset;
}

long parse_evaluate(ast_t *ast) {
    long cond;

    switch (ast->type) {
        case AST_TYPE_LITERAL:
            if (ast_type_isinteger(ast->ctype))
                return ast->integer;
            compile_error("not a valid integer constant expression `%s'", ast_string(ast));
            break;

        case '+':             return parse_evaluate(ast->left) +  parse_evaluate(ast->right);
        case '-':             return parse_evaluate(ast->left) -  parse_evaluate(ast->right);
        case '*':             return parse_evaluate(ast->left) *  parse_evaluate(ast->right);
        case '/':             return parse_evaluate(ast->left) /  parse_evaluate(ast->right);
        case '<':             return parse_evaluate(ast->left) <  parse_evaluate(ast->right);
        case '>':             return parse_evaluate(ast->left) >  parse_evaluate(ast->right);
        case '^':             return parse_evaluate(ast->left) ^  parse_evaluate(ast->right);
        case '%':             return parse_evaluate(ast->left) %  parse_evaluate(ast->right);
        case '|':             return parse_evaluate(ast->left) |  parse_evaluate(ast->right);

        case AST_TYPE_AND:    return parse_evaluate(ast->left) && parse_evaluate(ast->right);
        case AST_TYPE_OR:     return parse_evaluate(ast->left) || parse_evaluate(ast->right);
        case AST_TYPE_EQUAL:  return parse_evaluate(ast->left) == parse_evaluate(ast->right);
        case AST_TYPE_LEQUAL: return parse_evaluate(ast->left) <= parse_evaluate(ast->right);
        case AST_TYPE_GEQUAL: return parse_evaluate(ast->left) >= parse_evaluate(ast->right);
        case AST_TYPE_NEQUAL: return parse_evaluate(ast->left) != parse_evaluate(ast->right);
        case AST_TYPE_LSHIFT: return parse_evaluate(ast->left) << parse_evaluate(ast->right);
        case AST_TYPE_RSHIFT: return parse_evaluate(ast->left) >> parse_evaluate(ast->right);


        /* Deal with unary operations differently */
        case '!':                        return !parse_evaluate(ast->unary.operand);
        case '~':                        return ~parse_evaluate(ast->unary.operand);
        case AST_TYPE_NEGATE:            return -parse_evaluate(ast->unary.operand);
        case AST_TYPE_EXPRESSION_CAST:   return  parse_evaluate(ast->unary.operand);

        /* Branches too */
        case AST_TYPE_EXPRESSION_TERNARY:
            cond = parse_evaluate(ast->ifstmt.cond);
            if (cond)
                return ast->ifstmt.then ? parse_evaluate(ast->ifstmt.cond) : cond;
            return parse_evaluate(ast->ifstmt.last);

        /* Deal with logical right shift specifically */
        case AST_TYPE_LRSHIFT:
            return ((unsigned long)parse_evaluate(ast->left)) >> parse_evaluate(ast->right);

        case AST_TYPE_CONVERT:
            return parse_evaluate(ast->unary.operand);

        /*
         * Allow constant evaluation for things like offsetof, although
         * we could just implement __builtin_offsetof, this works easier
         * and also allows the existing idiom to work.
         */
        case AST_TYPE_ADDRESS:
            if (ast->unary.operand->type == AST_TYPE_STRUCT)
                return parse_evaluate_offsetof(ast->unary.operand, 0);
            goto error;

        case AST_TYPE_DEREFERENCE:
            if (ast->unary.operand->ctype->type == TYPE_POINTER)
                return parse_evaluate(ast->unary.operand);
            goto error;

        default:
        error:
            compile_error("not a valid integer constant expression `%s'", ast_string(ast));
    }
    return -1;
}

bool parse_next(int ch) {
    lexer_token_t *token = lexer_next();
    if (lexer_ispunct(token, ch))
        return true;
    lexer_unget(token);
    return false;
}

static list_t *parse_function_args(list_t *parameters) {
    list_t          *list = list_create();
    list_iterator_t *it   = list_iterator(parameters);
    for (;;) {
        if (parse_next(')'))
            break;
        ast_t       *arg  = parse_expression_assignment();
        data_type_t *type = list_iterator_next(it);

        if (!type) {
            type = ast_type_isfloating(arg->ctype)
                        ? ast_data_table[AST_DATA_DOUBLE]
                        : ast_type_isinteger(arg->ctype)
                            ? ast_data_table[AST_DATA_INT]
                            : arg->ctype;
        }

        // todo semantic check
        parse_semantic_assignable(ast_array_convert(type), ast_array_convert(arg->ctype));

        if (type->type != arg->ctype->type)
            arg = ast_type_convert(type, arg);

        list_push(list, arg);

        lexer_token_t *token = lexer_next();
        if (lexer_ispunct(token, ')'))
            break;
        if (!lexer_ispunct(token, ','))
            compile_error("unexpected token `%s'", lexer_token_string(token));
    }
    return list;
}

static ast_t *parse_va_start(void) {
    ast_t *ap = parse_expression_assignment();
    parse_expect(')');
    return ast_va_start(ap);
}

static ast_t *parse_va_arg(void) {
    ast_t *ap = parse_expression_assignment();
    parse_expect(',');
    data_type_t *type = parse_expression_cast_type();
    parse_expect(')');
    return ast_va_arg(type, ap);
}

static ast_t *parse_function_call(char *name, ast_t *function) {
    if (!strcmp(name, "__builtin_va_start"))
        return parse_va_start();
    if (!strcmp(name, "__builtin_va_arg"))
        return parse_va_arg();

    if (function) {
        data_type_t *declaration = function->ctype;
        if (declaration->type != TYPE_FUNCTION)
            compile_error("expected a function name, `%s' isn't a function", name);
        list_t *list = parse_function_args(declaration->parameters);
        return ast_call(declaration, name, list);
    }
    compile_warn("implicit declaration of function ‘%s’", name);
    list_t *list = parse_function_args(&SENTINEL_LIST);
    return ast_call(ast_prototype(ast_data_table[AST_DATA_INT], list_create(), true), name, list);
}

static ast_t *parse_function_pointer_call(ast_t *function) {
    list_t *list = parse_function_args(function->ctype->pointer->parameters);
    return ast_pointercall(function, list);
}

static ast_t *parse_generic(char *name) {
    ast_t *ast = table_find(ast_localenv ? ast_localenv : ast_globalenv, name);

    if (!ast || ast->ctype->type == TYPE_FUNCTION)
        return ast_designator(name, ast);

    return ast;
}

static ast_t *parse_number_integer(char *string) {
    char *p    = string;
    int   base = 10;

    if (!strncasecmp(string, "0x", 2)) {
        base = 16;
        p++;
        p++;
    } else if (!strncasecmp(string, "0b", 2)){
        if (!opt_extension_test(EXTENSION_BINARYCONSTANTS))
            compile_error("binary constants only supported in -std=lice or -std=gnuc");
        base = 2;
        p++;
        p++;
    } else if (string[0] == '0' && string[1] != '\0') {
        base = 8;
        p++;
    }

    char *digits = p;
    while (isxdigit(*p)) {
        if (base == 10 && isalpha(*p))
            compile_error("invalid character in decimal literal");
        if (base == 8 && !('0' <= *p && *p <= '7'))
            compile_error("invalid character in octal literal");
        if (base == 2 && (*p != '0' && *p != '1'))
            compile_error("invalid character in binary literal");
        p++;
    }

    if (!strcasecmp(p, "l"))
        return ast_new_integer(ast_data_table[AST_DATA_LONG], strtol(string, NULL, base));
    if (!strcasecmp(p, "ul") || !strcasecmp(p, "lu") || !strcasecmp(p, "u"))
        return ast_new_integer(ast_data_table[AST_DATA_ULONG], strtoul(string, NULL, base));
    if (!strcasecmp(p, "ll"))
        return ast_new_integer(ast_data_table[AST_DATA_LLONG], strtoll(string, NULL, base));
    if (!strcasecmp(p, "ull") || !strcasecmp(p, "llu"))
        return ast_new_integer(ast_data_table[AST_DATA_ULLONG], strtoull(string, NULL, base));
    if (*p != '\0')
        compile_error("invalid suffix for literal");

    long value = strtol(digits, NULL, base);
    return (value & ~(long)UINT_MAX)
                ? ast_new_integer(ast_data_table[AST_DATA_LONG], value)
                : ast_new_integer(ast_data_table[AST_DATA_INT], value);
}

static ast_t *parse_number_floating(char *string) {
    char *p = string;
    char *end;

    while (p[1])
        p++;

    ast_t *ast;
    if (*p == 'l' || *p == 'L')
        ast = ast_new_floating(ast_data_table[AST_DATA_LDOUBLE], strtold(string, &end));
    else if (*p == 'f' || *p == 'F')
        ast = ast_new_floating(ast_data_table[AST_DATA_FLOAT], strtof(string, &end));
    else {
        ast = ast_new_floating(ast_data_table[AST_DATA_DOUBLE], strtod(string, &end));
        p++;
    }

    if (end != p)
        compile_error("malformatted float literal");

    return ast;
}

static ast_t *parse_number(char *string) {
    return strpbrk(string, ".pP") ||
                /*
                 * 0xe.. is integer type, sadly it's hard to check for that
                 * without additonal logic.
                 */
                (strncasecmp(string, "0x", 2) && strpbrk(string, "eE"))
                    ? parse_number_floating(string)
                    : parse_number_integer(string);
}

static int parse_operation_reclassify(int punct) {
    switch (punct) {
        case LEXER_TOKEN_LSHIFT: return AST_TYPE_LSHIFT;
        case LEXER_TOKEN_RSHIFT: return AST_TYPE_RSHIFT;
        case LEXER_TOKEN_EQUAL:  return AST_TYPE_EQUAL;
        case LEXER_TOKEN_GEQUAL: return AST_TYPE_GEQUAL;
        case LEXER_TOKEN_LEQUAL: return AST_TYPE_LEQUAL;
        case LEXER_TOKEN_NEQUAL: return AST_TYPE_NEQUAL;
        case LEXER_TOKEN_AND:    return AST_TYPE_AND;
        case LEXER_TOKEN_OR:     return AST_TYPE_OR;
        default:
            break;
    }
    return punct;
}

static int parse_operation_compound_operator(lexer_token_t *token) {
    if (token->type != LEXER_TOKEN_PUNCT)
        return 0;

    switch (token->punct) {
        case LEXER_TOKEN_COMPOUND_RSHIFT: return LEXER_TOKEN_RSHIFT;
        case LEXER_TOKEN_COMPOUND_LSHIFT: return LEXER_TOKEN_LSHIFT;
        case LEXER_TOKEN_COMPOUND_ADD:    return '+';
        case LEXER_TOKEN_COMPOUND_AND:    return '&';
        case LEXER_TOKEN_COMPOUND_DIV:    return '/';
        case LEXER_TOKEN_COMPOUND_MOD:    return '%';
        case LEXER_TOKEN_COMPOUND_MUL:    return '*';
        case LEXER_TOKEN_COMPOUND_OR:     return '|';
        case LEXER_TOKEN_COMPOUND_SUB:    return '-';
        case LEXER_TOKEN_COMPOUND_XOR:    return '^';
        default:
            return 0;
    }
    return -1;
}

static ast_t *parse_expression_statement(void) {
    ast_t *ast = parse_statement_compound();
    parse_expect(')');
    data_type_t *type = ast_data_table[AST_DATA_VOID];
    if (list_length(ast->compound) > 0) {
        ast_t *last = list_tail(ast->compound);
        if (last)
            type = last->ctype;
    }
    ast->ctype = type;
    return ast;
}

static ast_t *parse_expression_primary(void) {
    lexer_token_t *token;

    if (!(token = lexer_next()))
        return NULL;

    if (lexer_ispunct(token, '(')) {

        if (parse_next('{')) {
            if (!opt_extension_test(EXTENSION_STATEMENTEXPRS))
                compile_error("statement expressions only supported in -std=gnuc or -std=licec");
            return parse_expression_statement();
        }

        ast_t *ast = parse_expression();
        parse_expect(')');
        return ast;
    }

    switch (token->type) {
        case LEXER_TOKEN_IDENTIFIER:
            return parse_generic(token->string);
        case LEXER_TOKEN_NUMBER:
            return parse_number(token->string);
        case LEXER_TOKEN_CHAR:
            return ast_new_integer(ast_data_table[AST_DATA_INT], token->character);
        case LEXER_TOKEN_STRING:
            return ast_new_string(token->string);
        case LEXER_TOKEN_PUNCT:
            lexer_unget(token);
            return NULL;
        default:
            break;
    }

    compile_ice("parse_expression_primary");
    return NULL;
}

static ast_t *parse_expression_subscript(ast_t *ast) {
    ast_t *subscript = parse_expression();
    if (!subscript)
        compile_error("expected subscript operand");
    parse_expect(']');
    ast_t *node = conv_usual('+', ast, subscript);
    return ast_new_unary(AST_TYPE_DEREFERENCE, node->ctype->pointer, node);
}

static data_type_t *parse_sizeof_operand(void) {
    lexer_token_t *token = lexer_next();

    if (lexer_ispunct(token, '(') && parse_type_check(lexer_peek())) {
        data_type_t *next = parse_function_parameter(NULL, true);
        parse_expect(')');
        return next;
    }

    lexer_unget(token);
    ast_t *expression = parse_expression_unary();

    if (opt_std_test(STANDARD_GNUC) || opt_std_test(STANDARD_LICEC)) {
        if (expression->type == AST_TYPE_DESIGNATOR)
            return expression->function.call.functionpointer->ctype;
        return expression->ctype;
    } else {
        if (expression->ctype->size == 0)
            compile_error("invalid operand to sizeof operator");
        return expression->ctype;
    }

    return NULL;
}

static ast_t *parse_sizeof(void) {
    data_type_t *type = parse_sizeof_operand();
    if (type->type == TYPE_VOID || type->type == TYPE_FUNCTION)
        return ast_new_integer(ast_data_table[AST_DATA_LONG], 1);
    return ast_new_integer(ast_data_table[AST_DATA_LONG], type->size);
}

static int parse_alignment(data_type_t *type) {
    int size = type->type == TYPE_ARRAY ? type->pointer->size : type->size;
    return MIN(size, ARCH_ALIGNMENT);
}

static ast_t *parse_alignof(void) {
    parse_expect('(');
    data_type_t *type = parse_function_parameter(NULL, true);
    parse_expect(')');
    return ast_new_integer(ast_data_table[AST_DATA_LONG], parse_alignment(type));
}

static ast_t *parse_structure_field(ast_t *structure) {
    if (structure->ctype->type != TYPE_STRUCTURE)
        compile_error("expected structure type, `%s' isn't structure type", ast_string(structure));
    lexer_token_t *name = lexer_next();
    if (name->type != LEXER_TOKEN_IDENTIFIER)
        compile_error("expected field name, got `%s' instead", lexer_token_string(name));

    data_type_t *field = table_find(structure->ctype->fields, name->string);
    if (!field)
        compile_error("structure has no such field `%s'", lexer_token_string(name));
    return ast_structure_reference(field, structure, name->string);
}

static void parse_static_assert(void) {
    parse_expect('(');
    int eval = parse_expression_evaluate();
    parse_expect(',');
    lexer_token_t *token = lexer_next();
    if (token->type != LEXER_TOKEN_STRING)
        compile_error("expected string");
    parse_expect(')');
    parse_expect(';');
    if (!eval)
        compile_error("static assertion failed: %s", token->string);
}

static ast_t *parse_label_address(void) {
    lexer_token_t *token = lexer_next();
    if (token->type != LEXER_TOKEN_IDENTIFIER)
        compile_error("expected identifier");
    ast_t *address = ast_label_address(token->string);
    list_push(ast_gotos, address);
    return address;
}

static ast_t *parse_expression_postfix_tail(ast_t *ast) {
    if (!ast)
        return NULL;
    for (;;) {
        if (parse_next('(')) {
            data_type_t *type = ast->ctype;
            if (type->type == TYPE_POINTER && type->pointer->type == TYPE_FUNCTION)
                return parse_function_pointer_call(ast);
            if (ast->type != AST_TYPE_DESIGNATOR)
                compile_error("expected function name");
            ast = parse_function_call(ast->function.name, ast->function.call.functionpointer);
            continue;
        }
        if (ast->type == AST_TYPE_DESIGNATOR && !ast->function.call.functionpointer)
            compile_error("undefined variable: %s", ast->function.name);

        if (parse_next('[')) {
            ast = parse_expression_subscript(ast);
            continue;
        }

        if (parse_next('.')) {
            ast = parse_structure_field(ast);
            continue;
        }

        if (parse_next(LEXER_TOKEN_ARROW)) {
            if (ast->ctype->type != TYPE_POINTER)
                compile_error("expected pointer type");
            ast = ast_new_unary(AST_TYPE_DEREFERENCE, ast->ctype->pointer, ast);
            ast = parse_structure_field(ast);
            continue;
        }

        lexer_token_t *peek = lexer_peek();
        if (parse_next(LEXER_TOKEN_INCREMENT) || parse_next(LEXER_TOKEN_DECREMENT)) {
            parse_semantic_lvalue(ast);
            int operation = lexer_ispunct(peek, LEXER_TOKEN_INCREMENT)
                ?   AST_TYPE_POST_INCREMENT
                :   AST_TYPE_POST_DECREMENT;

            return ast_new_unary(operation, ast->ctype, ast);
        }

        return ast;
    }

    return NULL;
}

static ast_t *parse_expression_postfix(void) {
    return parse_expression_postfix_tail(parse_expression_primary());
}

static ast_t *parse_expression_unary_address(void) {
    ast_t *operand = parse_expression_cast();
    if (operand->type == AST_TYPE_DESIGNATOR)
        return ast_designator_convert(operand);
    parse_semantic_lvalue(operand);
    return ast_new_unary(AST_TYPE_ADDRESS, ast_pointer(operand->ctype), operand);
}

static ast_t *parse_expression_unary_deref(void) {
    ast_t *operand = parse_expression_cast();
    operand = ast_designator_convert(operand);
    data_type_t *type = ast_array_convert(operand->ctype);
    if (type->type != TYPE_POINTER)
        compile_error("invalid type argument of unary ‘*’ (have ‘%s’)", ast_type_string(type));
    if (type->pointer->type == TYPE_FUNCTION)
        return operand;

    return ast_new_unary(AST_TYPE_DEREFERENCE, operand->ctype->pointer, operand);
}

static ast_t *parse_expression_unary_plus(void) {
    return parse_expression_cast();
}

static ast_t *parse_expression_unary_minus(void) {
    ast_t *expression = parse_expression_cast();
    // todo: semantic
    return ast_new_unary(AST_TYPE_NEGATE, expression->ctype, expression);
}

static ast_t *parse_expression_unary_bitcomp(void) {
    ast_t *expression = parse_expression_cast();
    expression = ast_designator_convert(expression);
    if (!ast_type_isinteger(expression->ctype))
        compile_error("invalid argument type ‘%s‘ to bit-complement", ast_type_string(expression->ctype));

    return ast_new_unary('~', expression->ctype, expression);
}

static ast_t *parse_expression_unary_not(void) {
    ast_t *operand = parse_expression_cast();
    operand = ast_designator_convert(operand);
    return ast_new_unary('!', ast_data_table[AST_DATA_INT], operand);
}

static ast_t *parse_expression_unary(void) {
    lexer_token_t *token = lexer_peek();
    if (parse_identifer_check(token, "sizeof")) {
        lexer_next();
        return parse_sizeof();
    }


    if (parse_identifer_check(token, "__alignof__"))
        goto alignof;

    if (parse_identifer_check(token, "_Alignof")) {
        if (!opt_std_test(STANDARD_C11) && !opt_std_test(STANDARD_LICEC))
            compile_error("_Alignof not supported in this version of the standard, try -std=c11 or -std=licec");

        alignof:
        lexer_next();
        return parse_alignof();
    }

    if (parse_next(LEXER_TOKEN_INCREMENT) || parse_next(LEXER_TOKEN_DECREMENT)) {
        ast_t *operand = parse_expression_unary();
        operand = ast_designator_convert(operand);
        parse_semantic_lvalue(operand);
        int operation = lexer_ispunct(token, LEXER_TOKEN_INCREMENT)
                            ? AST_TYPE_PRE_INCREMENT
                            : AST_TYPE_PRE_DECREMENT;

        return ast_new_unary(operation, operand->ctype, operand);
    }

    /* &&label for computed goto */
    if (parse_next(LEXER_TOKEN_AND))
        return parse_label_address();

    /* a more managable method for unary parsers */
    static ast_t *(*const parsers['~'-'!'+1])(void) = {
        [0]       = &parse_expression_unary_not,
        ['&'-'!'] = &parse_expression_unary_address,
        ['*'-'!'] = &parse_expression_unary_deref,
        ['+'-'!'] = &parse_expression_unary_plus,
        ['-'-'!'] = &parse_expression_unary_minus,
        ['~'-'!'] = &parse_expression_unary_bitcomp
    };

    for (const char *test = "!&*+-~"; *test; test++)
        if (parse_next(*test))
            return parsers[*test-'!']();

    return parse_expression_postfix();
}

ast_t *parse_expression_compound_literal(data_type_t *type) {
    char   *name = ast_label();
    list_t *init = init_entry(type);
    ast_t  *ast  = ast_variable_local(type, name);
    ast->variable.init = init;
    return ast;
}

static data_type_t *parse_expression_cast_type(void) {
    data_type_t *basetype = parse_declaration_specification(NULL);
    return parse_declarator(NULL, basetype, NULL, CDECL_CAST);
}

static ast_t *parse_expression_cast(void) {
    lexer_token_t *token = lexer_next();
    if (lexer_ispunct(token, '(') && parse_type_check(lexer_peek())) {
        data_type_t *type = parse_expression_cast_type();
        parse_expect(')');
        if (lexer_ispunct(lexer_peek(), '{')) {
            ast_t *ast = parse_expression_compound_literal(type);
            return parse_expression_postfix_tail(ast);
        }
        return ast_new_unary(AST_TYPE_EXPRESSION_CAST, type, parse_expression_cast());
    }
    lexer_unget(token);
    return parse_expression_unary();
}

static ast_t *parse_expression_multiplicative(void) {
    ast_t *ast = ast_designator_convert(parse_expression_cast());
    for (;;) {
             if (parse_next('*')) ast = conv_usual('*', ast, ast_designator_convert(parse_expression_cast()));
        else if (parse_next('/')) ast = conv_usual('/', ast, ast_designator_convert(parse_expression_cast()));
        else if (parse_next('%')) ast = conv_usual('%', ast, ast_designator_convert(parse_expression_cast()));
        else break;
    }
    return ast;
}

static ast_t *parse_expression_additive(void) {
    ast_t *ast = parse_expression_multiplicative();
    for (;;) {
             if (parse_next('+')) ast = conv_usual('+', ast, parse_expression_multiplicative());
        else if (parse_next('-')) ast = conv_usual('-', ast, parse_expression_multiplicative());
        else break;
    }
    return ast;
}

static ast_t *parse_expression_shift(void) {
    ast_t *ast = parse_expression_additive();
    for (;;) {
        lexer_token_t *token = lexer_next();
        int operation;
        if (lexer_ispunct(token, LEXER_TOKEN_LSHIFT))
            operation = AST_TYPE_LSHIFT;
        else if (lexer_ispunct(token, LEXER_TOKEN_RSHIFT))
            operation = ast->ctype->sign
                            ? AST_TYPE_RSHIFT
                            : AST_TYPE_LRSHIFT;
        else {
            lexer_unget(token);
            break;
        }
        ast_t *right = parse_expression_additive();
        // todo ensure integer
        data_type_t *result = conv_senority(ast->ctype, right->ctype);
        ast = ast_new_binary(result, operation, ast, right);
    }
    return ast;
}

static ast_t *parse_expression_relational(void) {
    ast_t *ast = parse_expression_shift();
    for (;;) {
             if (parse_next('<'))                ast = conv_usual('<',             ast, parse_expression_shift());
        else if (parse_next('>'))                ast = conv_usual('>',             ast, parse_expression_shift());
        else if (parse_next(LEXER_TOKEN_LEQUAL)) ast = conv_usual(AST_TYPE_LEQUAL, ast, parse_expression_shift());
        else if (parse_next(LEXER_TOKEN_GEQUAL)) ast = conv_usual(AST_TYPE_GEQUAL, ast, parse_expression_shift());
        else break;
    }
    return ast;
}

static ast_t *parse_expression_equality(void) {
    ast_t *ast = parse_expression_relational();
    if (parse_next(LEXER_TOKEN_EQUAL))
        return conv_usual(AST_TYPE_EQUAL, ast, parse_expression_equality());
    if (parse_next(LEXER_TOKEN_NEQUAL))
        return conv_usual(AST_TYPE_NEQUAL, ast, parse_expression_equality());
    return ast;
}

static ast_t *parse_expression_bitand(void) {
    ast_t *ast = parse_expression_equality();
    while (parse_next('&'))
        ast = conv_usual('&', ast, parse_expression_equality());
    return ast;
}

static ast_t *parse_expression_bitxor(void) {
    ast_t *ast = parse_expression_bitand();
    while (parse_next('^'))
        ast = conv_usual('^', ast, parse_expression_bitand());
    return ast;
}

static ast_t *parse_expression_bitor(void) {
    ast_t *ast = parse_expression_bitxor();
    while (parse_next('|'))
        ast = conv_usual('|', ast, parse_expression_bitxor());
    return ast;
}

static ast_t *parse_expression_logand(void) {
    ast_t *ast = parse_expression_bitor();
    while (parse_next(LEXER_TOKEN_AND))
        ast = ast_new_binary(ast_data_table[AST_DATA_INT], AST_TYPE_AND, ast, parse_expression_bitor());
    return ast;
}

static ast_t *parse_expression_logor(void) {
    ast_t *ast = parse_expression_logand();
    while (parse_next(LEXER_TOKEN_OR))
        ast = ast_new_binary(ast_data_table[AST_DATA_INT], AST_TYPE_OR, ast, parse_expression_logand());
    return ast;
}

ast_t *parse_expression_assignment(void) {
    ast_t         *ast   = parse_expression_logor();
    lexer_token_t *token = lexer_next();

    if (!token)
        return ast;

    if (lexer_ispunct(token, '?')) {
        ast_t *then = parse_expression_comma();
        parse_expect(':');
        ast_t *last = parse_expression_conditional();
        ast_t *operand = (opt_extension_test(EXTENSION_OMITOPCOND)) ? last : then;
        return ast_ternary(operand->ctype, ast, then, last);
    }

    int compound   = parse_operation_compound_operator(token);
    int reclassify = parse_operation_reclassify(compound);
    if (lexer_ispunct(token, '=') || compound) {
        ast_t *value = parse_expression_assignment();
        if (lexer_ispunct(token, '=') || compound)
            parse_semantic_lvalue(ast);

        ast_t *right = compound ? conv_usual(reclassify, ast, value) : value;
        if (conv_capable(right->ctype) && ast->ctype->type != right->ctype->type)
            right = ast_type_convert(ast->ctype, right);
        return ast_new_binary(ast->ctype, '=', ast, right);
    }

    lexer_unget(token);
    return ast;
}

static ast_t *parse_expression_comma(void) {
    ast_t *ast = parse_expression_assignment();
    while (parse_next(',')) {
        ast_t *expression = parse_expression_assignment();
        ast = ast_new_binary(expression->ctype, ',', ast, expression);
    }
    return ast;
}

static ast_t *parse_expression(void) {
    ast_t *ast = parse_expression_comma();
    if (!ast)
        compile_error("expression expected");
    return ast;
}

static ast_t *parse_expression_optional(void) {
    return parse_expression_comma();
}

static ast_t *parse_expression_condition(void) {
    ast_t *cond = parse_expression();
    if (ast_type_isfloating(cond->ctype))
        return ast_type_convert(ast_data_table[TYPE_BOOL], cond);
    return cond;
}

static ast_t *parse_expression_conditional(void) {
    ast_t *ast = parse_expression_logor();
    if (!parse_next('?'))
        return ast;
    ast_t *then = parse_expression_comma();
    parse_expect(':');
    ast_t *last = parse_expression_conditional();
    return ast_ternary(last->ctype, ast, then, last);
}

int parse_expression_evaluate(void) {
    return parse_evaluate(parse_expression_conditional());
}

static bool parse_type_check(lexer_token_t *token) {
    if (token->type != LEXER_TOKEN_IDENTIFIER)
        return false;

    static const char *keywords[] = {
        "char",     "short",  "int",      "long",     "float",    "double",
        "struct",   "union",  "signed",   "unsigned", "enum",     "void",
        "typedef",  "extern", "static",   "auto",     "register", "const",
        "volatile", "inline", "restrict", "typeof",   "_Bool",

        /*
         * Ironically there is also another way to specific some keywords
         * in C due to compilers being sneaky. Thus some code may use
         * things like, __static__, or __typeof__, which are compilers
         * reserved implementations of those keywords before the standard
         * was ratified.
         */
        "__static__",
        "__typeof__",
    };

    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
        if (!strcmp(keywords[i], token->string))
            return true;

    if (table_find(parse_typedefs, token->string))
        return true;

    return false;
}

/* struct / union */
static char *parse_memory_tag(void) {
    lexer_token_t *token = lexer_next();
    if (token->type == LEXER_TOKEN_IDENTIFIER)
        return token->string;
    lexer_unget(token);
    return NULL;
}

static int parse_memory_fields_padding(int offset, data_type_t *type) {
    int size = type->type == TYPE_ARRAY ? type->pointer->size : type->size;
    size = MIN(size, ARCH_ALIGNMENT);
    return (offset % size == 0) ? 0 : size - offset % size;
}

static void parse_memory_fields_flexiblize(list_t *fields) {
    list_iterator_t *it = list_iterator(fields);
    while (!list_iterator_end(it)) {
        pair_t      *pair = list_iterator_next(it);
        char        *name = pair->first;
        data_type_t *type = pair->second;

        if (type->type != TYPE_ARRAY)
            continue;

        if (!opt_std_test(STANDARD_GNUC) && !opt_std_test(STANDARD_LICEC)) {
            if (type->length == 0)
                type->length = -1;
        }

        if (type->length == -1) {
            if (!list_iterator_end(it))
                compile_error("flexible field %s can only appear as the last field in a structure", name);
            if (list_length(fields) == 1)
                compile_error("flexible field %s as only field in structure is illegal", name);
            type->size = 0;
        }
    }
}

static void parse_memory_fields_squash(table_t *table, data_type_t *unnamed, int offset) {
    for (list_iterator_t *it = list_iterator(table_keys(unnamed->fields)); !list_iterator_end(it); ) {
        char         *name = list_iterator_next(it);
        data_type_t  *type = ast_type_copy(table_find(unnamed->fields, name));
        type->offset += offset;
        table_insert(table, name, type);
    }
}

static int parse_bitfield_maybe(char *name, data_type_t *through) {
    if (!parse_next(':'))
        return -1;
    if (!ast_type_isinteger(through))
        compile_error("invalid type for bitfield size %s", ast_type_string(through));
    int evaluate = parse_expression_evaluate();
    if (evaluate < 0 || through->size * 8 < evaluate)
        compile_error("invalid bitfield size %s: %d", ast_type_string(through), evaluate);
    if (evaluate == 0 && name)
        compile_error("zero sized bitfield cannot be named");
    return evaluate;
}

static list_t *parse_memory_fields_intermediate(void) {
    list_t *list = list_create();
    for (;;) {

        lexer_token_t *next = lexer_next();
        if (parse_identifer_check(next, "_Static_assert")) {
            if (!opt_std_test(STANDARD_C11) && !opt_std_test(STANDARD_LICEC))
                compile_error("_Static_assert not supported in this version of the standard, try -std=c11 or -std=licec");

            parse_static_assert();
            continue;
        }

        lexer_unget(next);

        if (!parse_type_check(lexer_peek()))
            break;

        data_type_t *basetype = parse_declaration_specification(NULL);

        if (basetype->type == TYPE_STRUCTURE && parse_next(';')) {
            list_push(list, pair_create(NULL, basetype));
            continue;
        }

        for (;;) {
            char        *name      = NULL;
            data_type_t *fieldtype = parse_declarator(&name, basetype, NULL, CDECL_TYPEONLY);
            parse_semantic_notvoid(fieldtype);

            /* copy though for bitfield */
            fieldtype                = ast_type_copy(fieldtype);
            fieldtype->bitfield.size = parse_bitfield_maybe(name, fieldtype);

            list_push(list, pair_create(name, fieldtype));

            if (parse_next(','))
                continue;
            if (lexer_ispunct(lexer_peek(), '}'))
                compile_warn("no semicolon at end of struct or union");
            else
                parse_expect(';');
            break;
        }
    }
    parse_expect('}');
    return list;
}

static void parse_bitfield_update(int *offset, int *bitfield) {
    *offset  += (*bitfield + 8) / 8;
    *bitfield = -1;
}

static table_t *parse_struct_offset(list_t *fields, int *size) {
    int              offset   = 0;
    int              bitfield = -1;
    list_iterator_t *it       = list_iterator(fields);
    table_t         *table    = table_create(NULL);

    while (!list_iterator_end(it)) {
        pair_t      *pair = list_iterator_next(it);
        char        *name = pair->first;
        data_type_t *type = pair->second;

        if (!name && type->type == TYPE_STRUCTURE) {
            parse_bitfield_update(&offset, &bitfield);
            parse_memory_fields_squash(table, type, offset);
            offset += type->size;
            continue;
        }
        if (type->bitfield.size == 0) {
            parse_bitfield_update(&offset, &bitfield);
            offset += parse_memory_fields_padding(offset, type);
            bitfield = 0;
            continue;
        }
        if (type->bitfield.size >= 0) {
            /* bitfield space */
            int space = type->size * 8 - bitfield;
            if (0 <= bitfield && type->bitfield.size <= space) {
                type->bitfield.offset = bitfield;
                type->offset          = offset;
            } else {
                parse_bitfield_update(&offset, &bitfield);
                offset               += parse_memory_fields_padding(offset, type);
                type->offset          = offset;
                type->bitfield.offset = 0;
            }
            bitfield = type->bitfield.size;
        } else {
            parse_bitfield_update(&offset, &bitfield);
            offset += parse_memory_fields_padding(offset, type);
            type->offset = offset;
            offset += type->size;
        }
        if (name)
            table_insert(table, name, type); /* no copy needed */
    }
    parse_bitfield_update(&offset, &bitfield); /* stage it */
    *size = offset;
    return table;
}

static table_t *parse_union_offset(list_t *fields, int *size) {
    int              maxsize = 0;
    list_iterator_t *it      = list_iterator(fields);
    table_t         *table   = table_create(NULL);

    while (!list_iterator_end(it)) {
        pair_t      *pair = list_iterator_next(it);
        char        *name = pair->first;
        data_type_t *type = pair->second;

        maxsize = MAX(maxsize, type->size);
        if (!name && type->type == TYPE_STRUCTURE) {
            parse_memory_fields_squash(table, type, 0);
            continue;
        }
        type->offset = 0;
        if (type->bitfield.size >= 0)
            type->bitfield.offset = 0;
        if (name)
            table_insert(table, name, type);
    }
    *size = maxsize;
    return table;
}

static table_t *parse_memory_fields(int *size, bool isstruct) {
    if (!parse_next('{'))
        return NULL;
    list_t *fields = parse_memory_fields_intermediate();

    /* make flexible if it can be made flexible */
    parse_memory_fields_flexiblize(fields);

    return (isstruct)
                ? parse_struct_offset(fields, size)
                : parse_union_offset(fields, size);
}

static data_type_t *parse_tag_definition(table_t *table, bool isstruct) {
    char        *tag = parse_memory_tag();
    data_type_t *r;

    if (tag) {
        if (!(r = table_find(table, tag))) {
            r = ast_structure_new(NULL, 0, isstruct);
            table_insert(table, tag, r);
        }
    } else {
        r = ast_structure_new(NULL, 0, isstruct);
    }

    int      size   = 0;
    table_t *fields = parse_memory_fields(&size, isstruct);

    if (r && !fields)
        return r;

    if (r && fields) {
        r->fields = fields;
        r->size   = size;
        return r;
    }

    return NULL;
}

/* enum */
data_type_t *parse_enumeration(void) {
    lexer_token_t *token = lexer_next();
    if (token->type == LEXER_TOKEN_IDENTIFIER)
        token = lexer_next();
    if (!lexer_ispunct(token, '{')) {
        lexer_unget(token);
        return ast_data_table[AST_DATA_INT];
    }
    int accumulate = 0;
    for (;;) {
        token = lexer_next();
        if (lexer_ispunct(token, '}'))
            break;

        if (token->type != LEXER_TOKEN_IDENTIFIER)
            compile_error("expected identifier before ‘%s’ token", lexer_token_string(token));

        char *name = token->string;
        token = lexer_next();
        if (lexer_ispunct(token, '='))
            accumulate = parse_expression_evaluate();
        else
            lexer_unget(token);

        ast_t *constval = ast_new_integer(ast_data_table[AST_DATA_INT], accumulate++);
        table_insert(ast_localenv ? ast_localenv : ast_globalenv, name, constval);
        token = lexer_next();
        if (lexer_ispunct(token, ','))
            continue;
        if (lexer_ispunct(token, '}'))
            break;

        compile_ice("parse_enumeration");
    }
    return ast_data_table[AST_DATA_INT];
}

data_type_t *parse_typeof(void) {
    parse_expect('(');
    data_type_t *type = parse_type_check(lexer_peek())
                            ? parse_expression_cast_type()
                            : parse_expression_comma()->ctype;
    parse_expect(')');
    return type;
}

data_type_t *parse_structure(void) {
    return parse_tag_definition(ast_structures, true);
}

data_type_t *parse_union(void) {
    return parse_tag_definition(ast_unions, false);
}

data_type_t *parse_typedef_find(const char *key) {
    return table_find(parse_typedefs, key);
}

/* declarator */
static data_type_t *parse_declaration_specification(storage_t *rstorage) {
    lexer_token_t *token   = lexer_peek();
    if (!token || token->type != LEXER_TOKEN_IDENTIFIER)
        compile_ice("declaration specification");

    data_type_t *decl_spec(storage_t *);
    return decl_spec(rstorage);
}

static data_type_t *parse_function_parameter(char **name, bool next) {
    data_type_t *basetype;
    storage_t    storage;

    basetype = parse_declaration_specification(&storage);
    return parse_declarator(name, basetype, NULL, next ? CDECL_TYPEONLY : CDECL_PARAMETER);
}

static ast_t *parse_statement_if(void) {
    lexer_token_t *token;
    ast_t  *cond;
    ast_t *then;
    ast_t *last;

    parse_expect('(');
    cond = parse_expression_condition();
    parse_expect(')');


    then  = parse_statement();
    token = lexer_next();

    if (!token || token->type != LEXER_TOKEN_IDENTIFIER || strcmp(token->string, "else")) {
        lexer_unget(token);
        return ast_if(cond, then, NULL);
    }

    last = parse_statement();
    return ast_if(cond, then, last);
}

static ast_t *parse_statement_declaration_semicolon(void) {
    lexer_token_t *token = lexer_next();
    if (lexer_ispunct(token, ';'))
        return NULL;
    lexer_unget(token);
    list_t *list = list_create();
    parse_statement_declaration(list);
    return list_shift(list);
}

static ast_t *parse_statement_for(void) {
    parse_expect('(');
    ast_localenv = table_create(ast_localenv);
    ast_t *init = parse_statement_declaration_semicolon();
    ast_t *cond = parse_expression_optional();
    if (cond && ast_type_isfloating(cond->ctype))
        cond = ast_type_convert(ast_data_table[AST_DATA_BOOL], cond);
    parse_expect(';');
    ast_t *step = parse_expression_optional();
    parse_expect(')');
    ast_t *body = parse_statement();
    ast_localenv = table_parent(ast_localenv);
    return ast_for(init, cond, step, body);
}

static ast_t *parse_statement_while(void) {
    parse_expect('(');
    ast_t *cond = parse_expression_condition();
    parse_expect(')');
    ast_t *body = parse_statement();
    return ast_while(cond, body);
}

static ast_t *parse_statement_do(void) {
    ast_t         *body  = parse_statement();
    lexer_token_t *token = lexer_next();

    if (!parse_identifer_check(token, "while"))
        compile_error("expected ‘while’ before ‘%s’ token", lexer_token_string(token));

    parse_expect('(');
    ast_t *cond = parse_expression_condition();
    parse_expect(')');
    parse_expect(';');

    return ast_do(cond, body);
}

static ast_t *parse_statement_break(void) {
    parse_expect(';');
    return ast_make(AST_TYPE_STATEMENT_BREAK);
}

static ast_t *parse_statement_continue(void) {
    parse_expect(';');
    return ast_make(AST_TYPE_STATEMENT_CONTINUE);
}

static ast_t *parse_statement_switch(void) {
    parse_expect('(');
    ast_t *expression = parse_expression();

    if (!ast_type_isinteger(expression->ctype)) {
        compile_error(
            "switch statement requires expression of integer type (‘%s‘ invalid)",
            ast_type_string(expression->ctype)
        );
    }

    parse_expect(')');
    ast_t *body = parse_statement();
    return ast_switch(expression, body);
}

static ast_t *parse_statement_case(void) {
    int begin = parse_expression_evaluate();
    int end;
    lexer_token_t *token = lexer_next();
    if (parse_identifer_check(token, "..."))
        end = parse_expression_evaluate();
    else {
        end = begin;
        lexer_unget(token);
    }
    parse_expect(':');
    if (begin > end)
        compile_warn("empty case range specified");
    return ast_case(begin, end);
}

static ast_t *parse_statement_default(void) {
    parse_expect(':');
    return ast_make(AST_TYPE_STATEMENT_DEFAULT);
}

static ast_t *parse_statement_return(void) {
    ast_t *val = parse_expression_optional();
    parse_expect(';');
    if (val)
        return ast_return(ast_type_convert(ast_data_table[AST_DATA_FUNCTION]->returntype, val));
    return ast_return(NULL);
}

static ast_t *parse_statement_goto(void) {
    /* deal with computed goto */
    if (parse_next('*')) {
        ast_t *expression = parse_expression_cast();
        if (expression->ctype->type != TYPE_POINTER)
            compile_error("expected pointer type for computed goto");
        return ast_goto_computed(expression);
    }

    /* otherwise the traditional route */

    lexer_token_t *token = lexer_next();
    if (!token || token->type != LEXER_TOKEN_IDENTIFIER)
        compile_error("expected label in goto statement");
    parse_expect(';');

    ast_t *node = ast_goto(token->string);
    list_push(ast_gotos, node);

    return node;
}

static void parse_label_backfill(void) {
    for (list_iterator_t *it = list_iterator(ast_gotos); !list_iterator_end(it); ) {
        ast_t *source      = list_iterator_next(it);
        char  *label       = source->gotostmt.label;
        ast_t *destination = table_find(ast_labels, label);

        if (!destination)
            compile_error("undefined label ‘%s‘", label);
        if (destination->gotostmt.where)
            source->gotostmt.where = destination->gotostmt.where;
        else
            source->gotostmt.where = destination->gotostmt.where = ast_label();
    }
}

static ast_t *parse_label(lexer_token_t *token) {
    parse_expect(':');
    char  *label = token->string;
    ast_t *node  = ast_new_label(label);

    if (table_find(ast_labels, label))
        compile_error("duplicate label ‘%s‘", label);
    table_insert(ast_labels, label, node);

    return node;
}

static ast_t *parse_statement(void) {
    lexer_token_t *token = lexer_next();
    ast_t         *ast;

    if (lexer_ispunct        (token, '{'))        return parse_statement_compound();
    if (parse_identifer_check(token, "if"))       return parse_statement_if();
    if (parse_identifer_check(token, "for"))      return parse_statement_for();
    if (parse_identifer_check(token, "while"))    return parse_statement_while();
    if (parse_identifer_check(token, "do"))       return parse_statement_do();
    if (parse_identifer_check(token, "return"))   return parse_statement_return();
    if (parse_identifer_check(token, "switch"))   return parse_statement_switch();
    if (parse_identifer_check(token, "case"))     return parse_statement_case();
    if (parse_identifer_check(token, "default"))  return parse_statement_default();
    if (parse_identifer_check(token, "break"))    return parse_statement_break();
    if (parse_identifer_check(token, "continue")) return parse_statement_continue();
    if (parse_identifer_check(token, "goto"))     return parse_statement_goto();

    if (token->type == LEXER_TOKEN_IDENTIFIER && lexer_ispunct(lexer_peek(), ':'))
        return parse_label(token);

    lexer_unget(token);

    ast = parse_expression_optional();
    parse_expect(';');

    return ast;
}

static void parse_statement_declaration(list_t *list){
    lexer_token_t *token = lexer_peek();
    if (!token)
        compile_error("statement declaration with unexpected ending");
    if (parse_type_check(token)) {
        parse_declaration(list, ast_variable_local);
    } else {
        lexer_token_t *next = lexer_next();
        if (parse_identifer_check(next, "_Static_assert"))
            return parse_static_assert();
        else
            lexer_unget(next);
        ast_t *statement = parse_statement();
        if (statement)
            list_push(list, statement);
    }
}

static ast_t *parse_statement_compound(void) {
    ast_localenv = table_create(ast_localenv);
    list_t *statements = list_create();
    for (;;) {
        lexer_token_t *token = lexer_next();
        if (lexer_ispunct(token, '}'))
            break;

        lexer_unget(token);
        parse_statement_declaration(statements);
    }
    ast_localenv = table_parent(ast_localenv);
    return ast_compound(statements);
}

static data_type_t *parse_function_parameters(list_t *paramvars, data_type_t *returntype) {
    bool           typeonly   = !paramvars;
    list_t        *paramtypes = list_create();
    lexer_token_t *token      = lexer_next();
    lexer_token_t *next       = lexer_next();

    if (parse_identifer_check(token, "void") && lexer_ispunct(next, ')'))
        return ast_prototype(returntype, paramtypes, false);
    lexer_unget(next);
    if (lexer_ispunct(token, ')'))
        return ast_prototype(returntype, paramtypes, true);
    lexer_unget(token);

    for (;;) {
        token = lexer_next();
        if (parse_identifer_check(token, "...")) {
            if (list_length(paramtypes) == 0)
                compile_ice("parse_function_parameters");
            parse_expect(')');
            return ast_prototype(returntype, paramtypes, true);
        } else {
            lexer_unget(token);
        }

        char        *name;
        data_type_t *ptype = parse_function_parameter(&name, typeonly);
        parse_semantic_notvoid(ptype);

        if (ptype->type == TYPE_ARRAY)
            ptype = ast_pointer(ptype->pointer);
        list_push(paramtypes, ptype);

        if (!typeonly)
            list_push(paramvars, ast_variable_local(ptype, name));

        lexer_token_t *token = lexer_next();
        if (lexer_ispunct(token, ')'))
            return ast_prototype(returntype, paramtypes, false);

        if (!lexer_ispunct(token, ','))
            compile_ice("parse_function_parameters");
    }
}

static ast_t *parse_function_definition(data_type_t *functype, char *name, list_t *parameters) {
    ast_localenv                      = table_create(ast_localenv);
    ast_locals                        = list_create();
    ast_data_table[AST_DATA_FUNCTION] = functype;

    table_insert(ast_localenv, "__func__", ast_new_string(name));

    ast_t *body = parse_statement_compound();
    ast_t *r    = ast_function(functype, name, parameters, body, ast_locals);

    table_insert(ast_globalenv, name, r);

    ast_data_table[AST_DATA_FUNCTION] = NULL;
    ast_localenv                      = NULL;
    ast_locals                        = NULL;

    return r;
}

static bool parse_function_definition_check(void) {
    list_t *buffer = list_create();
    int     nests  = 0;
    bool    paren  = false;
    bool    ready  = true;

    for (;;) {

        lexer_token_t *token = lexer_next();
        list_push(buffer, token);

        if (!token)
            compile_error("function definition with unexpected ending");

        if (nests == 0 && paren && lexer_ispunct(token, '{'))
            break;

        if (nests == 0 && (lexer_ispunct(token, ';')
                         ||lexer_ispunct(token, ',')
                         ||lexer_ispunct(token, '=')))
        {
            ready = false;
            break;
        }

        if (lexer_ispunct(token, '('))
            nests++;

        if (lexer_ispunct(token, ')')) {
            if (nests == 0)
                compile_error("unmatched parenthesis");
            paren = true;
            nests--;
        }
    }

    while (list_length(buffer) > 0)
        lexer_unget(list_pop(buffer));

    return ready;
}

static ast_t *parse_function_definition_intermediate(void) {
    char        *name;
    storage_t    storage;
    list_t      *parameters = list_create();
    data_type_t *basetype   = ast_data_table[AST_DATA_INT];

    if (parse_type_check(lexer_peek()))
        basetype = parse_declaration_specification(&storage);
    else
        compile_warn("return type defaults to ’int’");

    ast_localenv = table_create(ast_globalenv);
    ast_labels   = table_create(NULL);
    ast_gotos    = list_create();

    data_type_t *functype = parse_declarator(&name, basetype, parameters, CDECL_BODY);
    functype->isstatic = !!(storage == STORAGE_STATIC);
    ast_variable_global(functype, name);
    parse_expect('{');
    ast_t *value = parse_function_definition(functype, name, parameters);

    parse_label_backfill();

    ast_localenv = NULL;
    return value;
}

static data_type_t *parse_declarator_direct_restage(data_type_t *basetype, list_t *parameters) {
    lexer_token_t *token = lexer_next();
    if (lexer_ispunct(token, '[')) {
        int length;
        token = lexer_next();
        if (lexer_ispunct(token, ']'))
            length = -1;
        else {
            lexer_unget(token);
            length = parse_expression_evaluate();
            parse_expect(']');
        }

        data_type_t *type = parse_declarator_direct_restage(basetype, parameters);
        if (type->type == TYPE_FUNCTION)
            compile_error("array of functions");
        return ast_array(type, length);
    }
    if (lexer_ispunct(token, '(')) {
        if (basetype->type == TYPE_FUNCTION)
            compile_error("function returning function");
        if (basetype->type == TYPE_ARRAY)
            compile_error("function returning array");
        return parse_function_parameters(parameters, basetype);
    }
    lexer_unget(token);
    return basetype;
}

static void parse_qualifiers_skip(void) {
    for (;;) {
        lexer_token_t *token = lexer_next();
        if (parse_identifer_check(token, "const")
         || parse_identifer_check(token, "volatile")
         || parse_identifer_check(token, "restrict")) {
            continue;
        }
        lexer_unget(token);
        return;
    }
}

static data_type_t *parse_declarator_direct(char **rname, data_type_t *basetype, list_t *parameters, cdecl_t context) {
    lexer_token_t *token = lexer_next();
    lexer_token_t *next  = lexer_peek();

    if (lexer_ispunct(token, '(') && !parse_type_check(next) && !lexer_ispunct(next, ')')) {
        data_type_t *stub = ast_type_stub();
        data_type_t *type = parse_declarator_direct(rname, stub, parameters, context);
        parse_expect(')');
        *stub = *parse_declarator_direct_restage(basetype, parameters);
        return type;
    }

    if (lexer_ispunct(token, '*')) {
        parse_qualifiers_skip();
        data_type_t *stub = ast_type_stub();
        data_type_t *type = parse_declarator_direct(rname, stub, parameters, context);
        *stub = *ast_pointer(basetype);
        return type;
    }

    if (token->type == LEXER_TOKEN_IDENTIFIER) {
        if (context == CDECL_CAST)
            compile_error("wasn't expecting identifier `%s'", lexer_token_string(token));
        *rname = token->string;
        return parse_declarator_direct_restage(basetype, parameters);
    }

    if (context == CDECL_BODY || context == CDECL_PARAMETER)
        compile_error("expected identifier, `(` or `*` for declarator");

    lexer_unget(token);

    return parse_declarator_direct_restage(basetype, parameters);
}

static void parse_array_fix(data_type_t *type) {
    if (type->type == TYPE_ARRAY) {
        parse_array_fix(type->pointer);
        type->size = type->length * type->pointer->size;
    } else if (type->type == TYPE_POINTER) {
        parse_array_fix(type->pointer);
    } else if (type->type == TYPE_FUNCTION) {
        parse_array_fix(type->returntype);
    }
}

static data_type_t *parse_declarator(char **rname, data_type_t *basetype, list_t *parameters, cdecl_t context) {
    data_type_t *type = parse_declarator_direct(rname, basetype, parameters, context);
    parse_array_fix(type);
    return type;
}

static void parse_declaration(list_t *list, ast_t *(*make)(data_type_t *, char *)) {
    storage_t      storage;
    data_type_t   *basetype = parse_declaration_specification(&storage);
    lexer_token_t *token    = lexer_next();

    if (lexer_ispunct(token, ';'))
        return;

    lexer_unget(token);

    for (;;) {
        char        *name = NULL;
        data_type_t *type = parse_declarator(&name, ast_type_copy_incomplete(basetype), NULL, CDECL_BODY);

        if (storage == STORAGE_STATIC)
            type->isstatic = true;

        token = lexer_next();
        if (lexer_ispunct(token, '=')) {
            if (storage == STORAGE_TYPEDEF)
                compile_error("invalid use of typedef");
            parse_semantic_notvoid(type);
            ast_t *var = make(type, name);
            list_push(list, ast_declaration(var, init_entry(var->ctype)));
            token = lexer_next();
        } else if (storage == STORAGE_TYPEDEF) {
            table_insert(parse_typedefs, name, type);
        } else if (type->type == TYPE_FUNCTION) {
            make(type, name);
        } else {
            ast_t *var = make(type, name);
            if (storage != STORAGE_EXTERN)
                list_push(list, ast_declaration(var, NULL));
        }
        if (lexer_ispunct(token, ';'))
            return;
        if (!lexer_ispunct(token, ','))
            compile_ice("confused %X", token);
    }
}

list_t *parse_run(void) {
    list_t *list = list_create();
    for (;;) {
        if (!lexer_peek())
            return list;
        if (parse_function_definition_check())
            list_push(list, parse_function_definition_intermediate());
        else
            parse_declaration(list, &ast_variable_global);
    }
    return NULL;
}

void parse_init(void) {
    data_type_t *voidp = ast_pointer(ast_data_table[AST_DATA_VOID]);
    data_type_t *type  = ast_prototype(voidp, list_create(), true);
    data_type_t *addr  = ast_prototype(voidp, list_default(ast_data_table[AST_DATA_ULONG]), true);

    table_insert(ast_globalenv, "__builtin_va_start",       ast_variable_global(type, "__builtin_va_start"));
    table_insert(ast_globalenv, "__builtin_va_arg",         ast_variable_global(type, "__builtin_va_arg"));
    table_insert(ast_globalenv, "__builtin_return_address", ast_variable_global(addr, "__buitlin_return_address"));
}
