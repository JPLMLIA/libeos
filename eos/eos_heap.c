/*
 * Methods to support maintenance of a detection heap structure,
 * sorted so that the smallest score detection is always on top.
 * (Heap stores the top 'size' detections)
 */
#include <stdlib.h>

#include "eos_heap.h"
#include "eos_log.h"

/*
 * Bubbles the last element in a heap up to maintain the heap property
 */
EosStatus detection_heap_bubble_up(EosDetectionHeap* heap) {
    U32 parent;
    U32 i;
    U32 j;

    if (eos_assert(heap != NULL)) { return EOS_ASSERT_ERROR; }

    i = heap->size - 1;
    // while (i > 0); loops bounded by heap capacity
    for (j = 0; j <= heap->capacity; j++) {
        if (i <= 0) { break; }
        parent = (i - 1) / 2;
        if (heap->data[parent].score > heap->data[i].score) {
            EosPixelDetection tmp = heap->data[parent];
            heap->data[parent] = heap->data[i];
            heap->data[i] = tmp;
        } else {
            break;
        }
        i = parent;
    }
    // Assert that we broke out of loop before exceeding bound
    if (eos_assert(j <= heap->capacity)) { return EOS_ASSERT_ERROR; }
    return EOS_SUCCESS;
}

/*
 * Sifts the top element in a heap down to maintain the heap property
 */
EosStatus detection_heap_sift_down(EosDetectionHeap* heap) {
    EosPixelDetection tmp;
    U32 swap;
    U32 child;
    U32 i = 0;
    U32 j;

    if (eos_assert(heap != NULL)) { return EOS_ASSERT_ERROR; }

    // while ((2*i + 1) < heap->size); loops bounded by size
    for (j = 0; j <= heap->size; j++) {
        if ((2*i + 1) >= heap->size) { break; }
        child = 2*i + 1;
        swap = i;
        if (heap->data[swap].score > heap->data[child].score) {
            swap = child;
        }
        if (((child + 1) < heap->size)
            && (heap->data[swap].score >
                heap->data[child +1].score)) {
            swap = child + 1;
        }
        if (swap == i) { break; }
        tmp = heap->data[swap];
        heap->data[swap] = heap->data[i];
        heap->data[i] = tmp;
        i = swap;
    }
    // Assert that we broke out of loop before exceeding bound
    if (eos_assert(j <= heap->size)) { return EOS_ASSERT_ERROR; }
    return EOS_SUCCESS;
}

// Push the detection onto a minimum-score-on-top heap
EosStatus detection_heap_push(EosDetectionHeap* heap,
        EosPixelDetection det) {

    EosStatus status;
    if (eos_assert(heap != NULL)) { return EOS_ASSERT_ERROR; }
    if (heap->capacity == 0) { return EOS_SUCCESS; }
    if (heap->capacity == heap->size) {
        // Heap already full, either replace an element or ignore
        if (det.score > heap->data[0].score) {
            // score larger than that of smallest element, so swap and sift down
            heap->data[0] = det;
            status = detection_heap_sift_down(heap);
            if (status != EOS_SUCCESS) { return status; }
        }
    } else {
        // Not full yet; add to end of list and bubble up
        heap->data[heap->size] = det;
        heap->size++;
        status = detection_heap_bubble_up(heap);
        if (status != EOS_SUCCESS) { return status; }
    }
    return EOS_SUCCESS;
}

// Sort the heap in-place; the heap property is not maintained by this
// operation
EosStatus detection_heap_sort(EosDetectionHeap* heap) {
    EosStatus status;
    EosPixelDetection tmp;
    EosDetectionHeap sort_heap;
    U32 i;
    if (eos_assert(heap != NULL)) { return EOS_ASSERT_ERROR; }
    if (heap->size <= 1) { return EOS_SUCCESS; }

    // Make a copy of the heap for sorting (the sorting heap will shrink;
    // the original heap will remain the same size)
    sort_heap = *heap;
    sort_heap.capacity = sort_heap.size;
    for (i = 0; i < heap->size - 1; i++) {
        // Swap top of heap to the end
        tmp = sort_heap.data[0];
        sort_heap.data[0] = sort_heap.data[sort_heap.size - 1];
        sort_heap.data[sort_heap.size - 1] = tmp;
        sort_heap.size--;

        // Sift the top entry back down
        status = detection_heap_sift_down(&sort_heap);
        if (status != EOS_SUCCESS) { return status; }
    }
    if (eos_assert(sort_heap.size == 1)) { return EOS_ASSERT_ERROR; }

    return EOS_SUCCESS;
}

