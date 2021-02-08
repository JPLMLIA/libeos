#include <stdlib.h>

#include "eos_ethemis.h"
#include "eos_heap.h"
#include "eos_log.h"

EosStatus eos_ethemis_detect_anomaly_band(const EosObsShape shape,
        const U16* data, const U16 threshold,
        U32* n_results, EosPixelDetection* results) {

    EosStatus status;
    EosPixelDetection det;
    EosDetectionHeap heap;

    if (eos_assert(n_results != NULL)) { return EOS_ASSERT_ERROR; }

    // If we are asked to compute 0 results, just return success
    if (*n_results == 0) {
        return EOS_SUCCESS;
    }

    // If the observation is zero size, just return success with zero results
    if (shape.rows == 0 || shape.cols == 0) {
        *n_results = 0;
        return EOS_SUCCESS;
    }

    // If we made it past the previous checks, we need to make sure these
    // pointers aren't NULL (it's ok if they were NULL if either the
    // observation size or n_results were zero)
    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(results != NULL)) { return EOS_ASSERT_ERROR; }

    // Initialize heap with results array
    heap.capacity = *n_results;
    heap.size = 0;
    heap.data = results;

    for (det.row = 0; det.row < shape.rows; det.row++) {
        for (det.col = 0; det.col < shape.cols; det.col++) {
            det.score = data[det.row*shape.cols + det.col];
            if (det.score >= (F64)threshold) {
                status = detection_heap_push(&heap, det);
                if (status != EOS_SUCCESS) { return status; }
            }
        }
    }

    status = detection_heap_sort(&heap);
    if (status != EOS_SUCCESS) { return status; }

    // The number of results is equal to the size of the heap
    *n_results = heap.size;

    return status;
}

