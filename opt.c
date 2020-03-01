#include "opt.h"

static const opt_extension_t opt_extension_matrix[] = {
    /*
     * same as GNUC but no dollar identifiers, they don't make much sense
     * and in C11 mode, opposed to GNUC which is C99
     */
    [STANDARD_LICEC] = EXTENSION_TYPEOF          | EXTENSION_OMITOPCOND      |
                       EXTENSION_STATEMENTEXPRS  | EXTENSION_NOMEMBERSTRUCT  |
                       EXTENSION_CASERANGES      | EXTENSION_ESCCHAR         |
                       EXTENSION_INCOMPLETEENUM  | EXTENSION_BINARYCONSTANTS |
                       EXTENSION_ARITHMETICVOID  | EXTENSION_LABELASVALUES   |
                       EXTENSION_ZEROARRAYS      | EXTENSION_ZEROARRAYS      |
                       EXTENSION_NONCONSTINIT    | EXTENSION_NONSTANDARD,

    /*
     * standard GNUC format, TODO: -std=gnuc90, which is the same as this
     * but C90, instead of C90
     */
    [STANDARD_GNUC]  = EXTENSION_TYPEOF          | EXTENSION_OMITOPCOND      |
                       EXTENSION_STATEMENTEXPRS  | EXTENSION_NOMEMBERSTRUCT  |
                       EXTENSION_CASERANGES      | EXTENSION_ESCCHAR         |
                       EXTENSION_INCOMPLETEENUM  | EXTENSION_BINARYCONSTANTS |
                       EXTENSION_ARITHMETICVOID  | EXTENSION_LABELASVALUES   |
                       EXTENSION_ZEROARRAYS      | EXTENSION_ZEROARRAYS      |
                       EXTENSION_NONCONSTINIT    | EXTENSION_DOLLAR          |
                       EXTENSION_NONSTANDARD,

    [STANDARD_C90]   = 0,
    [STANDARD_C99]   = 0,
    [STANDARD_C11]   = 0
};

static opt_std_t       standard   = STANDARD_LICEC;
static opt_extension_t extensions = ~0;

bool opt_std_test(opt_std_t std) {
    return (standard == std);
}

bool opt_extension_test(opt_extension_t ext) {
    return (extensions & ext);
}

void opt_std_set(opt_std_t std) {
    standard   = std;
    extensions = opt_extension_matrix[std];
}

void opt_extension_set(opt_extension_t ext) {
    if (opt_extension_matrix[standard] & ext)
        extensions &= ext;
}
