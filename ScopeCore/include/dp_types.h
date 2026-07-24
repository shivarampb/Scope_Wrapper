// dp_types.h
//
// Fundamental fixed-width type aliases shared across ScopeCore and all scope
// plugins. Mirrors the primitive typedef layer used by the ELoad_R2 reference
// (U32BIT / FDOUBLE / S8BIT) so the two instrument families share conventions.

#ifndef DP_TYPES_H
#define DP_TYPES_H

#include <cstdint>

typedef uint8_t   U8BIT;
typedef int8_t    S8BIT;
typedef uint16_t  U16BIT;
typedef int16_t   S16BIT;
typedef uint32_t  U32BIT;
typedef int32_t   S32BIT;
typedef uint64_t  U64BIT;
typedef int64_t   S64BIT;
typedef float     FSINGLE;
typedef double    FDOUBLE;

#endif // DP_TYPES_H
