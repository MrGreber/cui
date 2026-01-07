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

str_t* new_str(char_t* data, u64 length);
void del_str(str_t* src);
bool pop_char(str_t* src, const u64 index);
bool del_substr(str_t* src, const u64 start, const u64 end);
str_t* get_substr(str_t* src, const u64 start, const u64 end);
bool push_char(str_t* src, const char_t c);
bool insert_char(str_t* src, const u64 index, const char_t c);
bool concat_str(str_t* dst, str_t* src);
u64 find_char(str_t* src, const u64 start, const char_t c);
u64 rfind_char(str_t* src, const u64 start, const char_t c);


void prints(const str_t* src, const bool new_line);


#endif //STR_H
