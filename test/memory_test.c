#include <stdlib.h>
#include <stdint.h>

#include <eos_memory.h>
#include "CuTest.h"
#include "util.h"

void TestInsufficientMemory(CuTest *ct) {
    EosStatus status = memory_init(NULL, 1, 2);
    CuAssertIntEquals(ct, EOS_INSUFFICIENT_MEMORY, status);
}

void TestNullMemoryPointer(CuTest *ct) {
    /* Amount of memory requested is fine, but pointer is NULL;
     * e.g., the preceding call to malloc() failed. */
    EosStatus status = memory_init(NULL, 1, 1);
    CuAssertIntEquals(ct, EOS_ASSERT_ERROR, status);
}

void TestMallocFailMemory(CuTest *ct) {
    EosStatus status = memory_init(NULL, 0, SIZE_MAX / 2);
    CuAssertIntEquals(ct, EOS_INSUFFICIENT_MEMORY, status);
}

void TestOverAllocate(CuTest *ct) {
    int size = 1024;
    void *ptr = malloc(size);
    EosStatus status = memory_init(ptr, size, size);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    EosMemoryBuffer *buffer = NULL;
    buffer = lifo_allocate_buffer(2048);
    CuAssertPtrEquals(ct, NULL, buffer);
    memory_teardown();
    free(ptr);
}

void TestCheckedAllocation(CuTest *ct) {
    int size = 1024;
    void *ptr = malloc(size);
    EosStatus status = memory_init(ptr, size, size);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    EosMemoryBuffer *buffer = NULL;

    status = lifo_allocate_buffer_checked(&buffer, 2048, "test");
    CuAssertIntEquals(ct, EOS_INSUFFICIENT_MEMORY, status);
    CuAssertPtrEquals(ct, NULL, buffer);

    status = lifo_allocate_buffer_checked(&buffer, 512, "test");
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    memory_teardown();
    free(ptr);
}

void TestStackOverflow(CuTest *ct) {
    int size = 1024;
    void *ptr = malloc(size);
    EosStatus status = memory_init(ptr, size, size);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    int i;
    EosMemoryBuffer *buffer = NULL;
    for (i = 0; i < EOS_MEMORY_STACK_MAX_DEPTH; i++) {
        buffer = lifo_allocate_buffer(1);
        CuAssertPtrNotNull(ct, buffer);
    }
    buffer = lifo_allocate_buffer(1);
    CuAssertPtrEquals(ct, NULL, buffer);
    memory_teardown();
    free(ptr);
}

void TestBadDeallocationOrder(CuTest *ct) {
    int size = 1024;
    void *ptr = malloc(size);
    EosStatus status = memory_init(ptr, size, size);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    EosMemoryBuffer *buffer1 = lifo_allocate_buffer(1);
    lifo_allocate_buffer(1);
    EosMemoryBuffer *buffer3 = lifo_allocate_buffer(1);
    status = lifo_deallocate_buffer(buffer3);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    status = lifo_deallocate_buffer(buffer1);
    CuAssertIntEquals(ct, EOS_LIFO_MEMORY_VIOLATION, status);
    memory_teardown();
    free(ptr);
}

void TestAllocateAfterTeardown(CuTest *ct) {
    int size = 1024;
    void *ptr = malloc(size);
    EosStatus status = memory_init(ptr, size, size);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    
    EosMemoryBuffer *buffer1;
    status = lifo_allocate_buffer_checked(&buffer1, 10, "buffer1");
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = lifo_deallocate_buffer(buffer1);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    memory_teardown();

    status = lifo_allocate_buffer_checked(&buffer1, 10, "buffer1");
    CuAssertIntEquals(ct, EOS_INSUFFICIENT_MEMORY, status);

    free(ptr);
}

void MemoryLeakFunction(){
    EosMemoryBuffer *buffer1 = lifo_allocate_buffer(10);
    (void) buffer1;
}

void NoMemoryLeakFunction(){
    EosMemoryBuffer *buffer1 = lifo_allocate_buffer(10);
    lifo_deallocate_buffer(buffer1);
}

void TestMemoryLeakDetection(CuTest *ct){
    int size = 1024;
    void *ptr = malloc(size);
    EosStatus status = memory_init(ptr, size, size);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    /* Check whether the memory leak in MemoryLeakFunction() is detected. */
    U32 state = current_memory_state();
    MemoryLeakFunction();
    CuAssertTrue(ct, memory_leak(state));

    memory_teardown();

    status = memory_init(ptr, size, size);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    /* Check that NoMemoryLeakFunction() doesn't have a memory leak. */
    state = current_memory_state();
    NoMemoryLeakFunction();
    CuAssertTrue(ct, !memory_leak(state));

    memory_teardown();
    free(ptr);
}

CuSuite* CuMemoryGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestInsufficientMemory);
    SUITE_ADD_TEST(suite, TestNullMemoryPointer);
    SUITE_ADD_TEST(suite, TestMallocFailMemory);
    SUITE_ADD_TEST(suite, TestOverAllocate);
    SUITE_ADD_TEST(suite, TestCheckedAllocation);
    SUITE_ADD_TEST(suite, TestStackOverflow);
    SUITE_ADD_TEST(suite, TestBadDeallocationOrder);
    SUITE_ADD_TEST(suite, TestAllocateAfterTeardown);
    SUITE_ADD_TEST(suite, TestMemoryLeakDetection);

    return suite;
}
