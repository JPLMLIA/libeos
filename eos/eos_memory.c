#include <stdlib.h>
#include <string.h>

#include "eos_memory.h"
#include "eos_types.h"
#include "eos_util.h"
#include "eos_log.h"

static EosMemoryBuffer eos_memory_stack[EOS_MEMORY_STACK_MAX_DEPTH];
static void *eos_memory_ptr = NULL;
static U64 eos_memory_nbytes = 0;
static U8 self_allocated = EOS_FALSE;

U32 alignment_padding_nbytes(U64 ptr) {
    return (U32)((ALIGN_SIZE - (ptr % ALIGN_SIZE)) % ALIGN_SIZE);
}

EosStatus memory_init(void *initial_memory_ptr, U64 initial_memory_size, U64 required_nbytes) {
    U32 nbytes_padding;
    if (initial_memory_size > 0) {
        nbytes_padding = alignment_padding_nbytes((U64)initial_memory_ptr);
        if (initial_memory_size - nbytes_padding < required_nbytes) {
            eos_log(EOS_LOG_ERROR, "Provided memory less than required.");
            eos_memory_nbytes = 0;
            return EOS_INSUFFICIENT_MEMORY;
        }
        if (eos_assert(initial_memory_ptr != NULL)) { return EOS_ASSERT_ERROR; }
        eos_memory_nbytes = initial_memory_size - nbytes_padding;
        eos_memory_ptr = byte_offset(initial_memory_ptr, nbytes_padding);
        self_allocated = EOS_FALSE;
    } else {
        eos_logf(EOS_LOG_INFO,
            "No memory provided, so allocate our own memory (%lu bytes).",
            (U32) required_nbytes);
        eos_memory_ptr = malloc(required_nbytes);
        if (eos_memory_ptr == NULL) {
            eos_log(EOS_LOG_ERROR, "No memory provided and malloc failed.");
            return EOS_INSUFFICIENT_MEMORY;
        }
        eos_memory_nbytes = required_nbytes;
        self_allocated = EOS_TRUE;
    }

    lifo_stack_clear();

    return EOS_SUCCESS;
}

void memory_teardown() {
    if (self_allocated) {
        free(eos_memory_ptr);
    }
    eos_memory_ptr = NULL;
    eos_memory_nbytes = 0;
    self_allocated = EOS_FALSE;
}

void lifo_stack_clear() {
    memset((void*)eos_memory_stack, 0,
        EOS_MEMORY_STACK_MAX_DEPTH*sizeof(EosMemoryBuffer));
}

EosMemoryBuffer *lifo_allocate_buffer(U64 nbytes) {
    U64 aligned_nbytes;
    U64 required_nbytes;
    I32 i;
    U64 preallocated = 0;
    EosMemoryBuffer* buffer;

    aligned_nbytes = nbytes + alignment_padding_nbytes(nbytes);
    for (i = 0; i < EOS_MEMORY_STACK_MAX_DEPTH; i++) {
        if(eos_memory_stack[i].ptr == NULL) {
            break;
        }
        preallocated += eos_memory_stack[i].size;
    }

    if (i >= EOS_MEMORY_STACK_MAX_DEPTH) {
        eos_log(EOS_LOG_ERROR, "Stack depth exceeded.");
        return NULL;
    }

    required_nbytes = preallocated + aligned_nbytes;
    if (required_nbytes > eos_memory_nbytes) {
        eos_logf(EOS_LOG_ERROR,
                 "Required %lu bytes for allocation, %lu available.",
                 (U32) required_nbytes, (U32) eos_memory_nbytes);
        return NULL;
    }

    buffer = &eos_memory_stack[i];
    buffer->ptr = byte_offset(eos_memory_ptr, preallocated);
    buffer->size = aligned_nbytes;
    memset(buffer->ptr, 0, buffer->size);

    return buffer;
}

EosStatus lifo_deallocate_buffer(EosMemoryBuffer *buffer) {
    I32 i;
    if (eos_assert(buffer != NULL)) { return EOS_ASSERT_ERROR; }
    for (i = 0; i < EOS_MEMORY_STACK_MAX_DEPTH; i++) {
        if(eos_memory_stack[i].ptr == NULL) {
            break;
        }
    }
    if (&eos_memory_stack[i-1] != buffer) {
        eos_log(EOS_LOG_ERROR, "Memory not deallocated in LIFO order.");
        return EOS_LIFO_MEMORY_VIOLATION;
    }
    buffer->ptr = NULL;
    buffer->size = 0;
    return EOS_SUCCESS;
}

EosStatus lifo_allocate_buffer_checked(EosMemoryBuffer** buffer, U64 nbytes,
                                       const CHAR* error_message) {
    if (eos_assert(buffer != NULL)) { return EOS_ASSERT_ERROR; }
    *buffer = lifo_allocate_buffer(nbytes);
    if (*buffer == NULL) {
        eos_logf(EOS_LOG_ERROR,
            "Unable to allocate %ld bytes for \"%s\"",
            nbytes, error_message
        );
        return EOS_INSUFFICIENT_MEMORY;
    }
    return EOS_SUCCESS;
}

/* Returns the index of the first unallocated entry in eos_memory_stack. */
U32 lifo_stack_entries() {
    U32 i;
    for (i = 0; i < EOS_MEMORY_STACK_MAX_DEPTH; i++) {
        if(eos_memory_stack[i].ptr == NULL) {
            break;
        }
    }
    return i;
}
