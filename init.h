#ifndef LICE_INIT_HDR
#define LICE_INIT_HDR
#include "ast.h"
#include "util.h"

/*
 * Function: init_entry
 *  The core entry point to initializer parsing.
 *
 * Parameters:
 *  type - Base type of the current initializer
 *
 * Returns:
 *  A list containing all the initialization nodes for the initializer
 *
 * Remarks:
 *  Deals with all forms of initialization, lists, aggregates, including
 *  designated versions for user defined unions, structures, arrays and
 *  enumerations.
 *
 *  Will raise compiler error if syntax or lexical error in initializer
 *  resulting in a NULL, or partially filled list of ast initializer
 *  nodes.
 */
list_t *init_entry(data_type_t *type);

#endif
