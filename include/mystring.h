#pragma once
#include "common.h"
#include "arena.h"

typedef struct{
  byte* data;
  size_t len;
}String;

void string(String *str, const char* cstr);
String arena_strdup(Arena *arena, const char* cstr);
void free_string(String *str);

