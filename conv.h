#ifndef LICE_CONV_HDR
#define LICE_CONV_HDR

/*
 * File: conv.h
 *  Implements the interface to LICE's type conversion.
 */
#include "ast.h"

/*
 * Function: conv_capable
 *  Determines if type conversion is capable for a given type.
 *
 * Parameters:
 *  type    - The type to test for conversion capability
 *
 * Returns:
 *  `true` if conversion is capable, `false` otherwise.
 */
bool conv_capable(data_type_t *type);

/*
 * Function: conv_senority
 *  Determines the senority of two types in question, returning the
 *  one which out ranks the other. This is usually used in binary
 *  operations, thus the naming of lhs and rhs are used.
 *
 * Parameters:
 *  lhs     - Left hand side type
 *  rhs     - Right hand side type
 *
 * Returns:
 *  `lhs` if outranks `rhs`, otherwise `rhs`
 */
data_type_t *conv_senority(data_type_t *lhs, data_type_t *rhs);

/*
 * Function: conv_rank
 *  Determines the conversion ranking of a given data type.
 *
 * Parameters:
 *  type    - The type to get the conversion ranking of.
 *
 * Returns:
 *  An integer value of the conversion ranking of the given data type,
 *  which can be used in the process of typicla relational, or comparitive
 *  checks.
 */
int conv_rank(data_type_t *type);

/*
 * Function: conv_usual
 *  Given a binary operation and the two operands, this will perform
 *  usualy type conversion and return an ast node that signifies that
 *  operation (including the conversion).
 *
 * Parameters:
 *  operation   - An ast type, or character literal for typical binary
 *                operations, like '+'.
 *  left        - The left hand side of the expression.
 *  right       - The right hand side of the expression.
 */
ast_t *conv_usual(int operation, ast_t *left, ast_t *right);

#endif
