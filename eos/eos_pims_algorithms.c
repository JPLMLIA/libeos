#include "eos_pims_algorithms.h"
#include "eos_pims_baseline.h"
#include "eos_pims_helpers.h"
#include "eos_log.h"

static EosPimsAlgorithm current_algorithm = EOS_PIMS_NO_ALGORITHM;

/* state_request() router. */
EosStatus eos_pims_alg_state_request(EosPimsAlgorithm algorithm, EosPimsAlgorithmParams* params, EosPimsAlgorithmStateRequest* req){

    /* Call the corresponding algorithm's state_request(). */
    switch(algorithm){

        case EOS_PIMS_BASELINE:
            return eos_pims_baseline_state_request(&(params -> common_params), &(params -> baseline_params), &(req -> baseline_req));

        default:
            eos_log(EOS_LOG_ERROR, "Invalid PIMS algorithm specified.");
            return EOS_VALUE_ERROR;
    }
}

/* init_mreq() router. */
U64 eos_pims_alg_init_mreq(EosPimsAlgorithm algorithm, const EosPimsAlgorithmParams* params){

    /* Call the corresponding algorithm's init(). */
    switch(algorithm){

        case EOS_PIMS_BASELINE:
            return eos_pims_baseline_init_mreq(&(params -> common_params), &(params -> baseline_params));

        default:
            return 0;
    }
}

/* init() router. */
EosStatus eos_pims_alg_init(EosPimsAlgorithm algorithm, EosPimsAlgorithmParams* params, EosPimsAlgorithmState* state){

    /* Set as current algorithm. */
    current_algorithm = algorithm;

    /* Call the corresponding algorithm's init_mreq(). */
    switch(current_algorithm){

        case EOS_PIMS_BASELINE: 
            return eos_pims_baseline_init(&(params -> common_params), &(params -> baseline_params), &(state -> baseline_state));
        
        default:
            eos_log(EOS_LOG_ERROR, "Invalid PIMS algorithm specified.");
            return EOS_VALUE_ERROR;
    }
}

/* on_recv_mreq() router. */
U64 eos_pims_alg_on_recv_mreq(EosPimsAlgorithm algorithm, const EosPimsAlgorithmParams* params){

    /* Call the corresponding algorithm's on_recv_mreq(). */
    switch(algorithm){

        case EOS_PIMS_BASELINE:
            return eos_pims_baseline_on_recv_mreq(&(params -> common_params), &(params -> baseline_params));

        default:
            return 0;
    }
}


/* on_recv() router. */
EosStatus eos_pims_alg_on_recv(EosPimsObservation obs, EosPimsAlgorithmParams* params, EosPimsAlgorithmState* state, EosPimsDetection* result){

    /* Check if we have called init(). */
    EosStatus status = eos_pims_verify_initialization();
    if(status != EOS_SUCCESS){
        return status;
    }

    /* Call the corresponding algorithm's on_recv(). */
    switch(current_algorithm){

        case EOS_PIMS_BASELINE:
            return eos_pims_baseline_on_recv(obs, &(params -> common_params), &(params -> baseline_params), &(state -> baseline_state), result);

        default:
            eos_log(EOS_LOG_ERROR, "Invalid PIMS algorithm specified.");
            return EOS_VALUE_ERROR;
    }
}

/* Unselects the selected algorithm. */
EosStatus eos_pims_teardown(){

    /* Unset algorithm. */
    current_algorithm = EOS_PIMS_NO_ALGORITHM;
    return EOS_SUCCESS;
}

/* Checks if init() has been called. */
EosStatus eos_pims_verify_initialization(){
    if(current_algorithm == EOS_PIMS_NO_ALGORITHM){
        eos_log(EOS_LOG_ERROR, "'current_algorithm' is not initialized.");
        return EOS_PIMS_NOT_INITIALIZED;
    }
    return EOS_SUCCESS;
}
