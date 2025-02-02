#pragma once

#include <core/platform.h>

typedef struct crude_string_buffer
{
  char    *data;
  uint32  buffer_size;
  uint32  current_size;
  void    *allocator;
} crude_string_buffer;

void crude_string_buffer_initialize( crude_string_buffer* string, sizet size, void* allocator );
void crude_string_buffer_append( crude_string_buffer* string, const char* appended_string );
void crude_string_buffer_append_f( crude_string_buffer* string, const char* format, ... );
void crude_string_buffer_deinitialize( crude_string_buffer* string );