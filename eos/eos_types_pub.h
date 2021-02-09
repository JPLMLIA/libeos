#ifndef JPL_EOS_TYPES_PUB
#define JPL_EOS_TYPES_PUB

#include <stdint.h>

#define EOS_FALSE 0
#define EOS_TRUE 1

typedef enum {
    EOS_SUCCESS = 0,
    EOS_ERROR = 1,
    EOS_INSUFFICIENT_MEMORY = 2,
    EOS_NOT_INITIALIZED = 3,
    EOS_LOG_INITIALIZATION_FAILED = 4,
    EOS_LIFO_MEMORY_VIOLATION = 5,
    EOS_ASSERT_ERROR = 6,
    EOS_PARAM_ERROR = 7,
    EOS_VALUE_ERROR = 8,
    EOS_ETM_LOAD_ERROR = 9,
    EOS_ETM_VERSION_ERROR = 10,
    EOS_MISE_LOAD_ERROR = 11,
    EOS_MISE_VERSION_ERROR = 12,
    EOS_PIMS_LOAD_ERROR = 13,
    EOS_PIMS_VERSION_ERROR = 14,
    EOS_PIMS_NOT_INITIALIZED = 15,
    EOS_PIMS_BINS_MISMATCH_ERROR = 16,
    EOS_PIMS_QUEUE_EMPTY = 17,
    EOS_PIMS_QUEUE_FULL = 18,
} EosStatus;

/*
 * Enum for indicating severity for logging.
 */
typedef enum {
    EOS_LOG_DEBUG = 0,
    EOS_LOG_INFO = 1,
    EOS_LOG_WARN = 2,
    EOS_LOG_ERROR = 3,
    EOS_LOG_KEY_VALUE = 4,
} EosLogType;

/*
 * Generic structure to hold observation shape (rows, cols)
 */
typedef struct {
    uint32_t rows;
    uint32_t cols;
    uint32_t bands;
} EosObsShape;

/*
 * Enum for the E-THEMIS bands
 */
typedef enum {
    EOS_ETHEMIS_BAND_1 = 0,
    EOS_ETHEMIS_BAND_2 = 1,
    EOS_ETHEMIS_BAND_3 = 2,
    EOS_ETHEMIS_N_BANDS = 3,
} EosEthemisBand;

/*
 * Data from an E-THEMIS observation
 */
typedef struct {
    uint32_t observation_id;
    uint32_t timestamp;
    EosObsShape band_shape[EOS_ETHEMIS_N_BANDS];
    uint16_t* band_data[EOS_ETHEMIS_N_BANDS];
} EosEthemisObservation;

/*
 * Parameters relevant to E-THEMIS detector
 */
typedef struct {
    uint16_t band_threshold[EOS_ETHEMIS_N_BANDS];
} EosEthemisParams;

/*
 * Enum for MISE algorithms
 */
typedef enum {
    EOS_MISE_RX = 0,
    EOS_MISE_N_ALGS = 1,
} EosMiseAlgorithm;

/*
 * Parameters relevant to MISE detector
 */
typedef struct {
    EosMiseAlgorithm alg;
} EosMiseParams;

/*
 * An individual per-pixel detection result (for E-THEMIS or MISE)
 */
typedef struct {
    uint32_t row;
    uint32_t col;
    double score;
} EosPixelDetection;

/*
 * A set of detection results for E-THEMIS
 */
typedef struct {
    uint32_t n_results[EOS_ETHEMIS_N_BANDS];
    EosPixelDetection* band_results[EOS_ETHEMIS_N_BANDS];
} EosEthemisDetectionResult;

/*
 * Data from a MISE observation
 */
#define EOS_MISE_N_BANDS 421
typedef struct {
    uint32_t observation_id;
    uint32_t timestamp;
    EosObsShape shape;   /* Assume same for all bands */
    uint16_t* data;      /* All data in BIP (row, col, band) format */
} EosMiseObservation;

/*
 * A set of detection results for MISE
 */
typedef struct {
    uint32_t n_results;
    EosPixelDetection* results;
} EosMiseDetectionResult;

/*
 * PIMS modes
 */
typedef enum {
    EOS_PIMS_TRANSITION_MODE = 0,
    EOS_PIMS_MAGNETOSPHERIC_MODE = 1,
    EOS_PIMS_IONOSPHERIC_MODE = 2,
} EosPimsMode;

/*
 * PIMS mode information
 */
typedef struct {
    uint16_t num_bins;
    float* bin_log_energies;
} EosPimsModeInfo;

/* Do we want to store PIMS counts as U16? */
#ifdef EOS_PIMS_U16_DATA
    typedef uint16_t pims_count_t;
    static const uint32_t PIMS_COUNT_T_MAX = UINT16_MAX;
#else
    typedef uint32_t pims_count_t;
    static const uint32_t PIMS_COUNT_T_MAX = UINT32_MAX;
#endif

/*
 * Data from a PIMS observation
 */
typedef struct {
    uint32_t observation_id;
    uint32_t timestamp;
    uint32_t num_bins;        /* Number of bins. */
    EosPimsMode mode;         /* PIMS mode under which this observation was taken. */
    pims_count_t* bin_counts; /* Counts for each bin. */
    float* bin_log_energies;  /* Log of each bin's center energy. */
} EosPimsObservation;

/* 
 * To hold multiple PIMS observations, from a file
 */
typedef struct {
    uint32_t file_id;
    uint32_t num_modes;
    uint32_t max_bins;
    uint32_t num_observations;
    EosPimsModeInfo* modes_info;
    EosPimsObservation* observations;
} EosPimsObservationsFile;

/*
 * To hold multiple PIMS observations, in a queue
 */
typedef struct {
    EosPimsObservation* observations;
    uint32_t max_size;
    uint32_t head;
    uint64_t tail;
} EosPimsObservationQueue;

/*
 * State for PIMS algorithms
 */
typedef struct {
    EosPimsObservation last_smoothed_observation;
    EosPimsObservationQueue queue;
} EosPimsBaselineState;

typedef struct {
    EosPimsBaselineState baseline_state;
} EosPimsAlgorithmState;

/*
 * Size of state for each algorithm, requested from the simulator
 */ 
typedef struct {
    uint32_t queue_size;
    uint32_t max_bins;
} EosPimsBaselineStateRequest;

typedef struct {
    EosPimsBaselineStateRequest baseline_req;
} EosPimsAlgorithmStateRequest;

/*
 * Enum to indicate choice of algorithm for PIMS anomaly detection.
 */
typedef enum {
    EOS_PIMS_NO_ALGORITHM = 0,
    EOS_PIMS_BASELINE = 1,
} EosPimsAlgorithm;

/*
 * Enum to indicate choice of algorithm for PIMS anomaly detection.
 */
typedef enum {
    EOS_PIMS_NO_FILTER = 0,
    EOS_PIMS_MIN_FILTER = 1,
    EOS_PIMS_MEAN_FILTER = 2,
    EOS_PIMS_MEDIAN_FILTER = 3,
    EOS_PIMS_MAX_FILTER = 4,
} EosPimsFilter;

/*
 * Enum to indicate whether an event has occurred or not
 */
typedef enum {
    EOS_PIMS_NO_TRANSITION = 0,
    EOS_PIMS_TRANSITION = 1,
} EosPimsEvent;

/*
 * Parameters for PIMS algorithms
 */
/* 'baseline' has no parameters, but we can't have an empty struct. */
typedef struct {
    char _padding;
} EosPimsBaselineParams;

typedef struct {
    EosPimsFilter filter;
    uint32_t max_observations;
    float threshold;
    uint32_t max_bins;
} EosPimsCommonParams;

typedef struct {
    EosPimsBaselineParams baseline_params;
    EosPimsCommonParams common_params;
} EosPimsAlgorithmParams;

/*
 * An individual PIMS detection result
 */
typedef struct {
    EosPimsEvent event;
    uint32_t timestamp;
    float score;
} EosPimsDetection;

/*
 * Parameters relevant to PIMS detector
 */
typedef struct {
    EosPimsAlgorithm alg;
    EosPimsAlgorithmParams params;
} EosPimsParams;

/*
 * All parameter sets
 */
typedef struct {
    EosEthemisParams ethemis;
    EosMiseParams mise;
    EosPimsParams pims;
} EosParams;


/*
 * Passed to eos_init().
 */
typedef struct {
    EosPimsParams pims_params;
    uint32_t mise_max_bands;
} EosInitParams;

#endif
