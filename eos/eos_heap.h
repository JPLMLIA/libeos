#ifndef JPL_EOS_HEAP
#define JPL_EOS_HEAP

#include "eos_types.h"

EosStatus detection_heap_sift_down(EosDetectionHeap* heap);
EosStatus detection_heap_bubble_up(EosDetectionHeap* heap);

EosStatus detection_heap_push(EosDetectionHeap* heap,
                              EosPixelDetection detection);

EosStatus detection_heap_sort(EosDetectionHeap* heap);

#endif
