#ifndef LICE_OPT_HDR
#define LICE_OPT_HDR

#include <stdbool.h>

typedef enum {
    STANDARD_LICEC, /* LICE variant (C11 with extensions) */
    STANDARD_GNUC,  /* GNUC variant                       */
    STANDARD_KANDR, /* K&R C                              */
    STANDARD_C90,   /* C90 (ISO/IEC 9899:1990)            */
    STANDARD_C99,   /* C99 (ISO/IEC 9899:1999)            */
    STANDARD_C11    /* C11 (ISO/IEC 9889:2011)            */
} opt_std_t;

typedef enum {
    EXTENSION_DOLLAR          = 1 << 1,  /* Dollar signs in Identifier Names                  */
    EXTENSION_TYPEOF          = 1 << 2,  /* Referring to a Type with typeof                   */
    EXTENSION_OMITOPCOND      = 1 << 3,  /* Conditionals with Omitted Operands                */
    EXTENSION_STATEMENTEXPRS  = 1 << 4,  /* Statements and Declarations in Expressions        */
    EXTENSION_NOMEMBERSTRUCT  = 1 << 5,  /* Structures with No Members                        */
    EXTENSION_NONCONSTINIT    = 1 << 6,  /* Non-Constant Initializers                         */
    EXTENSION_CASERANGES      = 1 << 7,  /* Case Ranges                                       */
    EXTENSION_ESCCHAR         = 1 << 8,  /* The Character <ESC> in Constants                  */
    EXTENSION_INCOMPLETEENUM  = 1 << 9,  /* Incomplete enum Types                             */
    EXTENSION_BINARYCONSTANTS = 1 << 10, /* Binary constants using the '0b' prefix            */
    EXTENSION_ARITHMETICVOID  = 1 << 11, /* Arithmetic on void- and Function-Pointers         */
    EXTENSION_LABELASVALUES   = 1 << 12, /* Labels as Values                                  */
    EXTENSION_ZEROARRAYS      = 1 << 13, /* Arrays of Length Zero                             */

    /* always the last in the list */
    EXTENSION_NONSTANDARD     = 1 << 14
} opt_extension_t;

bool opt_std_test(opt_std_t std);
bool opt_extension_test(opt_extension_t ext);
void opt_std_set(opt_std_t std);
void opt_extension_set(opt_extension_t ext);

#endif
