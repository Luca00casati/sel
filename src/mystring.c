#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mystring.h"

void string(String *str, const char* cstr){
  DEBUG_HALT_IF(str->data != NULL || str->len != 0, "error invalid string string\n");
  str->len = strlen(cstr);
  str->data = (byte*)malloc(sizeof(byte) * str->len);
  if (str->data == NULL){
    fprintf(stderr, "error malloc fail string\n");
    exit(1);
  }
  memcpy(str->data, cstr, str->len); 
}

String arena_strdup(Arena *arena, const char *src) {
    String s = {0};
    s.len = strlen(src);
    s.data = arena_alloc(arena, s.len);
    memcpy(s.data, src, s.len);
    return s;
}

void free_string(String *str){
  free(str->data);
  str->len = 0;
}
