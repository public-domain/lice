#ifndef LICE_ARCH_AMD64_HDR
/*
 * File: arch_amd64.h
 *  Isolates AMD64 / SystemV ABI specific details that are used in
 *  a variety of places of the compiler to target AMD64.
 */

/*
 * Constants: Native type sizes
 *
 *  The following are macros which describe the sizes of various native
 *  data types, they should reflect their true sizes on the given
 *  architecture unless mentioned otherwise by a specific ABI.
 *
 *  ARCH_TYPE_SIZE_CHAR     - Size of a char
 *  ARCH_TYPE_SIZE_LONG     - Size of a long
 *  ARCH_TYPE_SIZE_LLONG    - Size of a long long
 *  ARCH_TYPE_SIZE_INT      - Size of a int
 *  ARCH_TYPE_SIZE_SHORT    - Size of a short
 *  ARCH_TYPE_SIZE_FLOAT    - Size of a float
 *  ARCH_TYPE_SIZE_DOUBLE   - Size of a double
 *  ARCH_TYPE_SIZE_LDOUBLE  - Size of a long double
 *  ARCH_TYPE_SIZE_POINTER  - Size of a pointer
 */
#define ARCH_TYPE_SIZE_CHAR      1
#define ARCH_TYPE_SIZE_LONG      8
#define ARCH_TYPE_SIZE_LLONG     8
#define ARCH_TYPE_SIZE_INT       4
#define ARCH_TYPE_SIZE_SHORT     2
#define ARCH_TYPE_SIZE_FLOAT     4
#define ARCH_TYPE_SIZE_DOUBLE    8
#define ARCH_TYPE_SIZE_LDOUBLE   8
#define ARCH_TYPE_SIZE_POINTER   8

/*
 * Macro: ARCH_ALIGNMENT
 *  The default alignment of structure elements (padding) for the given
 *  architecture / ABI
 */
#define ARCH_ALIGNMENT           16

#endif
