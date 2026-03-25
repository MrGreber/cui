#pragma once

#ifndef H_DEFINES
#define H_DEFINES

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Alias for wide character type
 */
#define wchar wchar_t

/**
 * @brief Alias for unsigned 8-bit integer
 */
#define byte uint8_t

/**
 * @brief Alias for unsigned 8-bit integer
 */
#define u8 uint8_t

/**
 * @brief Alias for signed 8-bit integer
 */
#define i8 int8_t

/**
 * @brief Alias for unsigned 16-bit integer
 */
#define u16 uint16_t

/**
 * @brief Alias for signed 16-bit integer
 */
#define i16 int16_t

/**
 * @brief Alias for unsigned 32-bit integer
 */
#define u32 uint32_t

/**
 * @brief Alias for signed 32-bit integer
 */
#define i32 int32_t

/**
 * @brief Alias for unsigned 64-bit integer
 */
#define u64 uint64_t

/**
 * @brief Alias for signed 64-bit integer
 */
#define i64 int64_t

/**
 * @brief Alias for 32-bit floating point
 */
#define f32 float

/**
 * @brief Alias for 64-bit floating point
 */
#define f64 double

#define uptr uintptr_t
#define iptr intptr_t

#define private(func) __private_##func
#endif // H_DEFINES
