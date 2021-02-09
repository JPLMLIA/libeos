#include <stdlib.h>
#include <stdio.h>

#include <eos.h>
#include <eos_pims.h>
#include "../sim/sim_util.h"
#include "util.h"
#include "CuTest.h"

/* 
 * Tests start here!
 */

/* Check whether pims_count_t is of the right datatype. */
void TestPimsCountsDatatype(CuTest* ct){
    #ifdef EOS_PIMS_U16_DATA
        /* 16 bits is 2 bytes. */
        CuAssertIntEquals(ct, 2, sizeof(pims_count_t));
    #else
        /* 32 bits is 4 bytes. */
        CuAssertIntEquals(ct, 4, sizeof(pims_count_t));
    #endif
}

/* Simple check on init() and on_recv(). */
void TestPimsInterface(CuTest* ct){
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_NO_FILTER,
            .max_observations = 1,
            .threshold = 0.,
            .max_bins = 30.,
        }
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    EosPimsObservation obs = InitPimsObs(30, 0);
    EosPimsDetection result;

    status = eos_pims_alg_on_recv(obs, &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    FreePimsObs(obs);

    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Checks init() under valid and invalid choices of 'algorithm'. */
void TestPimsInit(CuTest *ct) {
    EosStatus status;
    EosPimsAlgorithmParams params;
    EosPimsAlgorithmState state;

    status = eos_pims_verify_initialization();
    CuAssertIntEquals(ct, EOS_PIMS_NOT_INITIALIZED, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_verify_initialization();
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_NO_ALGORITHM, &params, &state);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    eos_pims_teardown();
}

/* Checks returned outputs for threshold 0 from on_recv() for 'baseline' after init(). */
void TestPimsBaselineOnRecvThres0(CuTest *ct) {
    EosStatus status;
    EosPimsAlgorithmParams params;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;
    EosPimsObservation last_stored_obs;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(31, 2), // Wrong number of bins: 31 vs 30! Should be caught later!
        InitPimsObs(30, 3),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    /* Set threshold as 0. */
    params.common_params.threshold = 0.;
    params.common_params.max_bins = 30;
    params.common_params.filter = EOS_PIMS_NO_FILTER;
    params.common_params.max_observations = num_observations;

    /* Allocate space for state, using the StateRequest feature. */
    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    /* init(). */
    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0, state.baseline_state.queue.head);
    CuAssertIntEquals(ct, 0, state.baseline_state.queue.tail);

    /* on_recv() for the observations defined above. */
    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0, state.baseline_state.queue.head);
    CuAssertIntEquals(ct, 1, state.baseline_state.queue.tail);
    CuAssertIntEquals(ct, EOS_PIMS_NO_TRANSITION, result.event);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[0]));

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0, state.baseline_state.queue.head);
    CuAssertIntEquals(ct, 2, state.baseline_state.queue.tail);
    CuAssertIntEquals(ct, EOS_PIMS_TRANSITION, result.event);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[1]));

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_PIMS_BINS_MISMATCH_ERROR, status);
    CuAssertIntEquals(ct, EOS_PIMS_NO_TRANSITION, result.event);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[1]));

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, EOS_PIMS_TRANSITION, result.event);
    CuAssertDblEquals(ct, 120., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[3]));

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Checks returned outputs for threshold 60 from on_recv() for 'baseline' after init(). */
void TestPimsBaselineOnRecvThres60(CuTest *ct) {
    EosStatus status;
    EosPimsAlgorithmParams params;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;
    EosPimsObservation last_stored_obs;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(31, 2), // Wrong number of bins: 31 vs 30! Should be caught later!
        InitPimsObs(30, 3),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    /* Set threshold as 60. */
    params.common_params.threshold = 60.;
    params.common_params.max_bins = 30;
    params.common_params.filter = EOS_PIMS_NO_FILTER;
    params.common_params.max_observations = num_observations;

    /* Allocate space for state, using the StateRequest feature. */
    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    /* init(). */
    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0, state.baseline_state.queue.head);
    CuAssertIntEquals(ct, 0, state.baseline_state.queue.tail);
    
    /* on_recv() for the observations defined above. */
    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, EOS_PIMS_NO_TRANSITION, result.event);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[0]));

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, EOS_PIMS_NO_TRANSITION, result.event);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[1]));

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_PIMS_BINS_MISMATCH_ERROR, status);
    CuAssertIntEquals(ct, EOS_PIMS_NO_TRANSITION, result.event);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[1]));

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, EOS_PIMS_TRANSITION, result.event);
    CuAssertDblEquals(ct, 120., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[3]));

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Checks returned outputs for threshold 200 from on_recv() for 'baseline' after init(). */
void TestPimsBaselineOnRecvThres200(CuTest *ct) {
    EosStatus status;
    EosPimsAlgorithmParams params;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;
    EosPimsObservation last_stored_obs;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(31, 2), // Wrong number of bins: 31 vs 30! Should be caught later!
        InitPimsObs(30, 3),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    /* Set threshold as 200. */
    params.common_params.threshold = 200.;
    params.common_params.max_bins = 30;
    params.common_params.filter = EOS_PIMS_NO_FILTER;
    params.common_params.max_observations = num_observations;

    /* Allocate space for state, using the StateRequest feature. */
    params.common_params.max_bins = 30;
    params.common_params.max_observations = num_observations;
    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    /* init(). */
    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 0, state.baseline_state.queue.head);
    CuAssertIntEquals(ct, 0, state.baseline_state.queue.tail);

    /* on_recv() for the observations defined above. */
    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, EOS_PIMS_NO_TRANSITION, result.event);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[0]));

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, EOS_PIMS_NO_TRANSITION, result.event);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[1]));

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_PIMS_BINS_MISMATCH_ERROR, status);
    CuAssertIntEquals(ct, EOS_PIMS_NO_TRANSITION, result.event);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[1]));

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, EOS_PIMS_NO_TRANSITION, result.event);
    CuAssertDblEquals(ct, 120., result.score, 1e-6);
    queue_tail(state.baseline_state.queue, &last_stored_obs);
    CuAssertTrue(ct, check_equality(last_stored_obs, obs[3]));

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Checks the case when on_recv() is called before init(). */
void TestPimsOnRecvBeforeInit(CuTest *ct){
    EosStatus status;
    EosPimsObservation obs = {0};
    EosPimsAlgorithmParams params;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    status = eos_pims_verify_initialization();
    CuAssertIntEquals(ct, EOS_PIMS_NOT_INITIALIZED, status);

    status = eos_pims_alg_on_recv(obs, &params, &state, &result);
    CuAssertIntEquals(ct, EOS_PIMS_NOT_INITIALIZED, status);
}

/* Checks the StateRequest facility for 'baseline'. */
void TestPimsBaselineStateRequest(CuTest *ct){
    EosStatus status;
    EosPimsAlgorithmState state = {{{0}, {0}}};
    EosPimsAlgorithmStateRequest req;

    /* Check zero observations case. */
    req.baseline_req.queue_size = 0;
    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    /* Check non-zero observations case. */
    req.baseline_req.queue_size = 10;
    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 10, state.baseline_state.queue.max_size);
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    
    /* Check non-zero observations case, again. */
    req.baseline_req.queue_size = 100;
    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertIntEquals(ct, 100, state.baseline_state.queue.max_size);
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the min-filter (size 1) for increasing PIMS observations. */
void TestPimsBaselineMinFilterSize1Increasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MIN_FILTER,
            .max_observations = 1,
            .threshold = 0.,
            .max_bins = 30.,
        }
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(30, 2),
        InitPimsObs(30, 3),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the min-filter (size 1) for decreasing PIMS observations. */
void TestPimsBaselineMinFilterSize1Decreasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MIN_FILTER,
            .max_observations = 1,
            .threshold = 0.,
            .max_bins = 30.,
        }
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 3),
        InitPimsObs(30, 2),
        InitPimsObs(30, 1),
        InitPimsObs(30, 0),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the min-filter (size 3) for increasing PIMS observations. */
void TestPimsBaselineMinFilterSize3Increasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MIN_FILTER,
            .max_observations = 3,
            .threshold = 0.,
            .max_bins = 30.,
        }
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(30, 2),
        InitPimsObs(30, 3),
        InitPimsObs(30, 4),
        InitPimsObs(30, 5),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[4], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[5], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the min-filter (size 3) for decreasing PIMS observations. */
void TestPimsBaselineMinFilterSize3Decreasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MIN_FILTER,
            .max_observations = 3,
            .threshold = 0.,
            .max_bins = 30.,
        }
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 5),
        InitPimsObs(30, 4),
        InitPimsObs(30, 3),
        InitPimsObs(30, 2),
        InitPimsObs(30, 1),
        InitPimsObs(30, 0),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[4], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[5], &params, &state, &result);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the mean-filter (size 1) for increasing PIMS observations. */
void TestPimsBaselineMeanFilterSize1Increasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MEAN_FILTER,
            .max_observations = 1,
            .threshold = 0.,
            .max_bins = 30.,
        }
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(30, 2),
        InitPimsObs(30, 3),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the mean-filter (size 1) for decreasing PIMS observations. */
void TestPimsBaselineMeanFilterSize1Decreasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MEAN_FILTER,
            .max_observations = 1,
            .threshold = 0.,
            .max_bins = 30.,
        }
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 3),
        InitPimsObs(30, 2),
        InitPimsObs(30, 1),
        InitPimsObs(30, 0),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the mean-filter (size 3) for increasing PIMS observations. */
void TestPimsBaselineMeanFilterSize3Increasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MEAN_FILTER,
            .max_observations = 3,
            .threshold = 0.,
            .max_bins = 30.,
        }
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(30, 2),
        InitPimsObs(30, 3),
        InitPimsObs(30, 4),
        InitPimsObs(30, 5),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[4], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[5], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the mean-filter (size 3) for decreasing PIMS observations. */
void TestPimsBaselineMeanFilterSize3Decreasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MEAN_FILTER,
            .max_observations = 3,
            .threshold = 0.,
            .max_bins = 30.,
        }
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 5),
        InitPimsObs(30, 4),
        InitPimsObs(30, 3),
        InitPimsObs(30, 2),
        InitPimsObs(30, 1),
        InitPimsObs(30, 0),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 5, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 4, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 4, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[4], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[5], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the median-filter (size 1) for increasing PIMS observations. */
void TestPimsBaselineMedianFilterSize1Increasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MEDIAN_FILTER,
            .max_observations = 1,
            .threshold = 0.,
            .max_bins = 30.,
        },
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(30, 2),
        InitPimsObs(30, 3),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the median-filter (size 1) for decreasing PIMS observations. */
void TestPimsBaselineMedianFilterSize1Decreasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MEDIAN_FILTER,
            .max_observations = 1,
            .threshold = 0.,
            .max_bins = 30.,
        },
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 3),
        InitPimsObs(30, 2),
        InitPimsObs(30, 1),
        InitPimsObs(30, 0),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the median-filter (size 3) for increasing PIMS observations. */
void TestPimsBaselineMedianFilterSize3Increasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MEDIAN_FILTER,
            .max_observations = 3,
            .threshold = 0.,
            .max_bins = 30.,
        },
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    init_params.pims_params.params = params;
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(30, 2),
        InitPimsObs(30, 3),
        InitPimsObs(30, 4),
        InitPimsObs(30, 5),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[4], &params, &state, &result);
    CuAssertIntEquals(ct, 2., state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[5], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the median-filter (size 3) for decreasing PIMS observations. */
void TestPimsBaselineMedianFilterSize3Decreasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MEDIAN_FILTER,
            .max_observations = 3,
            .threshold = 0.,
            .max_bins = 30.,
        },
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    init_params.pims_params.params = params;
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 5),
        InitPimsObs(30, 4),
        InitPimsObs(30, 3),
        InitPimsObs(30, 2),
        InitPimsObs(30, 1),
        InitPimsObs(30, 0),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 5, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 4, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 4, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[4], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[5], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the max-filter (size 1) for increasing PIMS observations. */
void TestPimsBaselineMaxFilterSize1Increasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MAX_FILTER,
            .max_observations = 1,
            .threshold = 0.,
            .max_bins = 30.,
        },
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(30, 2),
        InitPimsObs(30, 3),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the max-filter (size 1) for decreasing PIMS observations. */
void TestPimsBaselineMaxFilterSize1Decreasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MAX_FILTER,
            .max_observations = 1,
            .threshold = 0.,
            .max_bins = 30.,
        },
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 3),
        InitPimsObs(30, 2),
        InitPimsObs(30, 1),
        InitPimsObs(30, 0),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the max-filter (size 3) for increasing PIMS observations. */
void TestPimsBaselineMaxFilterSize3Increasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MAX_FILTER,
            .max_observations = 3,
            .threshold = 0.,
            .max_bins = 30.,
        },
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 0),
        InitPimsObs(30, 1),
        InitPimsObs(30, 2),
        InitPimsObs(30, 3),
        InitPimsObs(30, 4),
        InitPimsObs(30, 5),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 0, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 1, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 2, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[4], &params, &state, &result);
    CuAssertIntEquals(ct, 4, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[5], &params, &state, &result);
    CuAssertIntEquals(ct, 5, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Tests the max-filter (size 3) for decreasing PIMS observations. */
void TestPimsBaselineMaxFilterSize3Decreasing(CuTest *ct) {
    EosPimsAlgorithmParams params = {
        .common_params = {
            .filter = EOS_PIMS_MAX_FILTER,
            .max_observations = 3,
            .threshold = 0.,
            .max_bins = 30.,
        },
    };

    EosStatus status;
    EosPimsAlgorithmStateRequest req;
    EosPimsAlgorithmState state;
    EosPimsDetection result;

    EosInitParams init_params;
    status = default_init_params_pims(&init_params);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    eos_init(&init_params, NULL, 0, NULL);

    /* Fake observations from PIMS. */
    EosPimsObservation obs[] = {
        InitPimsObs(30, 5),
        InitPimsObs(30, 4),
        InitPimsObs(30, 3),
        InitPimsObs(30, 2),
        InitPimsObs(30, 1),
        InitPimsObs(30, 0),
    };
    U16 num_observations = sizeof(obs) / sizeof(obs[0]);

    status = eos_pims_alg_state_request(EOS_PIMS_BASELINE, &params, &req);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = sim_pims_handle_state_request(EOS_PIMS_BASELINE, &req, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_init(EOS_PIMS_BASELINE, &params, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_alg_on_recv(obs[0], &params, &state, &result);
    CuAssertIntEquals(ct, 5, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[1], &params, &state, &result);
    CuAssertIntEquals(ct, 5, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[2], &params, &state, &result);
    CuAssertIntEquals(ct, 5, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[3], &params, &state, &result);
    CuAssertIntEquals(ct, 5, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 0., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[4], &params, &state, &result);
    CuAssertIntEquals(ct, 4, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    status = eos_pims_alg_on_recv(obs[5], &params, &state, &result);
    CuAssertIntEquals(ct, 3, state.baseline_state.last_smoothed_observation.bin_counts[0]);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
    CuAssertDblEquals(ct, 30., result.score, 1e-6);

    /* Don't leak memory! */
    for(int i = 0; i < num_observations; ++i){
        FreePimsObs(obs[i]);
    }

    eos_teardown();
    eos_pims_teardown();
    status = sim_pims_handle_state_teardown(EOS_PIMS_BASELINE, &state);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);
}

/* Check when supplied with a bad value for 'filter'. */
void TestPimsBadFilter(CuTest *ct) {
    /* Fake observations from PIMS. */
    EosPimsObservation obs = InitPimsObs(30, 0);
    EosPimsObservationQueue q = {0};

    EosStatus status = eos_pims_filter(1000, obs, q);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    FreePimsObs(obs);
}

/* Checks the min-filter when supplied with a bad input. */
void TestPimsMinFilterBadInput(CuTest *ct) {
    
    /* Fake 'good' observations from PIMS. */
    EosPimsObservation obs_good = InitPimsObs(30, 0);
    EosPimsObservation good_obs[2] = {
        InitPimsObs(30, 1), InitPimsObs(30, 2),
    };
    EosPimsObservationQueue q_good = {
        .observations = good_obs,
    };
    queue_init(&q_good);
    queue_push(&q_good, good_obs[0]);
    queue_push(&q_good, good_obs[1]);

    /* Different kinds of 'bad' inputs. */
    EosPimsObservation obs_bad = {0};
    EosPimsObservationQueue q_bad = {0};
    EosPimsObservation null_obs[3] = {{0}, {0}, {0}};
    EosPimsObservationQueue q_null = {
        .observations = null_obs,
        .max_size = 2,
    };
    queue_init(&q_null);
    queue_push(&q_null, null_obs[0]);
    queue_push(&q_null, null_obs[1]);
    EosPimsObservation mismatch_obs[3] = {
        InitPimsObs(31, 1), InitPimsObs(30, 2), {0},
    };
    EosPimsObservationQueue q_mismatch = {
        .observations = mismatch_obs,
        .max_size = 2,
    };
    queue_init(&q_mismatch);
    queue_push(&q_mismatch, mismatch_obs[0]);
    queue_push(&q_mismatch, mismatch_obs[1]);

    EosStatus status;
    status = eos_pims_filter(EOS_PIMS_MIN_FILTER, obs_good, q_good);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_filter(EOS_PIMS_MIN_FILTER, obs_bad, q_good);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MIN_FILTER, obs_good, q_bad);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MIN_FILTER, obs_bad, q_bad);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MIN_FILTER, obs_good, q_null);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MIN_FILTER, obs_bad, q_null);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MIN_FILTER, obs_good, q_mismatch);
    CuAssertIntEquals(ct, EOS_PIMS_BINS_MISMATCH_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MIN_FILTER, obs_bad, q_mismatch);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    FreePimsObs(obs_good);
    FreePimsObs(good_obs[0]);
    FreePimsObs(good_obs[1]);
    FreePimsObs(mismatch_obs[0]);
    FreePimsObs(mismatch_obs[1]);
}

/* Checks the mean-filter when supplied with a bad input. */
void TestPimsMeanFilterBadInput(CuTest *ct) {
    
    /* Fake 'good' observations from PIMS. */
    EosPimsObservation obs_good = InitPimsObs(30, 0);
    EosPimsObservation good_obs[2] = {
        InitPimsObs(30, 1), InitPimsObs(30, 2),
    };
    EosPimsObservationQueue q_good = {
        .observations = good_obs,
    };
    queue_init(&q_good);
    queue_push(&q_good, good_obs[0]);
    queue_push(&q_good, good_obs[1]);

    /* Different kinds of 'bad' inputs. */
    EosPimsObservation obs_bad = {0};
    EosPimsObservationQueue q_bad = {0};
    EosPimsObservation null_obs[3] = {{0}, {0}, {0}};
    EosPimsObservationQueue q_null = {
        .observations = null_obs,
        .max_size = 2,
    };
    queue_init(&q_null);
    queue_push(&q_null, null_obs[0]);
    queue_push(&q_null, null_obs[1]);
    EosPimsObservation mismatch_obs[3] = {
        InitPimsObs(31, 1), InitPimsObs(30, 2), {0},
    };
    EosPimsObservationQueue q_mismatch = {
        .observations = mismatch_obs,
        .max_size = 2,
    };
    queue_init(&q_mismatch);
    queue_push(&q_mismatch, mismatch_obs[0]);
    queue_push(&q_mismatch, mismatch_obs[1]);

    /* Since the mean filter needs memory from the EOS stack, we initialize. */
    EosInitParams init_params;
    default_init_params_pims(&init_params);
    init_params.pims_params.params.common_params.filter = EOS_PIMS_MEAN_FILTER;
    init_params.pims_params.params.common_params.max_bins = 31;
    eos_init(&init_params, NULL, 0, NULL);

    EosStatus status;
    status = eos_pims_filter(EOS_PIMS_MEAN_FILTER, obs_good, q_good);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_filter(EOS_PIMS_MEAN_FILTER, obs_bad, q_good);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEAN_FILTER, obs_good, q_bad);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEAN_FILTER, obs_bad, q_bad);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEAN_FILTER, obs_good, q_null);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEAN_FILTER, obs_bad, q_null);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEAN_FILTER, obs_good, q_mismatch);
    CuAssertIntEquals(ct, EOS_PIMS_BINS_MISMATCH_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEAN_FILTER, obs_bad, q_mismatch);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    FreePimsObs(obs_good);
    FreePimsObs(good_obs[0]);
    FreePimsObs(good_obs[1]);
    FreePimsObs(mismatch_obs[0]);
    FreePimsObs(mismatch_obs[1]);
    eos_teardown();
}

/* Checks the median-filter when supplied with a bad input. */
void TestPimsMedianFilterBadInput(CuTest *ct) {
    
    /* Fake 'good' observations from PIMS. */
    EosPimsObservation obs_good = InitPimsObs(30, 0);
    EosPimsObservation good_obs[2] = {
        InitPimsObs(30, 1), InitPimsObs(30, 2),
    };
    EosPimsObservationQueue q_good = {
        .observations = good_obs,
    };
    queue_init(&q_good);
    queue_push(&q_good, good_obs[0]);
    queue_push(&q_good, good_obs[1]);

    /* Different kinds of 'bad' inputs. */
    EosPimsObservation obs_bad = {0};
    EosPimsObservationQueue q_bad = {0};
    EosPimsObservation null_obs[3] = {{0}, {0}, {0}};
    EosPimsObservationQueue q_null = {
        .observations = null_obs,
        .max_size = 2,
    };
    queue_init(&q_null);
    queue_push(&q_null, null_obs[0]);
    queue_push(&q_null, null_obs[1]);
    EosPimsObservation mismatch_obs[3] = {
        InitPimsObs(31, 1), InitPimsObs(30, 2), {0},
    };
    EosPimsObservationQueue q_mismatch = {
        .observations = mismatch_obs,
        .max_size = 2,
    };
    queue_init(&q_mismatch);
    queue_push(&q_mismatch, mismatch_obs[0]);
    queue_push(&q_mismatch, mismatch_obs[1]);

    /* Since the median filter needs memory from the EOS stack, we initialize. */
    EosInitParams init_params;
    default_init_params_pims(&init_params);
    init_params.pims_params.params.common_params.filter = EOS_PIMS_MEDIAN_FILTER;
    init_params.pims_params.params.common_params.max_bins = 31;
    init_params.pims_params.params.common_params.max_observations = 2;
    eos_init(&init_params, NULL, 0, NULL);

    EosStatus status;
    status = eos_pims_filter(EOS_PIMS_MEDIAN_FILTER, obs_good, q_good);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_filter(EOS_PIMS_MEDIAN_FILTER, obs_bad, q_good);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEDIAN_FILTER, obs_good, q_bad);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEDIAN_FILTER, obs_bad, q_bad);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEDIAN_FILTER, obs_good, q_null);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEDIAN_FILTER, obs_bad, q_null);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEDIAN_FILTER, obs_good, q_mismatch);
    CuAssertIntEquals(ct, EOS_PIMS_BINS_MISMATCH_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MEDIAN_FILTER, obs_bad, q_mismatch);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    FreePimsObs(obs_good);
    FreePimsObs(good_obs[0]);
    FreePimsObs(good_obs[1]);
    FreePimsObs(mismatch_obs[0]);
    FreePimsObs(mismatch_obs[1]);
    eos_teardown();
}

/* Checks the max-filter when supplied with a bad input. */
void TestPimsMaxFilterBadInput(CuTest *ct) {
    
    /* Fake 'good' observations from PIMS. */
    EosPimsObservation obs_good = InitPimsObs(30, 0);
    EosPimsObservation good_obs[2] = {
        InitPimsObs(30, 1), InitPimsObs(30, 2),
    };
    EosPimsObservationQueue q_good = {
        .observations = good_obs,
    };
    queue_init(&q_good);
    queue_push(&q_good, good_obs[0]);
    queue_push(&q_good, good_obs[1]);

    /* Different kinds of 'bad' inputs. */
    EosPimsObservation obs_bad = {0};
    EosPimsObservationQueue q_bad = {0};
    EosPimsObservation null_obs[3] = {{0}, {0}, {0}};
    EosPimsObservationQueue q_null = {
        .observations = null_obs,
        .max_size = 2,
    };
    queue_init(&q_null);
    queue_push(&q_null, null_obs[0]);
    queue_push(&q_null, null_obs[1]);
    EosPimsObservation mismatch_obs[3] = {
        InitPimsObs(31, 1), InitPimsObs(30, 2), {0},
    };
    EosPimsObservationQueue q_mismatch = {
        .observations = mismatch_obs,
        .max_size = 2,
    };
    queue_init(&q_mismatch);
    queue_push(&q_mismatch, mismatch_obs[0]);
    queue_push(&q_mismatch, mismatch_obs[1]);

    EosStatus status;
    status = eos_pims_filter(EOS_PIMS_MAX_FILTER, obs_good, q_good);
    CuAssertIntEquals(ct, EOS_SUCCESS, status);

    status = eos_pims_filter(EOS_PIMS_MAX_FILTER, obs_bad, q_good);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MAX_FILTER, obs_good, q_bad);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MAX_FILTER, obs_bad, q_bad);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MAX_FILTER, obs_good, q_null);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MAX_FILTER, obs_bad, q_null);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MAX_FILTER, obs_good, q_mismatch);
    CuAssertIntEquals(ct, EOS_PIMS_BINS_MISMATCH_ERROR, status);

    status = eos_pims_filter(EOS_PIMS_MAX_FILTER, obs_bad, q_mismatch);
    CuAssertIntEquals(ct, EOS_VALUE_ERROR, status);

    FreePimsObs(obs_good);
    FreePimsObs(good_obs[0]);
    FreePimsObs(good_obs[1]);
    FreePimsObs(mismatch_obs[0]);
    FreePimsObs(mismatch_obs[1]);
}

CuSuite* CuPimsGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestPimsCountsDatatype);
    SUITE_ADD_TEST(suite, TestPimsInterface);
    SUITE_ADD_TEST(suite, TestPimsInit);
    SUITE_ADD_TEST(suite, TestPimsOnRecvBeforeInit);
    SUITE_ADD_TEST(suite, TestPimsBaselineOnRecvThres0);
    SUITE_ADD_TEST(suite, TestPimsBaselineOnRecvThres60);
    SUITE_ADD_TEST(suite, TestPimsBaselineOnRecvThres200);
    SUITE_ADD_TEST(suite, TestPimsBaselineStateRequest);
    SUITE_ADD_TEST(suite, TestPimsBaselineMinFilterSize1Increasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMinFilterSize1Decreasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMinFilterSize3Increasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMinFilterSize3Decreasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMeanFilterSize1Increasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMeanFilterSize1Decreasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMeanFilterSize3Increasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMeanFilterSize3Decreasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMedianFilterSize1Increasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMedianFilterSize1Decreasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMedianFilterSize3Increasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMedianFilterSize3Decreasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMaxFilterSize1Increasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMaxFilterSize1Decreasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMaxFilterSize3Increasing);
    SUITE_ADD_TEST(suite, TestPimsBaselineMaxFilterSize3Decreasing);
    SUITE_ADD_TEST(suite, TestPimsBadFilter);
    SUITE_ADD_TEST(suite, TestPimsMinFilterBadInput);
    SUITE_ADD_TEST(suite, TestPimsMeanFilterBadInput);
    SUITE_ADD_TEST(suite, TestPimsMedianFilterBadInput);
    SUITE_ADD_TEST(suite, TestPimsMaxFilterBadInput);
    return suite;
}
