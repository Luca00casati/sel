#include <stdint.h>
#include <stddef.h>
#pragma once

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define HALT_IF(cond, error) do{if(cond){perror(error);abort();}}while(0)

#ifndef NDEBUG
#define DEBUG_HALT_IF(cond, error) HALT_IF(cond, error)
#else
#define DEBUG_HALT_IF(cond, error) ((void)0)
#endif

typedef uint8_t byte;
typedef int64_t word;
typedef size_t word_size;
