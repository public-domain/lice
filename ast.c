#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "lice.h"
#include "ast.h"
#include "lexer.h"
#include "conv.h"

data_type_t *ast_data_table[AST_DATA_COUNT] = {
    &(data_type_t) { TYPE_VOID,    0,                      true },   /* void                */
    &(data_type_t) { TYPE_BOOL,    ARCH_TYPE_SIZE_INT,     false},   /* _Bool               */
    &(data_type_t) { TYPE_LONG,    ARCH_TYPE_SIZE_LONG,    true },   /* long                */
    &(data_type_t) { TYPE_LLONG,   ARCH_TYPE_SIZE_LLONG,   true },   /* long long           */
    &(data_type_t) { TYPE_INT,     ARCH_TYPE_SIZE_INT,     true },   /* int                 */
    &(data_type_t) { TYPE_SHORT,   ARCH_TYPE_SIZE_SHORT,   true },   /* short               */
    &(data_type_t) { TYPE_CHAR,    ARCH_TYPE_SIZE_CHAR,    true },   /* char                */
    &(data_type_t) { TYPE_FLOAT,   ARCH_TYPE_SIZE_FLOAT,   true },   /* float               */
    &(data_type_t) { TYPE_DOUBLE,  ARCH_TYPE_SIZE_DOUBLE,  true },   /* double              */
    &(data_type_t) { TYPE_LDOUBLE, ARCH_TYPE_SIZE_LDOUBLE, true },   /* long double         */
    &(data_type_t) { TYPE_LONG,    ARCH_TYPE_SIZE_LONG,    false },  /* unsigned long       */
    &(data_type_t) { TYPE_LLONG,   ARCH_TYPE_SIZE_LLONG,   false },  /* unsigned long long  */
    NULL                                                             /* function            */
};

data_type_t *ast_data_function = NULL;
list_t      *ast_locals        = NULL;
list_t      *ast_gotos         = NULL;
table_t     *ast_labels        = NULL;
table_t     *ast_globalenv     = &SENTINEL_TABLE;
table_t     *ast_localenv      = &SENTINEL_TABLE;
table_t     *ast_structures    = &SENTINEL_TABLE;
table_t     *ast_unions        = &SENTINEL_TABLE;

bool ast_struct_compare(data_type_t *a, data_type_t *b) {
    list_t          *la;
    list_t          *lb;
    list_iterator_t *lait;
    list_iterator_t *lbit;

    if (a->type != b->type)
        return false;

    switch (a->type) {
        case TYPE_ARRAY:
            if (a->length == b->length)
                return ast_struct_compare(a->pointer, b->pointer);
            return false;

        case TYPE_POINTER:
            return ast_struct_compare(a->pointer, b->pointer);

        case TYPE_STRUCTURE:
            if (a->isstruct != b->isstruct)
                return false;

            la = table_keys(a->fields);
            lb = table_keys(b->fields);

            if (list_length(la) != list_length(lb))
                return false;

            lait = list_iterator(la);
            lbit = list_iterator(lb);

            while (!list_iterator_end(lait))
                if (!ast_struct_compare(list_iterator_next(lait), list_iterator_next(lbit)))
                    return false;

        default:
            return true;
    }
    return false;
}

data_type_t *ast_result_type(int operation, data_type_t *type) {
    switch (operation) {
        case AST_TYPE_LEQUAL:
        case AST_TYPE_GEQUAL:
        case AST_TYPE_EQUAL:
        case AST_TYPE_NEQUAL:
        case '<':
        case '>':
            return ast_data_table[AST_DATA_INT];
        default:
            return conv_senority(type, type);
    }
}

ast_t *ast_copy(ast_t *ast) {
    ast_t *copy = memory_allocate(sizeof(ast_t));
    *copy = *ast;
    return copy;
}

ast_t *ast_structure_reference(data_type_t *type, ast_t *structure, char *name) {
    return ast_copy(&(ast_t) {
        .type       = AST_TYPE_STRUCT,
        .ctype      = type,
        .structure  = structure,
        .field      = name
    });
}

ast_t *ast_new_unary(int type, data_type_t *data, ast_t *operand) {
    return ast_copy(&(ast_t) {
        .type          = type,
        .ctype         = data,
        .unary.operand = operand
    });
}

ast_t *ast_new_binary(data_type_t *ctype, int type, ast_t *left, ast_t *right) {
    ast_t *ast = ast_copy(&(ast_t){
        .type  = type,
        .ctype = ctype
    });
    ast->left  = left;
    ast->right = right;
    return ast;
}

ast_t *ast_new_integer(data_type_t *type, int value) {
    return ast_copy(&(ast_t) {
        .type    = AST_TYPE_LITERAL,
        .ctype   = type,
        .integer = value
    });
}

ast_t *ast_new_floating(data_type_t *type, double value) {
    return ast_copy(&(ast_t){
        .type           = AST_TYPE_LITERAL,
        .ctype          = type,
        .floating.value = value,
        .floating.label = NULL
    });
}

ast_t *ast_new_string(char *value) {
    return ast_copy(&(ast_t) {
        .type         = AST_TYPE_STRING,
        .ctype        = ast_array(ast_data_table[AST_DATA_CHAR], strlen(value) + 1),
        .string.data  = value,
        .string.label = NULL
    });
}

ast_t *ast_variable_local(data_type_t *type, char *name) {
    ast_t *ast = ast_copy(&(ast_t){
        .type          = AST_TYPE_VAR_LOCAL,
        .ctype         = type,
        .variable.name = name
    });
    if (ast_localenv)
        table_insert(ast_localenv, name, ast);
    if (ast_locals)
        list_push(ast_locals, ast);
    return ast;
}

ast_t *ast_variable_global(data_type_t *type, char *name) {
    ast_t *ast = ast_copy(&(ast_t){
        .type           = AST_TYPE_VAR_GLOBAL,
        .ctype          = type,
        .variable.name  = name,
        .variable.label = name
    });
    table_insert(ast_globalenv, name, ast);
    return ast;
}

ast_t *ast_function(data_type_t *ret, char *name, list_t *params, ast_t *body, list_t *locals) {
    return ast_copy(&(ast_t) {
        .type             = AST_TYPE_FUNCTION,
        .ctype            = ret,
        .function.name    = name,
        .function.params  = params,
        .function.locals  = locals,
        .function.body    = body
    });
}

ast_t *ast_designator(char *name, ast_t *func) {
    return ast_copy(&(ast_t){
        .type                          = AST_TYPE_DESIGNATOR,
        .ctype                         = ast_data_table[AST_DATA_VOID],
        .function.name                 = name,
        .function.call.functionpointer = func
    });
}

ast_t *ast_pointercall(ast_t *functionpointer, list_t *args) {
    return ast_copy(&(ast_t) {
        .type                          = AST_TYPE_POINTERCALL,
        .ctype                         = functionpointer->ctype->pointer->returntype,
        .function.call.functionpointer = functionpointer,
        .function.call.args            = args
    });
}

ast_t *ast_call(data_type_t *type, char *name, list_t *arguments) {
    return ast_copy(&(ast_t) {
        .type               = AST_TYPE_CALL,
        .ctype              = type->returntype,
        .function.call.args = arguments,
        .function.call.type = type,
        .function.name      = name,
    });
}

ast_t *ast_va_start(ast_t *ap) {
    return ast_copy(&(ast_t){
        .type  = AST_TYPE_VA_START,
        .ctype = ast_data_table[AST_DATA_VOID],
        .ap    = ap
    });
}

ast_t *ast_va_arg(data_type_t *type, ast_t *ap) {
    return ast_copy(&(ast_t){
        .type  = AST_TYPE_VA_ARG,
        .ctype = type,
        .ap    = ap
    });
}

ast_t *ast_declaration(ast_t *var, list_t *init) {
    return ast_copy(&(ast_t) {
        .type      = AST_TYPE_DECLARATION,
        .ctype     = NULL,
        .decl.var  = var,
        .decl.init = init,
    });
}

ast_t *ast_initializer(ast_t *value, data_type_t *to, int offset) {
    return ast_copy(&(ast_t){
        .type          = AST_TYPE_INITIALIZER,
        .init.value    = value,
        .init.offset   = offset,
        .init.type     = to
    });
}

ast_t *ast_ternary(data_type_t *type, ast_t *cond, ast_t *then, ast_t *last) {
    return ast_copy(&(ast_t){
        .type         = AST_TYPE_EXPRESSION_TERNARY,
        .ctype        = type,
        .ifstmt.cond  = cond,
        .ifstmt.then  = then,
        .ifstmt.last  = last
    });
}

static ast_t *ast_for_intermediate(int type, ast_t *init, ast_t *cond, ast_t *step, ast_t *body) {
    return ast_copy(&(ast_t){
        .type         = type,
        .ctype        = NULL,
        .forstmt.init = init,
        .forstmt.cond = cond,
        .forstmt.step = step,
        .forstmt.body = body
    });
}

ast_t *ast_switch(ast_t *expr, ast_t *body) {
    return ast_copy(&(ast_t){
        .type            = AST_TYPE_STATEMENT_SWITCH,
        .switchstmt.expr = expr,
        .switchstmt.body = body
    });
}

ast_t *ast_case(int begin, int end) {
    return ast_copy(&(ast_t){
        .type    = AST_TYPE_STATEMENT_CASE,
        .casebeg = begin,
        .caseend = end
    });
}

ast_t *ast_make(int type) {
    return ast_copy(&(ast_t){
        .type = type
    });
}

ast_t *ast_if(ast_t *cond, ast_t *then, ast_t *last) {
    return ast_copy(&(ast_t){
        .type        = AST_TYPE_STATEMENT_IF,
        .ctype       = NULL,
        .ifstmt.cond = cond,
        .ifstmt.then = then,
        .ifstmt.last = last
    });
}

ast_t *ast_for(ast_t *init, ast_t *cond, ast_t *step, ast_t *body) {
    return ast_for_intermediate(AST_TYPE_STATEMENT_FOR, init, cond, step, body);
}
ast_t *ast_while(ast_t *cond, ast_t *body) {
    return ast_for_intermediate(AST_TYPE_STATEMENT_WHILE, NULL, cond, NULL, body);
}
ast_t *ast_do(ast_t *cond, ast_t *body) {
    return ast_for_intermediate(AST_TYPE_STATEMENT_DO, NULL, cond, NULL, body);
}

ast_t *ast_goto(char *label) {
    return ast_copy(&(ast_t){
        .type           = AST_TYPE_STATEMENT_GOTO,
        .gotostmt.label = label,
        .gotostmt.where = NULL
    });
}

ast_t *ast_new_label(char *label) {
    return ast_copy(&(ast_t){
        .type           = AST_TYPE_STATEMENT_LABEL,
        .gotostmt.label = label,
        .gotostmt.where = NULL
    });
}

ast_t *ast_return(ast_t *value) {
    return ast_copy(&(ast_t){
        .type       = AST_TYPE_STATEMENT_RETURN,
        .returnstmt = value
    });
}

ast_t *ast_compound(list_t *statements) {
    return ast_copy(&(ast_t){
        .type     = AST_TYPE_STATEMENT_COMPOUND,
        .ctype    = NULL,
        .compound = statements
    });
}

data_type_t *ast_structure_new(table_t *fields, int size, bool isstruct) {
    return ast_type_copy(&(data_type_t) {
        .type     = TYPE_STRUCTURE,
        .size     = size,
        .fields   = fields,
        .isstruct = isstruct
    });
}

char *ast_label(void) {
    static int index = 0;
    string_t *string = string_create();
    string_catf(string, ".L%d", index++);
    return string_buffer(string);
}

ast_t *ast_label_address(char *label) {
    return ast_copy(&(ast_t){
        .type           = AST_TYPE_STATEMENT_LABEL_COMPUTED,
        .ctype          = ast_pointer(ast_data_table[AST_DATA_VOID]),
        .gotostmt.label = label
    });
}

ast_t *ast_goto_computed(ast_t *expression) {
    return ast_copy(&(ast_t){
        .type          = AST_TYPE_STATEMENT_GOTO_COMPUTED,
        .unary.operand = expression
    });
}

bool ast_type_isinteger(data_type_t *type) {
    switch (type->type) {
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_LLONG:
            return true;
        default:
            return false;
    }
}

bool ast_type_isfloating(data_type_t *type) {
    switch (type->type) {
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
        case TYPE_LDOUBLE:
            return true;
        default:
            return false;
    }
}

bool ast_type_isstring(data_type_t *type) {
    return type->type == TYPE_ARRAY && type->pointer->type == TYPE_CHAR;
}

data_type_t *ast_type_copy(data_type_t *type) {
    return memcpy(memory_allocate(sizeof(data_type_t)), type, sizeof(data_type_t));
}

data_type_t *ast_type_copy_incomplete(data_type_t *type) {
    if (!type)
        return NULL;
    return (type->length == -1)
                ? ast_type_copy(type)
                : type;
}

data_type_t *ast_type_create(type_t type, bool sign) {

    data_type_t *t = memory_allocate(sizeof(data_type_t));

    t->type = type;
    t->sign = sign;

    switch (type) {
        case TYPE_VOID:    t->size = 0;                      break;
        case TYPE_BOOL:    t->size = ARCH_TYPE_SIZE_INT;     break;
        case TYPE_CHAR:    t->size = ARCH_TYPE_SIZE_CHAR;    break;
        case TYPE_SHORT:   t->size = ARCH_TYPE_SIZE_SHORT;   break;
        case TYPE_INT:     t->size = ARCH_TYPE_SIZE_INT;     break;
        case TYPE_LONG:    t->size = ARCH_TYPE_SIZE_LONG;    break;
        case TYPE_LLONG:   t->size = ARCH_TYPE_SIZE_LLONG;   break;
        case TYPE_FLOAT:   t->size = ARCH_TYPE_SIZE_FLOAT;   break;
        case TYPE_DOUBLE:  t->size = ARCH_TYPE_SIZE_DOUBLE;  break;
        case TYPE_LDOUBLE: t->size = ARCH_TYPE_SIZE_LDOUBLE; break;
        default:
            compile_error("ICE");
    }

    return t;
}

data_type_t *ast_type_stub(void) {
    return ast_type_copy(&(data_type_t) {
        .type = TYPE_CDECL,
        .size = 0
    });
}

ast_t *ast_type_convert(data_type_t *type, ast_t *ast) {
    return ast_copy(&(ast_t){
        .type          = AST_TYPE_CONVERT,
        .ctype         = type,
        .unary.operand = ast
    });
}

data_type_t *ast_prototype(data_type_t *returntype, list_t *paramtypes, bool dots) {
    return ast_type_copy(&(data_type_t){
        .type       = TYPE_FUNCTION,
        .returntype = returntype,
        .parameters = paramtypes,
        .hasdots    = dots
    });
}

data_type_t *ast_array(data_type_t *type, int length) {
    return ast_type_copy(&(data_type_t){
        .type    = TYPE_ARRAY,
        .pointer = type,
        .size    = (length < 0) ? -1 : type->size * length,
        .length  = length
    });
}

data_type_t *ast_array_convert(data_type_t *type) {
    if (type->type != TYPE_ARRAY)
        return type;
    return ast_pointer(type->pointer);
}

ast_t *ast_designator_convert(ast_t *ast) {
    if (!ast)
        return NULL;
    if (ast->type == AST_TYPE_DESIGNATOR) {
        return ast_new_unary(
                    AST_TYPE_ADDRESS,
                    ast_pointer(ast->function.call.functionpointer->function.call.type),
                    ast->function.call.functionpointer
        );
    }

    return ast;
}

data_type_t *ast_pointer(data_type_t *type) {
    return ast_type_copy(&(data_type_t){
        .type    = TYPE_POINTER,
        .pointer = type,
        .size    = ARCH_TYPE_SIZE_POINTER
    });
}

const char *ast_type_string(data_type_t *type) {
    string_t *string;

    switch (type->type) {
        case TYPE_VOID:     return "void";
        case TYPE_BOOL:     return "_Bool";
        case TYPE_INT:      return "int";
        case TYPE_CHAR:     return "char";
        case TYPE_LONG:     return "long";
        case TYPE_LLONG:    return "long long";
        case TYPE_SHORT:    return "short";
        case TYPE_FLOAT:    return "float";
        case TYPE_DOUBLE:   return "double";
        case TYPE_LDOUBLE:  return "long double";

        case TYPE_FUNCTION:
            string = string_create();
            string_cat(string, '(');
            for (list_iterator_t *it = list_iterator(type->parameters); !list_iterator_end(it); ) {
                data_type_t *next = list_iterator_next(it);
                string_catf(string, "%s", ast_type_string(next));
                if (!list_iterator_end(it))
                    string_cat(string, ',');
            }
            string_catf(string, ") -> %s", ast_type_string(type->returntype));
            return string_buffer(string);

        case TYPE_POINTER:
            string = string_create();
            string_catf(string, "%s*", ast_type_string(type->pointer));
            return string_buffer(string);

        case TYPE_ARRAY:
            string = string_create();
            string_catf(
                string,
                "%s[%d]",
                ast_type_string(type->pointer),
                type->length
            );
            return string_buffer(string);

        case TYPE_STRUCTURE:
            string = string_create();
            string_catf(string, "(struct");
            for (list_iterator_t *it = list_iterator(table_values(type->fields)); !list_iterator_end(it); ) {
                data_type_t *ftype = list_iterator_next(it);
                if (ftype->bitfield.size < 0) {
                    string_catf(string, " (%s)", ast_type_string(ftype));
                } else {
                    string_catf(
                        string,
                        "(%s:%d:%d)",
                        ast_type_string(ftype),
                        ftype->bitfield.offset,
                        ftype->bitfield.offset + ftype->bitfield.size
                    );
                }
            }
            string_cat(string, ')');
            return string_buffer(string);

        default:
            break;
    }
    return NULL;
}

static void ast_string_unary(string_t *string, const char *op, ast_t *ast) {
    string_catf(string, "(%s %s)", op, ast_string(ast->unary.operand));
}

static void ast_string_binary(string_t *string, const char *op, ast_t *ast) {
    string_catf(string, "(%s %s %s)", op, ast_string(ast->left), ast_string(ast->right));
}

static void ast_string_initialization_declaration(string_t *string, list_t *initlist) {
    if (!initlist)
        return;

    for (list_iterator_t *it = list_iterator(initlist); !list_iterator_end(it); ) {
        ast_t *init = list_iterator_next(it);
        string_catf(string, "%s", ast_string(init));
        if (!list_iterator_end(it))
            string_cat(string, ' ');
    }
}

static void ast_string_impl(string_t *string, ast_t *ast) {
    char *left  = NULL;
    char *right = NULL;

    if (!ast) {
        string_catf(string, "(null)");
        return;
    }

    switch (ast->type) {
        case AST_TYPE_LITERAL:
            switch (ast->ctype->type) {
                case TYPE_INT:
                case TYPE_SHORT:
                    string_catf(string, "%d",   ast->integer);
                    break;

                case TYPE_FLOAT:
                case TYPE_DOUBLE:
                    string_catf(string, "%f",   ast->floating.value);
                    break;

                case TYPE_LONG:
                    string_catf(string, "%ldL", ast->integer);
                    break;

                case TYPE_CHAR:
                    if (ast->integer == '\n')
                        string_catf(string, "'\n'");
                    else if (ast->integer == '\\')
                        string_catf(string, "'\\\\'");
                    else if (ast->integer == '\0')
                        string_catf(string, "'\\0'");
                    else
                        string_catf(string, "'%c'", ast->integer);
                    break;

                default:
                    compile_ice("ast_string_impl");
                    break;
            }
            break;

        case AST_TYPE_STRING:
            string_catf(string, "\"%s\"", string_quote(ast->string.data));
            break;

        case AST_TYPE_VAR_LOCAL:
            string_catf(string, "%s", ast->variable.name);
            if (ast->variable.init) {
                string_cat(string, '(');
                ast_string_initialization_declaration(string, ast->variable.init);
                string_cat(string, ')');
            }
            break;

        case AST_TYPE_VAR_GLOBAL:
            string_catf(string, "%s", ast->variable.name);
            break;

        case AST_TYPE_CALL:
        case AST_TYPE_POINTERCALL:
            string_catf(string, "(%s)%s(", ast_type_string(ast->ctype),
                (ast->type == AST_TYPE_CALL)
                    ?ast->function.name
                    : ast_string(ast)
            );

            for (list_iterator_t *it = list_iterator(ast->function.call.args); !list_iterator_end(it); ) {
                string_catf(string, "%s", ast_string(list_iterator_next(it)));
                if (!list_iterator_end(it))
                    string_cat(string, ',');
            }
            string_cat(string, ')');
            break;

        case AST_TYPE_FUNCTION:
            string_catf(string, "(%s)%s(", ast_type_string(ast->ctype), ast->function.name);
            for (list_iterator_t *it = list_iterator(ast->function.params); !list_iterator_end(it); ) {
                ast_t *param = list_iterator_next(it);
                string_catf(string, "%s %s", ast_type_string(param->ctype), ast_string(param));
                if (!list_iterator_end(it))
                    string_cat(string, ',');
            }
            string_cat(string, ')');
            ast_string_impl(string, ast->function.body);
            break;

        case AST_TYPE_DECLARATION:
            string_catf(string, "(decl %s %s ",
                    ast_type_string(ast->decl.var->ctype),
                    ast->decl.var->variable.name
            );
            ast_string_initialization_declaration(string, ast->decl.init);
            string_cat(string, ')');
            break;

        case AST_TYPE_INITIALIZER:
            string_catf(string, "%s@%d", ast_string(ast->init.value), ast->init.offset);
            break;

        case AST_TYPE_CONVERT:
            string_catf(string, "(convert %s -> %s)", ast_string(ast->unary.operand), ast_type_string(ast->ctype));
            break;

        case AST_TYPE_STATEMENT_COMPOUND:
            string_cat(string, '{');
            for (list_iterator_t *it = list_iterator(ast->compound); !list_iterator_end(it); ) {
                ast_string_impl(string, list_iterator_next(it));
                string_cat(string, ';');
            }
            string_cat(string, '}');
            break;

        case AST_TYPE_STRUCT:
            ast_string_impl(string, ast->structure);
            string_cat(string, '.');
            string_catf(string, ast->field);
            break;

        case AST_TYPE_EXPRESSION_TERNARY:
            string_catf(string, "(? %s %s %s)",
                            ast_string(ast->ifstmt.cond),
                            ast_string(ast->ifstmt.then),
                            ast_string(ast->ifstmt.last)
            );
            break;

        case AST_TYPE_STATEMENT_IF:
            string_catf(string, "(if %s %s", ast_string(ast->ifstmt.cond), ast_string(ast->ifstmt.then));
            if (ast->ifstmt.last)
                string_catf(string, " %s", ast_string(ast->ifstmt.last));
            string_cat(string, ')');
            break;

        case AST_TYPE_STATEMENT_FOR:
            string_catf(string, "(for %s %s %s %s)",
                ast_string(ast->forstmt.init),
                ast_string(ast->forstmt.cond),
                ast_string(ast->forstmt.step),
                ast_string(ast->forstmt.body)
            );
            break;

        case AST_TYPE_STATEMENT_WHILE:
            string_catf(string, "(while %s %s)",
                ast_string(ast->forstmt.cond),
                ast_string(ast->forstmt.body)
            );
            break;

        case AST_TYPE_STATEMENT_DO:
            string_catf(string, "(do %s %s)",
                ast_string(ast->forstmt.cond),
                ast_string(ast->forstmt.body)
            );
            break;

        case AST_TYPE_STATEMENT_RETURN:
            string_catf(string, "(return %s)", ast_string(ast->returnstmt));
            break;

        case AST_TYPE_LRSHIFT:            ast_string_binary(string, ">>",    ast); break;
        case AST_TYPE_ADDRESS:            ast_string_unary (string, "addr",  ast); break;
        case AST_TYPE_DEREFERENCE:        ast_string_unary (string, "deref", ast); break;


        case LEXER_TOKEN_COMPOUND_LSHIFT: ast_string_binary(string, "<<=",     ast); break;
        case LEXER_TOKEN_COMPOUND_RSHIFT: ast_string_binary(string, ">>=",     ast); break;
        case AST_TYPE_POST_INCREMENT:     ast_string_unary (string, "postinc", ast); break;
        case AST_TYPE_POST_DECREMENT:     ast_string_unary (string, "postdec", ast); break;
        case AST_TYPE_PRE_INCREMENT:      ast_string_unary (string, "preinc",  ast); break;
        case AST_TYPE_PRE_DECREMENT:      ast_string_unary (string, "predec",  ast); break;
        case AST_TYPE_NEGATE:             ast_string_unary (string, "negate",  ast); break;
        case '!':                         ast_string_unary (string, "bitnot",  ast); break;
        case '&':                         ast_string_binary(string, "bitand",  ast); break;
        case '|':                         ast_string_binary(string, "bitor",   ast); break;
        case AST_TYPE_AND:                ast_string_binary(string, "logand",  ast); break;
        case AST_TYPE_OR:                 ast_string_binary(string, "logor",   ast); break;
        case AST_TYPE_GEQUAL:             ast_string_binary(string, "gteq",    ast); break;
        case AST_TYPE_LEQUAL:             ast_string_binary(string, "lteq",    ast); break;
        case AST_TYPE_NEQUAL:             ast_string_binary(string, "ne",      ast); break;
        case AST_TYPE_LSHIFT:             ast_string_binary(string, "lshift",  ast); break;
        case AST_TYPE_RSHIFT:             ast_string_binary(string, "rshift",  ast); break;

        case AST_TYPE_DESIGNATOR:
            string_catf(string, "(designator %s)", ast_string(ast->function.call.functionpointer));
            break;

        case AST_TYPE_EXPRESSION_CAST:
            string_catf(string, "((%s) -> (%s) %s)",
                ast_type_string(ast->unary.operand->ctype),
                ast_type_string(ast->ctype),
                ast_string(ast->unary.operand)
            );
            break;

        case AST_TYPE_STATEMENT_LABEL_COMPUTED:
            string_catf(string, "(labeladdr %s)", ast->gotostmt.label);
            break;

        default:
            if (!ast->left || !ast->right)
                break;

            left  = ast_string(ast->left);
            right = ast_string(ast->right);
            if (ast->type == LEXER_TOKEN_EQUAL)
                string_catf(string, "(== %s %s)", left, right);
            else
                string_catf(string, "(%c %s %s)", ast->type, left, right);
    }
}

char *ast_string(ast_t *ast) {
    string_t *string = string_create();
    ast_string_impl(string, ast);
    return string_buffer(string);
}
