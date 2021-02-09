#include <stdlib.h>
#include <stdio.h>

#include <eos_types.h>
#include <eos_heap.h>
#include "CuTest.h"

void TestHeapAdd1(CuTest *ct) {
    EosStatus status;
    EosDetectionHeap heap;
    EosPixelDetection det; // Dummy detection

    heap.capacity = 1;
    heap.size = 0;
    heap.data = calloc(sizeof(EosPixelDetection), heap.capacity);

    det.score = 1;
    status = detection_heap_push(&heap, det);
    // check status
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    // check that heap has one item and it is a 1
    CuAssertIntEquals(ct, 1, heap.size);
    CuAssertDblEquals(ct, 1.0, heap.data[0].score, 1e-9);

    // check again after sorting
    status = detection_heap_sort(&heap);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 1, heap.size);
    CuAssertDblEquals(ct, 1.0, heap.data[0].score, 1e-9);

    // clean up
    free(heap.data);
}

void TestHeapAdd1Then2(CuTest *ct) {
    EosStatus status;
    EosDetectionHeap heap;
    EosPixelDetection det; // Dummy detection

    heap.capacity = 2;
    heap.size = 0;
    heap.data = calloc(sizeof(EosPixelDetection), heap.capacity);

    det.score = 1;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    det.score = 2;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    // check that heap has two items and top is 1
    CuAssertIntEquals(ct, 2, heap.size);
    CuAssertDblEquals(ct, 1, heap.data[0].score, 1e-9);
    CuAssertDblEquals(ct, 2, heap.data[1].score, 1e-9);

    // check again after sorting; top should be 2
    status = detection_heap_sort(&heap);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 2, heap.size);
    CuAssertDblEquals(ct, 2, heap.data[0].score, 1e-9);
    CuAssertDblEquals(ct, 1, heap.data[1].score, 1e-9);

    // clean up
    free(heap.data);
}

void TestHeapAdd2Then1(CuTest *ct) {
    EosStatus status;
    EosDetectionHeap heap;
    EosPixelDetection det; // Dummy detection

    heap.capacity = 2;
    heap.size = 0;
    heap.data = calloc(sizeof(EosPixelDetection), heap.capacity);

    det.score = 2;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    det.score = 1;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    // check that heap has two items and top is 1
    CuAssertIntEquals(ct, 2, heap.size);
    CuAssertDblEquals(ct, 1, heap.data[0].score, 1e-9);
    CuAssertDblEquals(ct, 2, heap.data[1].score, 1e-9);

    // check again after sorting; top should be 2
    status = detection_heap_sort(&heap);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 2, heap.size);
    CuAssertDblEquals(ct, 2, heap.data[0].score, 1e-9);
    CuAssertDblEquals(ct, 1, heap.data[1].score, 1e-9);

    // clean up
    free(heap.data);
}

void TestHeapAdd1Then1(CuTest *ct) {
    EosStatus status;
    EosDetectionHeap heap;
    EosPixelDetection det; // Dummy detection

    heap.capacity = 2;
    heap.size = 0;
    heap.data = calloc(sizeof(EosPixelDetection), heap.capacity);

    det.score = 1;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    det.score = 1;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    // check that heap has two items and top is 1
    CuAssertIntEquals(ct, 2, heap.size);
    CuAssertDblEquals(ct, 1, heap.data[0].score, 1e-9);
    CuAssertDblEquals(ct, 1, heap.data[1].score, 1e-9);

    // check again after sorting; top should be 1
    status = detection_heap_sort(&heap);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 2, heap.size);
    CuAssertDblEquals(ct, 1, heap.data[0].score, 1e-9);
    CuAssertDblEquals(ct, 1, heap.data[1].score, 1e-9);

    // clean up
    free(heap.data);
}

/*
 * Test behavior if we add 5 items in order;
 * should exercise the case where a pushed item is greater than both children.
 */
void TestHeapAdd12345(CuTest *ct) {
    EosStatus status;
    EosDetectionHeap heap;
    EosPixelDetection det; // Dummy detection

    heap.capacity = 5;
    heap.size = 0;
    heap.data = calloc(sizeof(EosPixelDetection), heap.capacity);

    uint16_t i;
    for (i=1; i<=heap.capacity; i++) {
        det.score = i;
        status = detection_heap_push(&heap, det);
        CuAssertIntEquals(ct, EOS_SUCCESS, status);
    }

    // check that heap has four items and contains what we expect
    CuAssertIntEquals(ct, heap.capacity, heap.size);
    for (i=0; i<heap.capacity; i++) {
        CuAssertDblEquals(ct, i+1, heap.data[i].score, 1e-9);
    }

    // check again after sorting; top should be 5
    status = detection_heap_sort(&heap);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, heap.capacity, heap.size);
    CuAssertDblEquals(ct, 5, heap.data[0].score, 1e-9);

    // clean up
    free(heap.data);
}

/*
 * If we add an item beyond capacity, and it's higher than existing items,
 * it should replace them.
 */
void TestHeapAddExtraHigh(CuTest *ct) {
    EosStatus status;
    EosDetectionHeap heap;
    EosPixelDetection det; // Dummy detection

    heap.capacity = 1;
    heap.size = 0;
    heap.data = calloc(sizeof(EosPixelDetection), heap.capacity);

    det.score = 1;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    det.score = 2;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    // check that heap has one item and it is a 2
    CuAssertIntEquals(ct, 1, heap.size);
    CuAssertDblEquals(ct, 2, heap.data[0].score, 1e-9);

    // clean up
    free(heap.data);
}

/*
 * If we add an item beyond capacity, and it's NOT higher than existing items,
 * it should be ignored.
 */
void TestHeapAddExtraLow(CuTest *ct) {
    EosStatus status;
    EosDetectionHeap heap;
    EosPixelDetection det; // Dummy detection

    heap.capacity = 1;
    heap.size = 0;
    heap.data = calloc(sizeof(EosPixelDetection), heap.capacity);

    det.score = 1;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    det.score = 0;
    status = detection_heap_push(&heap, det);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    // check that heap has one item and it is a 1
    CuAssertIntEquals(ct, 1, heap.size);
    CuAssertDblEquals(ct, 1, heap.data[0].score, 1e-9);

    // clean up
    free(heap.data);
}

/*
 * Sorting an empty heap should do nothing (with success).
 */
void TestHeapSortEmpty(CuTest *ct) {
    EosStatus status;
    EosDetectionHeap heap;

    heap.capacity = 1;
    heap.size = 0;
    heap.data = NULL;

    status = detection_heap_sort(&heap);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

CuSuite* CuHeapGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestHeapAdd1);
    SUITE_ADD_TEST(suite, TestHeapAdd1Then2);
    SUITE_ADD_TEST(suite, TestHeapAdd2Then1);
    SUITE_ADD_TEST(suite, TestHeapAdd1Then1);
    SUITE_ADD_TEST(suite, TestHeapAddExtraHigh);
    SUITE_ADD_TEST(suite, TestHeapAddExtraLow);
    SUITE_ADD_TEST(suite, TestHeapAdd12345);
    SUITE_ADD_TEST(suite, TestHeapSortEmpty);

    return suite;
}
