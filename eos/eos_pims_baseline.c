#include <stdlib.h>

#include "eos_pims_baseline.h"
#include "eos_pims_helpers.h"
#include "eos_pims_filters.h"

#include "eos_memory.h"
#include "eos_types_pub.h"
#include "eos_util.h"
#include "eos_log.h"

/* Private function prototypes. */
EosStatus _squared_l2_diff(EosPimsObservation obs1, EosPimsObservation obs2, U64* result);

/* state_request() for 'baseline'. */
EosStatus eos_pims_baseline_state_request(EosPimsCommonParams* c_params, EosPimsBaselineParams* params, EosPimsBaselineStateRequest* req){
    (void) params;
    req -> queue_size = (c_params -> max_observations);
    req -> max_bins = (c_params -> max_bins);
    return EOS_SUCCESS;
}

/* init_mreq() for 'baseline'. */
U64 eos_pims_baseline_init_mreq(const EosPimsCommonParams* c_params, const EosPimsBaselineParams* params){
    /* We really don't do much in init()! */
    (void) params;
    (void) c_params;
    return 0;
}

/* init() for 'baseline'. */
EosStatus eos_pims_baseline_init(EosPimsCommonParams* c_params, EosPimsBaselineParams* params, EosPimsBaselineState* state){
    /* We initialize the queue as empty. */
    (void) c_params;
    (void) params;
    EosStatus status = queue_init(&(state -> queue));
    return status;
}

/* on_recv_mreq() for 'baseline'. */
U64 eos_pims_baseline_on_recv_mreq(const EosPimsCommonParams* c_params, const EosPimsBaselineParams* params){
    (void) params;
    U64 mem_req = sizeof(pims_count_t) * (c_params -> max_bins);
    mem_req += eos_pims_filter_mreq(c_params);
    return mem_req;
}

/* on_recv() for 'baseline'. */
EosStatus eos_pims_baseline_on_recv(EosPimsObservation obs, EosPimsCommonParams* c_params, EosPimsBaselineParams* params, EosPimsBaselineState* state, EosPimsDetection* result){

    (void) params;

    /* Initialize result. */
    result -> timestamp = obs.timestamp;
    result -> event = EOS_PIMS_NO_TRANSITION;
    result -> score = 0.;

    /* To store the status of sub-operations. */
    EosStatus status;

    /* Compare L2-distance with last observation. */
    if(!queue_empty(state -> queue)){

        /* Check consistency of bin definitions. */
        if(!check_bin_definitions(state -> last_smoothed_observation, obs)){
            return EOS_PIMS_BINS_MISMATCH_ERROR;
        }

        /* Allocate space for a copy of the current observation, to smooth soon. */
        EosMemoryBuffer* smoothed_bin_counts_buf;
        status = lifo_allocate_buffer_checked(&smoothed_bin_counts_buf,
                                              sizeof(pims_count_t) * obs.num_bins,
                                              "smoothed bin counts buffer");
        if(status != EOS_SUCCESS){
            return status;
        }
        pims_count_t* smoothed_bin_counts = (pims_count_t*) smoothed_bin_counts_buf -> ptr;
        EosPimsObservation curr_smoothed_observation = {
            .bin_counts = smoothed_bin_counts,
        };

        /* Make a deep-copy of the current observation, to smooth soon.
         * We need the unsmoothed observation to be kept in our queue. */
        status = deepcopy_observation(&obs, &curr_smoothed_observation);
        if(status != EOS_SUCCESS){
            return status;
        }

        /* Now, smooth the current observation out. */
        status = eos_pims_filter(c_params -> filter, curr_smoothed_observation, state -> queue);
        if(status != EOS_SUCCESS){
            return status;
        }

        /* _squared_l2_diff() will return EOS_PIMS_BINS_MISMATCH_ERROR if the bins don't match. */
        U64 squared_l2_diff_counts;
        status = _squared_l2_diff(state -> last_smoothed_observation, curr_smoothed_observation, &squared_l2_diff_counts);
        if(status != EOS_SUCCESS){
            return status;
        }

        /* Replace 'last_smoothed_observation' by the current smoothed observation. */
        status = deepcopy_observation(&curr_smoothed_observation, &(state -> last_smoothed_observation));
        if(status != EOS_SUCCESS){
            return status;
        }

        /* Assign score as the squared L2-difference. */
        result -> score = (F32) squared_l2_diff_counts;
     
        /* Mark as a transition if the score is above the threshold. */
        F32 squared_l2_diff_threshold = c_params -> threshold;
        if(result -> score >= squared_l2_diff_threshold){
            result -> event = EOS_PIMS_TRANSITION;
        }

        /* Deallocate memory for smoothed bin-counts. */
        status = lifo_deallocate_buffer(smoothed_bin_counts_buf);
        if(status != EOS_SUCCESS){
            return status;
        }

    } else {

        /* If this is the first observation, set this as the 'last_smoothed_observation' . */
        status = deepcopy_observation(&obs, &(state -> last_smoothed_observation));
        if(status != EOS_SUCCESS){
            return status;
        }
    }

    /* If we already have the maximum number of observations, delete the oldest. */
    if(queue_full(state -> queue)){
        status = queue_pop(&(state -> queue));
        if(status != EOS_SUCCESS){
            return status;
        }
    }

    /* Add current observation to queue of observations. */
    status = queue_push(&(state -> queue), obs);
    return status;
}

/* Computes the squared L2-difference between obs1 and obs2, placing the answer in result. */
EosStatus _squared_l2_diff(EosPimsObservation obs1, EosPimsObservation obs2, U64* result){

    /* Check if bin definitions are the same. */
    U32 bin_def_eq = check_bin_definitions(obs1, obs2);
    if(!bin_def_eq){
        return EOS_PIMS_BINS_MISMATCH_ERROR;
    }

    /* Loop over all bins. */
    U64 diff = 0;
    *result = 0;
    for(U32 bin = 0; bin < obs1.num_bins; ++bin){
        diff = (U64) eos_uabs_diff(obs1.bin_counts[bin], obs2.bin_counts[bin]);
        *result += diff * diff;
    }

    return EOS_SUCCESS;
}
