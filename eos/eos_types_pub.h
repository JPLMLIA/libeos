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
    EOS_ETM_LOAD_ERROR = 8,
    EOS_ETM_VERSION_ERROR = 9,
    EOS_MISE_LOAD_ERROR = 10,
    EOS_MISE_VERSION_ERROR = 11,
    EOS_VALUE_ERROR = 12,
} EosStatus;

typedef struct {
    uint32_t mise_max_bands;
} EosInitParams;

typedef enum {
    EOS_LOG_DEBUG,
    EOS_LOG_INFO,
    EOS_LOG_WARN,
    EOS_LOG_ERROR,
    EOS_LOG_KEY_VALUE,
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
 * All parameter sets
 */
typedef struct {
    EosEthemisParams ethemis;
    EosMiseParams mise;
} EosParams;

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

#endif
