/*
 * Deals with all the complexity in C's declaration specification with
 * a rather large state machine model. C has a lot of ways to specify
 * something, that happens to be equivlant to other meanings, which are
 * also used. This state machine monitors the occurance of certain
 * identifiers to build a serise of on/off state which ultimatly
 * allows us to disambiguate the meaning, while at the same time enforcing
 * correctness.
 *
 * For instance it isn't legal in C to have a typedef of a 'signed' size
 * specified type, than use that typedef with another size specifier.
 * More of these rules apply as well, and are documented in the state
 * machine set logic.
 *
 * Once the state machine has completed it's work the get function uses
 * the state of the machine to determine what type to return from the
 * ast data table for types, or if there needs to be a new type created
 * to compensate for the declaration. Similarly at this stage the state
 * can be invalid (if something wen terribly wrong) and we can handle,
 * or ice.
 *
 * The main entry point is decl_spec and it's called from the parser,
 * if everything passes the callsite gets a data_type_t of the type
 * specified.
 */
#include <string.h>

#include "parse.h"
#include "lice.h"
#include "lexer.h"

typedef enum {
    SPEC_TYPE_NULL,
    SPEC_TYPE_VOID,
    SPEC_TYPE_BOOL,
    SPEC_TYPE_CHAR,
    SPEC_TYPE_INT,
    SPEC_TYPE_FLOAT,
    SPEC_TYPE_DOUBLE,
} spec_type_t;

typedef enum {
    SPEC_SIZE_NULL,
    SPEC_SIZE_SHORT,
    SPEC_SIZE_LONG,
    SPEC_SIZE_LLONG
} spec_size_t;

typedef enum {
    SPEC_SIGN_NULL,
    SPEC_SIGN_SIGNED,
    SPEC_SIGN_UNSIGNED
} spec_sign_t;

static const char *spec_type_string[] = {
    "null", "void",  "_Bool", "char",
    "int",  "float", "double"
};

static const char *spec_size_string[] = {
    "null", "short", "long", "long long"
};

static const char *spec_sign_string[] = {
    "null", "signed", "unsigned"
};

static const char *spec_var_string[] = {
    "null", "type", "size", "sign", "user"
};

typedef struct {
    storage_t    class;
    spec_type_t  type;
    spec_size_t  size;
    spec_sign_t  sign;
    data_type_t *user;
    bool         kconst;
    bool         kvolatile;
    bool         kinline;
} decl_spec_t;

typedef enum {
    SPEC_VAR_NULL,
    SPEC_VAR_TYPE,
    SPEC_VAR_SIZE,
    SPEC_VAR_SIGN,
    SPEC_VAR_USER
} decl_var_t;

#define decl_spec_error(X, SELECT) \
    decl_spec_error_impl((X), (SELECT), __LINE__)

static const char *debug_storage_string(const storage_t class) {
    switch (class) {
        case STORAGE_AUTO:      return "auto";
        case STORAGE_EXTERN:    return "extern";
        case STORAGE_REGISTER:  return "register";
        case STORAGE_STATIC:    return "static";
        case STORAGE_TYPEDEF:   return "typedef";
    }
    return "default";
}

static void decl_spec_error_impl(const decl_spec_t *spec, const decl_var_t select, const size_t line) {
    const char *type = spec_type_string[spec->type];
    const char *size = spec_size_string[spec->size];
    const char *sign = spec_sign_string[spec->sign];
    const char *var  = spec_var_string[select];

    if (!type) type = "unspecified";
    if (!size) size = "unspecified";
    if (!sign) sign = "unspecified";
    if (!var)  var  = "unspecified";

    compile_ice("declaration specifier error %d\n"
                "debug info:\n"
                "   select:   %s\n"
                "   class:    %s\n"
                "   type:     %s\n"
                "   size:     %s\n"
                "   sign:     %s\n"
                "   const:    %s\n"
                "   volatile: %s\n"
                "   inline:   %s\n",
                line,
                var,
                debug_storage_string(spec->class),
                type,
                size,
                sign,
                bool_string(spec->kconst),
                bool_string(spec->kvolatile),
                bool_string(spec->kinline)
    );
}

static void decl_spec_class(decl_spec_t *spec, const storage_t class) {
    if (spec->class != 0)
        decl_spec_error(spec, SPEC_VAR_NULL);
    spec->class = class;
}

static void decl_spec_set(decl_spec_t *spec, const decl_var_t select, void *value) {
    switch (select) {
        case SPEC_VAR_SIGN:
            if (spec->sign != SPEC_SIGN_NULL)
                decl_spec_error(spec, select);
            spec->sign = *(spec_sign_t*)value;
            break;
        case SPEC_VAR_SIZE:
            if (spec->size != SPEC_SIZE_NULL)
                decl_spec_error(spec, select);
            spec->size = *(spec_size_t*)value;
            break;
        case SPEC_VAR_TYPE:
            if (spec->type != SPEC_TYPE_NULL)
                decl_spec_error(spec, select);
            spec->type = *(spec_type_t*)value;
            break;
        case SPEC_VAR_USER:
            if (spec->user != 0)
                decl_spec_error(spec, select);
            spec->user = value;
            break;
        default:
            compile_ice("decl_spec_get state machine got null variable reference");
            break;
    }

    /* bool cannot have a sign, it's only legal as it's own entity. */
    if (spec->type == SPEC_TYPE_BOOL && (spec->size != SPEC_SIZE_NULL && spec->sign != SPEC_SIGN_NULL))
        decl_spec_error(spec, select);

    switch (spec->size) {
        case SPEC_SIZE_SHORT:
            /*
             * short and short int are the only legal uses of the short
             * size specifier.
             */
            if (spec->type != SPEC_TYPE_NULL && spec->type != SPEC_TYPE_INT)
                decl_spec_error(spec, select);
            break;

        case SPEC_SIZE_LONG:
            /*
             * long, long int and long double are the only legal uses of
             * long size specifier.
             */
            if (spec->type != SPEC_TYPE_NULL && spec->type != SPEC_TYPE_INT && spec->type != SPEC_TYPE_DOUBLE)
                decl_spec_error(spec, select);
            break;

        default:
            break;
    }

    /*
     * sign and unsigned sign specifiers are not legal on void, float and
     * double types.
     */
    if (spec->sign != SPEC_SIGN_NULL) {
        switch (spec->type) {
            case SPEC_TYPE_VOID:
            case SPEC_TYPE_FLOAT:
            case SPEC_TYPE_DOUBLE:
                decl_spec_error(spec, select);
                break;
            default:
                break;
        }
    }

    /*
     * user types cannot have additional levels of specification on it,
     * for instance 'typedef unsigned int foo; 'signed foo'.
     */
    if (spec->user && (spec->type != SPEC_TYPE_NULL ||
                       spec->size != SPEC_SIZE_NULL ||
                       spec->sign != SPEC_SIGN_NULL))
        decl_spec_error(spec, select);
}

#define decl_spec_seti(SPEC, SELECT, VAR) \
    decl_spec_set((SPEC), (SELECT), &(int){ VAR })

static data_type_t *decl_spec_get(const decl_spec_t *spec) {
    bool sign = !!(spec->sign != SPEC_SIGN_UNSIGNED);

    switch (spec->type) {
        case SPEC_TYPE_VOID:
            return ast_data_table[AST_DATA_VOID];
        case SPEC_TYPE_BOOL:
            return ast_type_create(TYPE_BOOL, false);
        case SPEC_TYPE_CHAR:
            return ast_type_create(TYPE_CHAR, sign);
        case SPEC_TYPE_FLOAT:
            return ast_type_create(TYPE_FLOAT, false);
        case SPEC_TYPE_DOUBLE:
            if (spec->size == SPEC_SIZE_LONG)
                return ast_type_create(TYPE_LDOUBLE, false);
            return ast_type_create(TYPE_DOUBLE, false);
        default:
            break;
    }

    switch (spec->size) {
        case SPEC_SIZE_SHORT:
            return ast_type_create(TYPE_SHORT, sign);
        case SPEC_SIZE_LONG:
            return ast_type_create(TYPE_LONG, sign);
        case SPEC_SIZE_LLONG:
            return ast_type_create(TYPE_LLONG, sign);
        default:
            /* implicit int */
            return ast_type_create(TYPE_INT, sign);
    }
    compile_ice("declaration specifier");
}

data_type_t *decl_spec(storage_t *const class) {
    decl_spec_t spec;
    memset(&spec, 0, sizeof(spec));

    for (;;) {
        lexer_token_t *token = lexer_next();
        if (!token)
            compile_error("type specification with unexpected ending");

        if (token->type != LEXER_TOKEN_IDENTIFIER) {
            lexer_unget(token);
            break;
        }

        if (!strcmp(token->string, "const"))
            spec.kconst = true;
        else if (!strcmp(token->string, "volatile"))
            spec.kvolatile = true;
        else if (!strcmp(token->string, "inline"))
            spec.kinline = true;
        else if (!strcmp(token->string, "typedef"))
            decl_spec_class(&spec, STORAGE_TYPEDEF);
        else if (!strcmp(token->string, "extern"))
            decl_spec_class(&spec, STORAGE_EXTERN);
        else if (!strcmp(token->string, "static") || !strcmp(token->string, "__static__"))
            decl_spec_class(&spec, STORAGE_STATIC);
        else if (!strcmp(token->string, "auto"))
            decl_spec_class(&spec, STORAGE_AUTO);
        else if (!strcmp(token->string, "register"))
            decl_spec_class(&spec, STORAGE_REGISTER);
        else if (!strcmp(token->string, "void"))
            decl_spec_seti(&spec, SPEC_VAR_TYPE, SPEC_TYPE_VOID);
        else if (!strcmp(token->string, "_Bool"))
            decl_spec_seti(&spec, SPEC_VAR_TYPE, SPEC_TYPE_BOOL);
        else if (!strcmp(token->string, "char"))
            decl_spec_seti(&spec, SPEC_VAR_TYPE, SPEC_TYPE_CHAR);
        else if (!strcmp(token->string, "int"))
            decl_spec_seti(&spec, SPEC_VAR_TYPE, SPEC_TYPE_INT);
        else if (!strcmp(token->string, "float"))
            decl_spec_seti(&spec, SPEC_VAR_TYPE, SPEC_TYPE_FLOAT);
        else if (!strcmp(token->string, "double"))
            decl_spec_seti(&spec, SPEC_VAR_TYPE, SPEC_TYPE_DOUBLE);
        else if (!strcmp(token->string, "signed"))
            decl_spec_seti(&spec, SPEC_VAR_SIGN, SPEC_SIGN_SIGNED);
        else if (!strcmp(token->string, "unsigned"))
            decl_spec_seti(&spec, SPEC_VAR_SIGN, SPEC_SIGN_UNSIGNED);
        else if (!strcmp(token->string, "struct"))
            decl_spec_set(&spec, SPEC_VAR_USER, parse_structure());
        else if (!strcmp(token->string, "union"))
            decl_spec_set(&spec, SPEC_VAR_USER, parse_union());
        else if (!strcmp(token->string, "enum"))
            decl_spec_set(&spec, SPEC_VAR_USER, parse_enumeration());
        else if (!strcmp(token->string, "short"))
            decl_spec_seti(&spec, SPEC_VAR_SIZE, SPEC_SIZE_SHORT);
        else if (!strcmp(token->string, "long")) {
            if (spec.size == 0)
                decl_spec_seti(&spec, SPEC_VAR_SIZE, SPEC_SIZE_LONG);
            else if (spec.size == SPEC_SIZE_LONG)
                spec.size = SPEC_SIZE_LLONG;
            else
                decl_spec_error(&spec, SPEC_VAR_NULL);
        }
        else if (!strcmp(token->string, "typeof") || !strcmp(token->string, "__typeof__"))
            decl_spec_set(&spec, SPEC_VAR_USER, parse_typeof());
        else if (parse_typedef_find(token->string) && !spec.user)
            decl_spec_set(&spec, SPEC_VAR_USER, parse_typedef_find(token->string));
        else {
            lexer_unget(token);
            break;
        }
    }

    if (class)
        *class = spec.class;
    if (spec.user)
        return spec.user;

    return decl_spec_get(&spec);
}
