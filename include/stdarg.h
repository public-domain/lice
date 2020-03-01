#ifndef __STDARG_HDR
#define __STDARG_HDR

typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void        *overflow_arg_area;
    void        *reg_save_area;
} va_list[1];

#define va_start(AP, LAST) __builtin_va_start(AP)
#define va_arg(AP, TYPE)   __builtin_va_arg(AP, TYPE)
#define va_end(AP)         1
#define va_copy(DEST, SRC) ((DEST)[0] = (SRC)[0])


#define __GNUC_VA_LIST 1
typedef va_list __gnuc_va_list; // deal with gnuc headers

#endif
