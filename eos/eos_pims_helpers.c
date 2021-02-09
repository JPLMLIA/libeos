#include <stdlib.h>

#include "eos_pims_helpers.h"
#include "eos_log.h"
#include "eos_types_pub.h"

/* Deep-copies observations. */
EosStatus deepcopy_observation(EosPimsObservation* src, EosPimsObservation* dst){
    if(eos_assert(dst != NULL) || eos_assert(src != NULL)){
        return EOS_VALUE_ERROR;
    }

    dst -> observation_id = src -> observation_id;
    dst -> timestamp = src -> timestamp;
    dst -> num_bins = src -> num_bins;
    dst -> mode = src -> mode;
    dst -> bin_log_energies = src -> bin_log_energies;

    for(U32 bin = 0; bin < src -> num_bins; ++bin){
        dst -> bin_counts[bin] = src -> bin_counts[bin];
    }

    return EOS_SUCCESS;
}

/* Checks if bin definitions are the same. */
U32 check_bin_definitions(EosPimsObservation obs1, EosPimsObservation obs2){

    if(obs1.num_bins != obs2.num_bins){
        return EOS_FALSE;
    }

    F32 bin_energy_diff;
    for(U32 bin = 0; bin < obs1.num_bins; ++bin){
        bin_energy_diff = (obs1.bin_log_energies[bin] - obs2.bin_log_energies[bin]);
        if(bin_energy_diff > 1e-6 || bin_energy_diff < -1e-6){
            return EOS_FALSE;
        }
    }

    return EOS_TRUE;
}

/* Checks whether two EosPimsObservations are equal:
 * same number of bins, same counts, same bin definitions. */
U32 check_equality(EosPimsObservation obs1, EosPimsObservation obs2){

    if(!check_bin_definitions(obs1, obs2)){
        return EOS_FALSE;
    }

    F32 bin_counts_diff;
    for(U32 bin = 0; bin < obs1.num_bins; ++bin){
        bin_counts_diff = (obs1.bin_counts[bin] - obs2.bin_counts[bin]);
        if(bin_counts_diff > 1e-6 || bin_counts_diff < -1e-6){
            return EOS_FALSE;
        }
    }

    return EOS_TRUE;
}


/* 
 * Functions for interfacing with the queue of packets.
 */

/* Initialize the queue. */
EosStatus queue_init(EosPimsObservationQueue* q){
    q -> head = 0;
    q -> tail = 0;
    return EOS_SUCCESS;
}

/* Queue size. */
U32 queue_size(EosPimsObservationQueue q){
    return (q.tail + ((q.max_size + 1) - q.head)) % (q.max_size + 1);
}

/* Is the queue full? */
U32 queue_full(EosPimsObservationQueue q){
    return queue_size(q) == q.max_size;
}

/* Is the queue empty? */
U32 queue_empty(EosPimsObservationQueue q){
    return queue_size(q) == 0;
}

/* Push to the back of the queue. */
EosStatus queue_push(EosPimsObservationQueue* q, EosPimsObservation obs){
    /* Check for null pointers. */
    if(eos_assert(q != NULL)){
        return EOS_VALUE_ERROR;
    }

    if(queue_full(*q)){
        return EOS_PIMS_QUEUE_FULL;
    }

    q -> observations[q -> tail] = obs;
    q -> tail = (q -> tail + 1) % (q -> max_size + 1);
    return EOS_SUCCESS;
}

/* Pop from the front of the queue. */
EosStatus queue_pop(EosPimsObservationQueue* q){
    /* Check for null pointers. */
    if(eos_assert(q != NULL)){
        return EOS_VALUE_ERROR;
    }

    if(queue_empty(*q)){
        return EOS_PIMS_QUEUE_EMPTY;
    }

    q -> head = (q -> head + 1) % (q -> max_size + 1);
    return EOS_SUCCESS;
}

/* Assigns the last element of the queue to 'obs'. */
EosStatus queue_tail(EosPimsObservationQueue q, EosPimsObservation* obs){
    /* Check for null pointers. */
    if(eos_assert(obs != NULL)){
        return EOS_VALUE_ERROR;
    }

    /* If the queue is empty, there is no last element! */
    if(queue_empty(q)){
        return EOS_PIMS_QUEUE_EMPTY;
    }

    U32 last_index = (q.tail + q.max_size) % (q.max_size + 1);
    *obs = q.observations[last_index];
    return EOS_SUCCESS;
}

/* Make 'obs' point to the first element in the queue. */
EosStatus queue_begin(EosPimsObservationQueue q, EosPimsObservation** obs){
    /* Check for null pointers. */
    if(eos_assert(obs != NULL)){
        return EOS_VALUE_ERROR;
    }

    *obs = &(q.observations[q.head]);
    return EOS_SUCCESS;
}

/* Make 'obs' point just beyond the last element in the queue. */
EosStatus queue_end(EosPimsObservationQueue q, EosPimsObservation** obs){
    /* Check for null pointers. */
    if(eos_assert(obs != NULL)){
        return EOS_VALUE_ERROR;
    }

    *obs = &(q.observations[q.tail]);
    return EOS_SUCCESS;
}

/* Increment 'obs' to point to the next element in the queue. */
EosStatus queue_next(EosPimsObservationQueue q, EosPimsObservation** obs){
    /* Check for null pointers. */
    if(eos_assert(obs != NULL) || eos_assert(*obs != NULL)){
        return EOS_VALUE_ERROR;
    }

    /* *obs must be within the bounds of the array. */
    if(eos_assert(*obs - q.observations <= q.max_size)){
        return EOS_VALUE_ERROR;
    }

    /* Increment pointer. */
    *obs = *obs + 1;

    /* Check if we have gone off the end of the array.
     * If so, go back to the start. */
    if(*obs - q.observations == (q.max_size + 1)){
        *obs = &(q.observations[0]);
    }

    return EOS_SUCCESS;
}
