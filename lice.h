#ifndef LICE_HDR
#define LICE_HDR
#include "util.h"

#ifdef LICE_TARGET_AMD64
#   include "arch_amd64.h"
#else
    /*
     * Any additional future targets will just keep bracing with
     * conditional inclusion here.
     */
#   include "arch_dummy.h"
#endif

#ifdef __GNUC__
#   define NORETURN __attribute__((noreturn))
#else
#   define NORETURN
#endif

/*
 * Function: compile_error
 *  Write compiler error diagnostic to stderr, formatted
 *
 * Parameters:
 *  fmt - Standard format specification string
 *  ... - Additional variable arguments
 *
 * Remarks:
 *  This function does not return, it kills execution via a call to
 *  exit(1);
 */
void NORETURN compile_error(const char *fmt, ...);

/*
 * Function: compile_warn
 *  Write compiler warning diagnostic to stderr, formatted
 *
 * Parameters:
 *  fmt - Standard format specification string
 *  ... - Additional variable arguments
 */
void compile_warn(const char *fmt, ...);

/*
 * Function: compile_ice
 *  Write an internal compiler error diagnostic to stderr, formatted
 *  and abort.
 *
 * Parameters:
 *  fmt - Standard format specification string
 *  ... - Additional variable arguments
 *
 * Remarks:
 *  Thie function does not return, it aborts execution via a call to
 *  abort()
 */
void NORETURN compile_ice(const char *fmt, ...);

/* TODO: eliminate */
extern bool compile_warning;

#endif
