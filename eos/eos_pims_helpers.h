#ifndef JPL_EOS_PIMS_HELPERS
#define JPL_EOS_PIMS_HELPERS

#include "eos_types.h"

/* Deep-copies observations. */
EosStatus deepcopy_observation(EosPimsObservation* src, EosPimsObservation* dst);

/* Checks if bin definitions are the same. */
U32 check_bin_definitions(EosPimsObservation obs1, EosPimsObservation obs2);

/* Checks if bin definitions and counts are the same. */
U32 check_equality(EosPimsObservation obs1, EosPimsObservation obs2);

/* EosPimsObservationQueue functions. */
EosStatus queue_init(EosPimsObservationQueue* q);
U32 queue_size(EosPimsObservationQueue q);
U32 queue_full(EosPimsObservationQueue q);
U32 queue_empty(EosPimsObservationQueue q);
EosStatus queue_pop(EosPimsObservationQueue* q);
EosStatus queue_push(EosPimsObservationQueue* q, EosPimsObservation obs);
EosStatus queue_tail(EosPimsObservationQueue q, EosPimsObservation* obs);
EosStatus queue_begin(EosPimsObservationQueue q, EosPimsObservation** obs);
EosStatus queue_end(EosPimsObservationQueue q, EosPimsObservation** obs);
EosStatus queue_next(EosPimsObservationQueue q, EosPimsObservation** obs);

#endif
