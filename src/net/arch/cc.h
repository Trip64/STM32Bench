/* lwIP Architecture / Compiler configuration for ARM GCC (Cortex-M7) */

#ifndef ARCH_CC_H
#define ARCH_CC_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define BYTE_ORDER  LITTLE_ENDIAN

/* Basic Types */
typedef uint8_t     u8_t;
typedef int8_t      s8_t;
typedef uint16_t    u16_t;
typedef int16_t     s16_t;
typedef uint32_t    u32_t;
typedef int32_t     s32_t;
typedef uintptr_t   mem_ptr_t;

/* Format specifiers */
#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "lu"
#define S32_F "ld"
#define X32_F "lx"
#define SZT_F "lu"

/* Struct packing */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

/* Platform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x)   do { printf x; } while(0)
#define LWIP_PLATFORM_ASSERT(x) do { printf("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__); } while(0)

#endif /* ARCH_CC_H */
