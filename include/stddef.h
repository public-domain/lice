#ifndef __STDDEF_HDR
#define __STDDEF_HDR

#define NULL ((void*)0)

typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef char wchar_t;
typedef long double max_align_t;

#define offsetof(TYPE, MEMBER) ((size_t)&(((TYPE*)NULL)->MEMBER))

#endif
