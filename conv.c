/*
 * The complicated C rule set for type conversion. This is a full research
 * oriented approach, run against the standard, and the tons of trial and
 * error.
 *
 * A little bit about what is involed in type conversion:
 *  - Arithmetic type rules
 *  - Implicit conversion
 *  - Explicit conversion
 *
 * 1. Arithmetic type rules:
 *      The C standard defines a set of rules about arithmetic type
 *      conversion, known as the conversion rank rules, which
 *      essentially dictate which sides of an expression need to be
 *      converted.
 *
 *      First rule:
 *      If the left hand side of an expression isn't an arithmetic type
 *      or the right hand side of an expression isn't an arithmetic type
 *      no conversion takes place.
 *
 *      Second rule:
 *      If the conversion rank of the left hand side expression type
 *      is less than the conversion rank of the right hand side
 *      expression type, then the left hand side of that expressions type
 *      gets converted to the right hands type.
 *
 *      Third rule:
 *      If the conversion rank of the left hand expression type doesn't
 *      compare equal to the right hands type, then the right hand side of
 *      that expressions type gets converted to the left hands type.
 *
 *      Last rule:
 *      If none of the above applies, then nothing is subjected to conversion,
 *      and doesn't need to be converted, unless the following:
 *
 *          The binary expression in which each operand is associated with happens
 *          to be of a relational one in which case the type is converted to
 *          integer type.
 *
 *          The expression happens to be of array type, in which case the array
 *          decays to a pointer of it's base type.
 *
 *  2. Implicit conversion
 *      Implicit type conversion takes place in two senarios, 1, when
 *      conversion ranking is involved (promoted types), or when the
 *      subject of a shift operation where the larger types is always
 *      assumed to satisfy the shift operation.
 *
 *  3. Explicit conversion
 *      The type which is assumed in explicit conversion (casting) is
 *      the type in which the operand is converted to, unless the conversion
 *      isn't legal (vector -> scalar for instance)
 */
#include "ast.h"
#include "lice.h"

bool conv_capable(data_type_t *type) {
    return ast_type_isinteger(type) || ast_type_isfloating(type);
}

int conv_rank(data_type_t *type) {
    if (!conv_capable(type))
        goto error;

    switch (type->type) {
        case TYPE_BOOL:    return 0;
        case TYPE_CHAR:    return 1;
        case TYPE_SHORT:   return 2;
        case TYPE_INT:     return 3;
        case TYPE_LONG:    return 4;
        case TYPE_LLONG:   return 5;
        case TYPE_FLOAT:   return 6;
        case TYPE_DOUBLE:  return 7;
        case TYPE_LDOUBLE: return 8;
        default:
            goto error;
    }

error:
    compile_ice("conv_rank");
}

data_type_t *conv_senority(data_type_t *lhs, data_type_t *rhs) {
    return conv_rank(lhs) < conv_rank(rhs) ? rhs : lhs;
}

ast_t *conv_usual(int operation, ast_t *left, ast_t *right) {
    if (!conv_capable(left->ctype) || !conv_capable(right->ctype)) {
        data_type_t *result;
        switch (operation) {
            case AST_TYPE_LEQUAL:
            case AST_TYPE_GEQUAL:
            case AST_TYPE_EQUAL:
            case AST_TYPE_NEQUAL:
            case '<':
            case '>':
                result = ast_data_table[AST_DATA_INT];
                break;
            default:
                result = ast_array_convert(left->ctype);
                break;
        }

        return ast_new_binary(result, operation, left, right);
    }

    int lrank = conv_rank(left->ctype);
    int rrank = conv_rank(right->ctype);

    if (lrank < rrank)
        left  = ast_type_convert(right->ctype, left);
    else if (lrank != rrank)
        right = ast_type_convert(left->ctype, right);

    data_type_t *result = ast_result_type(operation, left->ctype);
    return ast_new_binary(result, operation, left, right);
}
