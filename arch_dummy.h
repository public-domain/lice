#ifndef LICE_ARCH_DUMMY_HDR
/*
 * File: arch_dummy.h
 *  Stubded dummy architecture useful as a template for retargeting
 *  LICE.
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
#define ARCH_TYPE_SIZE_CHAR      -1
#define ARCH_TYPE_SIZE_LONG      -1
#define ARCH_TYPE_SIZE_LLONG     -1
#define ARCH_TYPE_SIZE_INT       -1
#define ARCH_TYPE_SIZE_SHORT     -1
#define ARCH_TYPE_SIZE_FLOAT     -1
#define ARCH_TYPE_SIZE_DOUBLE    -1
#define ARCH_TYPE_SIZE_LDOUBLE   -1
#define ARCH_TYPE_SIZE_POINTER   -1

/*
 * Macro: ARCH_ALIGNMENT
 *  The default alignment of structure elements (padding) for the given
 *  architecture / ABI
 */
#define ARCH_ALIGNMENT           -1
#endif
