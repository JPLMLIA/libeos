#ifndef JPL_EOS_MEMORY
#define JPL_EOS_MEMORY

#include "eos_types.h"

#define EOS_MEMORY_STACK_MAX_DEPTH 20
#define ALIGN_SIZE 8

EosStatus memory_init(void *initial_memory_ptr, U64 initial_memory_size, U64 required_nbytes);
void memory_teardown();

void lifo_stack_clear();
EosMemoryBuffer *lifo_allocate_buffer(U64 nbytes);
EosStatus lifo_deallocate_buffer(EosMemoryBuffer *buffer);
EosStatus lifo_allocate_buffer_checked(EosMemoryBuffer** buffer, U64 nbytes,
                                       const CHAR* error_message);
U32 lifo_stack_entries();

#endif
