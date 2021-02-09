#include <stdlib.h>
#include <math.h>

#include "eos_memory.h"
#include "eos_pims_filters.h"
#include "eos_pims_helpers.h"
#include "eos_util.h"
#include "eos_log.h"
#include "eos_types_pub.h"

/* Minimum filter for 'obs' against all observations in 'q'. */
EosStatus _minimum_filter(EosPimsObservation obs, EosPimsObservationQueue q){

    /* Check for null pointers. */
    if(eos_assert(q.observations != NULL) ||
       eos_assert(obs.bin_counts != NULL) ||
       eos_assert(obs.bin_log_energies != NULL)){
        return EOS_VALUE_ERROR;
    }

    /* Initialize observations that point to the beginning
     * and (just beyond) the end of the queue. */
    EosPimsObservation* q_begin;
    EosPimsObservation* q_end;

    EosStatus status = EOS_SUCCESS;
    status |= queue_begin(q, &q_begin);
    status |= queue_end(q, &q_end);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Loop over all of these observations. */
    for(EosPimsObservation* q_obs = q_begin; q_obs != q_end; status = queue_next(q, &q_obs)){
        if(status != EOS_SUCCESS){
            return status;
        }

        /* Check if the observation has valid counts. */
        if(eos_assert(q_obs -> bin_counts != NULL)){
            return EOS_VALUE_ERROR;
        }

        /* Check if the bins match up with previous observations. */
        if(eos_assert(q_obs -> num_bins == obs.num_bins)){
            return EOS_PIMS_BINS_MISMATCH_ERROR;
        }

        /* Across each of these observations, and each bin, take the minimum. */
        for(U32 bin = 0; bin < obs.num_bins; ++bin){
            obs.bin_counts[bin] = eos_umin(obs.bin_counts[bin], q_obs -> bin_counts[bin]);
        }
    }

    return EOS_SUCCESS;
}

/* Mean filter for 'obs' against all observations in 'q'. */
EosStatus _mean_filter(EosPimsObservation obs, EosPimsObservationQueue q){

    /* Check for null pointers. */
    if(eos_assert(q.observations != NULL) ||
       eos_assert(obs.bin_counts != NULL) ||
       eos_assert(obs.bin_log_energies != NULL)){
        return EOS_VALUE_ERROR;
    }

    /* Initialize observations that point to the beginning
     * and (just beyond) the end of the queue. */
    EosPimsObservation* q_begin;
    EosPimsObservation* q_end;

    EosStatus status = EOS_SUCCESS;
    status |= queue_begin(q, &q_begin);
    status |= queue_end(q, &q_end);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Allocate array to store sums over bins: U64 to be safe. */
    EosMemoryBuffer* sum_bins_buf;
    status = lifo_allocate_buffer_checked(&sum_bins_buf,
                                          sizeof(U64) * obs.num_bins,
                                          "sum over bins buffer");
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Initialize sum array. */
    U64* sum_bins = (U64*) sum_bins_buf -> ptr;
    for(U32 bin = 0; bin < obs.num_bins; ++bin){
        sum_bins[bin] = obs.bin_counts[bin];
    }

    /* Loop over all of these observations. */
    for(EosPimsObservation* q_obs = q_begin; q_obs != q_end; status = queue_next(q, &q_obs)){
        if(status != EOS_SUCCESS){
            lifo_deallocate_buffer(sum_bins_buf);
            return status;
        }

        /* Check if the observation has valid counts. */
        if(eos_assert(q_obs -> bin_counts != NULL)){
            lifo_deallocate_buffer(sum_bins_buf);
            return EOS_VALUE_ERROR;
        }

        /* Check if the bins match up with previous observations. */
        if(eos_assert(q_obs -> num_bins == obs.num_bins)){
            lifo_deallocate_buffer(sum_bins_buf);
            return EOS_PIMS_BINS_MISMATCH_ERROR;
        }

        /* Across each of these observations, and each bin, take the sum.
         * We will divide by the number of observations later. */
        for(U32 bin = 0; bin < obs.num_bins; ++bin){
            sum_bins[bin] += q_obs -> bin_counts[bin];
        }
    }

    /* Now, we divide by the number of observations.
     * We have summed over all observations in the queue AND this new observation. */
    U32 num_obs = queue_size(q) + 1;
    for(U32 bin = 0; bin < obs.num_bins; ++bin){
        /* This is integer division! */
        obs.bin_counts[bin] = sum_bins[bin]/num_obs;
    }

    /* Deallocate memory for smoothed bin-counts. */
    status = lifo_deallocate_buffer(sum_bins_buf);
    return status;
}

/* A comparator function used in the median filter. */
I32 _median_filter_cmpfunc(const void *a, const void *b){
    pims_count_t x = *(const pims_count_t*) a;
    pims_count_t y = *(const pims_count_t*) b;

    /* Don't do 'return (x - y)' because these are unsigned integers!
     * Casting to I32 and then doing 'return (x - y)' can lead to overflow.
     * The below code is explicit, but safe. */
    if(x < y){
        return 1;
    } else if (x == y){
        return 0;
    } else {
        return -1;
    }
}

/* Median filter for observation 'obs' against all observations in queue 'q'. */
EosStatus _median_filter(EosPimsObservation obs, EosPimsObservationQueue q){

    /* Check for null pointers. */
    if(eos_assert(q.observations != NULL) ||
       eos_assert(obs.bin_counts != NULL) ||
       eos_assert(obs.bin_log_energies != NULL)){
        return EOS_VALUE_ERROR;
    }

    /* Initialize observations that point to the beginning
     * and (just beyond) the end of the queue. */
    EosPimsObservation* q_begin;
    EosPimsObservation* q_end;

    EosStatus status = EOS_SUCCESS;
    status |= queue_begin(q, &q_begin);
    status |= queue_end(q, &q_end);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Allocate space for copying off queue data. */
    U32 num_obs = queue_size(q) + 1;
    EosMemoryBuffer* q_copy_buf;
    status = lifo_allocate_buffer_checked(&q_copy_buf,
                                          sizeof(pims_count_t) * obs.num_bins * num_obs,
                                          "copy of queue buffer");
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Copy off the current observation as the last entry in the 2D array. */
    pims_count_t* q_copy = (pims_count_t*) q_copy_buf -> ptr;
    for(U32 bin = 0; bin < obs.num_bins; ++bin){
        q_copy[(bin * num_obs) + num_obs - 1] = obs.bin_counts[bin];
    }

    /* Loop over all of these observations. */
    U32 obs_index = 0;
    for(EosPimsObservation* q_obs = q_begin; q_obs != q_end; status = queue_next(q, &q_obs), obs_index++){
        if(status != EOS_SUCCESS){
            lifo_deallocate_buffer(q_copy_buf);
            return status;
        }

        /* Check if the observation has valid counts. */
        if(eos_assert(q_obs -> bin_counts != NULL)){
            lifo_deallocate_buffer(q_copy_buf);
            return EOS_VALUE_ERROR;
        }

        /* Check if the bins match up with previous observations. */
        if(eos_assert(q_obs -> num_bins == obs.num_bins)){
            lifo_deallocate_buffer(q_copy_buf);
            return EOS_PIMS_BINS_MISMATCH_ERROR;
        }

        /* Copy each observations data, bin-wise. */
        for(U32 bin = 0; bin < obs.num_bins; ++bin){
            q_copy[(bin * num_obs) + obs_index] = q_obs -> bin_counts[bin];
        }
    }

    /* Sort bin-wise. */
    for(U32 bin = 0; bin < obs.num_bins; ++bin){
        qsort(q_copy + bin * num_obs, num_obs, sizeof(pims_count_t), _median_filter_cmpfunc);
    }

    /* Set the mean of the center elements of the sorted 2D array,
     * across each bin, for the smoothed observation.
     * This works both when 'num_obs' is even and when it's odd. */
    for(U32 bin = 0; bin < obs.num_bins; ++bin){
        obs.bin_counts[bin] = (q_copy[(bin * num_obs) + (num_obs - 1)/2] + q_copy[(bin * num_obs) + num_obs/2])/2;
    }

    /* Deallocate memory for copy of observations. */
    status = lifo_deallocate_buffer(q_copy_buf);
    return status;
}

/* Maximum filter for observation 'obs' against all observations in queue 'q'. */
EosStatus _maximum_filter(EosPimsObservation obs, EosPimsObservationQueue q){

    /* Check for null pointers. */
    if(eos_assert(q.observations != NULL) ||
       eos_assert(obs.bin_counts != NULL) || eos_assert(obs.bin_log_energies != NULL)){
        return EOS_VALUE_ERROR;
    }

     /* Initialize observations that point to the beginning
     * and (just beyond) the end of the queue. */
    EosPimsObservation* q_begin;
    EosPimsObservation* q_end;

    EosStatus status = EOS_SUCCESS;
    status |= queue_begin(q, &q_begin);
    status |= queue_end(q, &q_end);
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Loop over all of these observations. */
    for(EosPimsObservation* q_obs = q_begin; q_obs != q_end; status = queue_next(q, &q_obs)){
        if(status != EOS_SUCCESS){
            return status;
        }

        /* Check if the observation has valid counts. */
        if(eos_assert(q_obs -> bin_counts != NULL)){
            return EOS_VALUE_ERROR;
        }

        /* Check if the bins match up with previous observations. */
        if(eos_assert(q_obs -> num_bins == obs.num_bins)){
            return EOS_PIMS_BINS_MISMATCH_ERROR;
        }

        /* Across each of these observations, and each bin, take the maximum. */
        for(U32 bin = 0; bin < obs.num_bins; ++bin){
            obs.bin_counts[bin] = eos_umax(obs.bin_counts[bin], q_obs -> bin_counts[bin]);
        }
    }

    return EOS_SUCCESS;
}

/* _mreq for filters, based on current implementations above. */
U64 eos_pims_filter_mreq(const EosPimsCommonParams* c_params){
    switch (c_params -> filter){
        case EOS_PIMS_MEAN_FILTER:
            return sizeof(U64) * (c_params -> max_bins);
        case EOS_PIMS_MEDIAN_FILTER:
            return sizeof(pims_count_t) * (c_params -> max_bins) * (1 + c_params -> max_observations);
        default:
            return 0;
    }
}

/* Router function for different filters. */
EosStatus eos_pims_filter(EosPimsFilter filter, EosPimsObservation obs, EosPimsObservationQueue q){

    switch(filter){
        case EOS_PIMS_NO_FILTER:
            return EOS_SUCCESS;

        case EOS_PIMS_MIN_FILTER:
            return _minimum_filter(obs, q);

        case EOS_PIMS_MEAN_FILTER:
            return _mean_filter(obs, q);

        case EOS_PIMS_MEDIAN_FILTER:
            return _median_filter(obs, q);

        case EOS_PIMS_MAX_FILTER:
            return _maximum_filter(obs, q);

        default:
            return EOS_VALUE_ERROR;
    }    
}
