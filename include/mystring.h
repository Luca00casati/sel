#pragma once
#include "common.h"

typedef struct{
  byte* data;
  size_t len;
}String;

void string(String *str, const char* cstr);

void free_string(String *str);

