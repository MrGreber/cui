#pragma once

#ifndef STR_H
#define STR_H
#include <defines.h>

// Todo: currently just utf-8, NO UNICODE!!!!
#if !UNICODE
#define char_t wchar
#define _C_ L
#else
#define char_t char
#define _C_
#endif

typedef struct string {
    char_t* data;
    u64 length;
    u64 capacity;
} str_t;

#define String(func) __string_##func
str_t* String(new)(char_t* data, u64 length);
bool String(set)(str_t* dst, char_t* src, const u64 length);
void String(del)(str_t* src);
bool String(pop_char)(str_t* src, const u64 index);
bool String(del_sub)(str_t* src, const u64 start, const u64 end);
str_t* String(get_sub)(str_t* src, const u64 start, const u64 end);
bool String(push_char)(str_t* src, const char_t c);
bool String(insert_char)(str_t* src, const u64 index, const char_t c);
bool String(concat)(str_t* dst, str_t* src);
u64 String(find_char)(str_t* src, const u64 start, const char_t c);
u64 String(rfind_char)(str_t* src, const u64 start, const char_t c);

void String(print)(const str_t* src, const bool new_line);

#endif //STR_H
