#include <stdlib.h>
#include <math.h>
#include <float.h>  /* for DBL_EPSILON */
#include <string.h> /* for memset() */

#include "eos_mise.h"
#include "eos_heap.h"
#include "eos_memory.h"
#include "eos_types.h"
#include "eos_util.h"
#include "eos_log.h"

/*
 * Given a BIP array of U16 data, compute the mean pixel and store in mp.
 * (If the input observation has zero size, the mean will contain all zeros.)
 *
 * :param data: pixel data in BIP format
 * :param shape: pointer to observation shape struct
 * :param mp: array for storing pixel mean; must have pre-allocated space for
 *            at least shape->bands values
 *
 * :return: status indicating whether an error occurred
 */
EosStatus compute_mean_pixel(const U16* data, const EosObsShape* shape,
                             F64 mp[]) {

    U32 i, b;

    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(shape != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(mp != NULL)) { return EOS_ASSERT_ERROR; }

    const U32 n_pixels = shape->rows * shape->cols;

    /* Initialize to zero */
    memset(mp, 0, sizeof(F64) * shape->bands);

    if (n_pixels == 0) {
        // If observation is empty, return zero mean
        return EOS_SUCCESS;
    }

    for (i = 0; i < n_pixels; i++) {
        const U16* next_pixel = &(data[i * shape->bands]); /* BIP format */
        for (b = 0; b < shape->bands; b++) {
            mp[b] += next_pixel[b];
        }
    }
    for (b = 0; b < shape->bands; b++) {
        mp[b] /= n_pixels;
    }

    return EOS_SUCCESS;
}

/*
 * Given a BIP array of U16 data and its mean pixel, compute the sample
 * covariance matrix (with DOF=N-1) and store in cov.
 *
 * :param data: pixel data in BIP format
 * :param shape: pointer to observation shape struct
 * :param mean_pixel: array containing the mean pixel value for each band
 * :param cov: destination of the covariance matrix
 *
 * :return: status indicating whether an error occurred
 */
EosStatus compute_covariance(const U16* data, const EosObsShape* shape,
                             F64 mean_pixel[], F64* cov) {

    U32 i, b1, b2;

    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(shape != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(mean_pixel != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(cov != NULL)) { return EOS_ASSERT_ERROR; }

    const U32 n_pixels = shape->rows * shape->cols;
    const U32 cov_size = shape->bands * shape->bands;
    F64 mean_sub[shape->bands];   /* mean-subtracted pixel */

    if (n_pixels <= 1) {
        // Sample size not large enough to compute covariance
        return EOS_VALUE_ERROR;
    }

    /* Initialize to zero */
    memset(cov, 0, sizeof(F64) * cov_size);

    for (i = 0; i < n_pixels; i++) {
        const U16* next_pixel = &(data[i * shape->bands]); /* BIP format */
        for (b1 = 0; b1 < shape->bands; b1++) {
            mean_sub[b1] = next_pixel[b1] - mean_pixel[b1];
        }
        /* cov = 1/(n-1) * sum_i (x_i - mean x) (x_i - mean x)'
         * where x_i is a vector of b band observations */
        for (b1 = 0; b1 < shape->bands; b1++) {
            for (b2 = 0; b2 < shape->bands; b2++) {
                cov[b1 * shape->bands + b2] += mean_sub[b1] * mean_sub[b2];
            }
        }
    }
    /* divide by n_pixels minus 1 */
    for (b1 = 0; b1 < cov_size; b1++) {
        cov[b1] /= (n_pixels-1);
    }

    return EOS_SUCCESS;
}

/***********************************************************
 * Methods to support matrix inversion were borrowed from
 * or inspired by VPT:
https://github-fn.jpl.nasa.gov/COSMIC/COSMIC_VPT/blob/bbae9e62af1582f4a780724f9198d4f222576a26/vpt/vpt_util.c
https://github-fn.jpl.nasa.gov/COSMIC/COSMIC_VPT/blob/ed73b4cc919d1953fe8c5563703d0bea08718d41/vpt/vpt_homography.c
*/
EosStatus _rotate_F64(F64 *a, F64 *b, F64 c, F64 d) {
    F64 a0;
    F64 b0;
    if (eos_assert(a != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(b != NULL)) { return EOS_ASSERT_ERROR; }
    b0 = *b;
    a0 = *a;
    *a = a0*c - b0*d;
    *b = a0*d + b0*c;
    return EOS_SUCCESS;
}

EosStatus _eigen_rotate(U32 n, F64* A, F64* V, U32 k, U32 l, F64 c, F64 s) {
    EosStatus status;
    U32 i;

    if (eos_assert(A != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(V != NULL)) { return EOS_ASSERT_ERROR; }

    // rotate rows and columns k and l
    for (i = 0; i < k; i++) {
        status = _rotate_F64(&A[n*i+k], &A[n*i+l], c, s);
        if (status != EOS_SUCCESS) { return status; }
    }
    for (i = k+1; i < l; i++) {
        status = _rotate_F64(&A[n*k+i], &A[n*i+l], c, s);
        if (status != EOS_SUCCESS) { return status; }
    }
    for (i = l+1; i < n; i++) {
        status = _rotate_F64(&A[n*k+i], &A[n*l+i], c, s);
        if (status != EOS_SUCCESS) { return status; }
    }

    // rotate eigenvectors
    for (i = 0; i < n; i++) {
        status = _rotate_F64(&V[n*k+i], &V[n*l+i], c, s);
        if (status != EOS_SUCCESS) { return status; }
    }
    return EOS_SUCCESS;
}

EosStatus _eigen_maxind(U32 n, F64* A, U32 idx, U32* row_index, U32* col_index) {
    U32 m;
    U32 i;
    F64 mv;
    F64 val;

    if (eos_assert(A != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(row_index != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(col_index != NULL)) { return EOS_ASSERT_ERROR; }

    if (idx < n - 1) {
        m = idx+1;
        mv = fabs(A[n*idx + m]);
        for (i = idx+2; i < n; i++) {
            val = fabs(A[n*idx+i]);
            if (mv < val) {
                mv = val, m = i;
            }
        }
        row_index[idx] = m;
    }
    if (idx > 0) {
        m = 0;
        mv = fabs(A[idx]);
        for (i = 1; i < idx; i++) {
            val = fabs(A[n*i+idx]);
            if (mv < val) {
                mv = val, m = i;
            }
        }
        col_index[idx] = m;
    }
    return EOS_SUCCESS;
}

EosStatus _eigen_pivot(F64 p, F64 y, F64* c, F64* s, F64* t) {
    if (eos_assert(c != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(s != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(t != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(p != 0)) { return EOS_ASSERT_ERROR; }
    *t = fabs(y) + hypot(p, y);
    *s = hypot(p, *t);
    *c = *t / *s;
    *s = p / *s;
    *t = (p / *t) *p;
    if (y < 0) {
        *s = -*s;
        *t = -*t;
    }
    return EOS_SUCCESS;
}

/*
 * Find eigenvalues/vectors of matrix A,
 * assuming A is symmetric and square.
 * Eigenvalues are stored in w and eigenvectors are in the rows of V.
 * The buf array should have size at least 2*n.
 */
EosStatus get_eigen_symm(U32 n, F64* A, F64* w, F64* V, U32* buf) {
    EosStatus status;
    U32 i, k, l;
    U32 iters;
    U32* row_index;
    U32* col_index;
    F64 mv;
    F64 val;
    F64 p, y, t, s, c;

    if (eos_assert(A != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(w != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(V != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(buf != NULL)) { return EOS_ASSERT_ERROR; }

    memset(w, 0, sizeof(F64)*n);
    memset(V, 0, sizeof(F64)*n*n);
    for (i = 0; i < n; i++) {
        V[i*n + i] = 1;
    }

    row_index = buf;
    col_index = row_index + n;

    for (k = 0; k < n; k++) {
        w[k] = A[(n + 1)*k]; /* diagonal */
        status = _eigen_maxind(n, A, k, row_index, col_index);
        if (status != EOS_SUCCESS) { return status; }
    }

    if (n <= 1) {
        return EOS_SUCCESS;
    }

    for (iters = 0; iters < n*n*30; iters++) {
        // find index (k,l) of pivot p
        k = 0;
        mv = fabs(A[row_index[0]]);
        for (i = 1; i < n-1; i++) {
            val = fabs(A[n*i + row_index[i]]);
            if (mv < val) {
                mv = val;
                k = i;
            }
        }
        l = row_index[k];
        for (i = 1; i < n; i++) {
            val = fabs(A[n*col_index[i] + i]);
            if (mv < val) {
                mv = val;
                k = col_index[i];
                l = i;
            }
        }

        p = A[n*k + l];
        if (fabs(p) <= DBL_EPSILON) { break; }
        y = 0.5*(w[l] - w[k]);

        status = _eigen_pivot(p, y, &c, &s, &t);
        if (status != EOS_SUCCESS) { return status; }

        A[n*k + l] = 0;

        w[k] -= t;
        w[l] += t;

        status = _eigen_rotate(n, A, V, k, l, c, s);
        if (status != EOS_SUCCESS) { return status; }

        status = _eigen_maxind(n, A, k, row_index, col_index);
        if (status != EOS_SUCCESS) { return status; }
        status = _eigen_maxind(n, A, l, row_index, col_index);
        if (status != EOS_SUCCESS) { return status; }
    }

    return EOS_SUCCESS;
}


/* This matrix inversion method was inspired by the VPT method
 * for inverting a symmetric matrix:
https://github-fn.jpl.nasa.gov/COSMIC/COSMIC_VPT/blob/ed73b4cc919d1953fe8c5563703d0bea08718d41/vpt/vpt_homography.c#L227-L266
*/
EosStatus invert_sym_matrix(U32 n, F64* A, F64* A_inv) {
    U32 i, j, k;
    F64 Ac[n*n];
    F64 V[n*n];
    F64 w[n];
    U32 iptr[2*n];
    F64 threshold;
    EosStatus status;

    if (eos_assert(A != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(A_inv != NULL)) { return EOS_ASSERT_ERROR; }

    /* Make a copy because get_eigen_symm is destructive */
    memcpy(Ac, A, sizeof(F64)*n*n);

    /* find eigenvalues of a symmetric matrix and populate
     * w (eigenvalues) and V (eigenvectors) */
    status = get_eigen_symm(n, Ac, w, V, iptr);
    if (status != EOS_SUCCESS) { return status; }

    threshold = 2*DBL_EPSILON*fabs(eos_dsum(n, w));
    memset(A_inv, 0, sizeof(F64)*n*n);

    // Ainv = v * inv(w) * vT
    for (i = 0; i < n; i++) {
        if (fabs(w[i]) <= threshold) {
            continue;
        }

        for (j = 0; j < n; j++) {
            for (k = 0; k < n; k++) {
                A_inv[j*n + k] += V[n*i + j] * V[i*n + k] / w[i];
            }
        }
    }

    return EOS_SUCCESS;

}
/***********************************************************/

/* Compute the RX score of the mean-subtracted observation
 * with respect to the (inverse) covariance matrix
 *    rx_score = np.dot(np.dot(sub, cov_inv), sub.T)
 */
EosStatus _rx_score(F64* mean_sub, F64* cov_inv, const EosObsShape shape,
        F64* temp, F64* score) {

    U32 b1, b2;
    if (eos_assert(mean_sub != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(cov_inv != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(temp != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(score != NULL)) { return EOS_ASSERT_ERROR; }

    // Initialize score to zero
    *score = 0.0;

    /* compute mean_sub . cov_inv */
    for (b1 = 0; b1 < shape.bands; b1++) {
        temp[b1] = 0.0;
        for (b2 = 0; b2 < shape.bands; b2++) {
            temp[b1] += mean_sub[b2] * cov_inv[b2 * shape.bands + b1];
        }
    }
    /* compute (mean_sub . cov_inv) . mean_sub */
    for (b1 = 0; b1 < shape.bands; b1++) {
        *score += temp[b1] * mean_sub[b1];
    }

    return EOS_SUCCESS;

}

U64 eos_mise_detect_anomaly_rx_mreq(const EosInitParams* params) {
    U64 base_size = 0;
    U64 call_size = 0;
    U32 n;

    if (eos_assert(params != NULL)) { return 0; }

    n = params->mise_max_bands;

    // mean_pixel, mean_sub, and temp
    base_size += 3 * sizeof(F64) * n;
    // cov, cov_inv
    base_size += 2 * sizeof(F64) * (n * n);

    // No memory-allocating functions called (no need to update `call_size`)

    return base_size + call_size;
}

/* Use the RX algorithm to rank all pixels and return the top n_results */
EosStatus eos_mise_detect_anomaly_rx(const EosObsShape shape,
                                     const U16* data, U32* n_results,
                                     EosPixelDetection* results) {

    EosStatus status = EOS_SUCCESS;
    U32 b;
    F64 *mean_pixel, *mean_sub, *temp,
        *cov, *cov_inv;
    EosMemoryBuffer *mean_pixel_buffer,
        *cov_buffer, *cov_inv_buffer,
        *mean_sub_buffer, *temp_buffer;
    EosPixelDetection det;
    EosDetectionHeap heap;
    F64 score;

    if (eos_assert(n_results != NULL)) { return EOS_ASSERT_ERROR; }

    /* If we are asked to compute 0 results, just return success */
    if (*n_results == 0) {
        return EOS_SUCCESS;
    }

    /* If the observation is zero size, return success with zero results */
    if (shape.rows == 0 || shape.cols == 0) {
        *n_results = 0;
        return EOS_SUCCESS;
    }

    /* If we made it past the previous checks, we need to make sure these
     *  pointers aren't NULL (it's ok if they were NULL if either the
     * observation size or n_results were zero) */
    if (eos_assert(data != NULL)) { return EOS_ASSERT_ERROR; }
    if (eos_assert(results != NULL)) { return EOS_ASSERT_ERROR; }

    // Allocate memory after we've passed basic checks above
    status = lifo_allocate_buffer_checked(&mean_pixel_buffer,
        sizeof(F64) * shape.bands, "mean pixel buffer");
    if (status != EOS_SUCCESS) { return status; }
    mean_pixel = (F64*) mean_pixel_buffer->ptr;

    status = lifo_allocate_buffer_checked(&mean_sub_buffer,
        sizeof(F64) * shape.bands, "mean sub buffer");
    if (status != EOS_SUCCESS) { return status; }
    mean_sub = (F64*) mean_sub_buffer->ptr;

    status = lifo_allocate_buffer_checked(&temp_buffer,
        sizeof(F64) * shape.bands, "temp buffer");
    if (status != EOS_SUCCESS) { return status; }
    temp = (F64*) temp_buffer->ptr;

    status = lifo_allocate_buffer_checked(&cov_buffer,
        sizeof(F64) * shape.bands * shape.bands, "cov buffer");
    if (status != EOS_SUCCESS) { return status; }
    cov = (F64*) cov_buffer->ptr;

    status = lifo_allocate_buffer_checked(&cov_inv_buffer,
        sizeof(F64) * shape.bands * shape.bands, "cov_inv buffer");
    if (status != EOS_SUCCESS) { return status; }
    cov_inv = (F64*) cov_inv_buffer->ptr;

    /* 1. Compute RX background from all pixels */
    /* Compute mean pixel */
    status = compute_mean_pixel(data, &shape, mean_pixel);
    if (status != EOS_SUCCESS) { return status; }

    /* Compute the covariance matrix and invert it */
    status = compute_covariance(data, &shape, mean_pixel, cov);
    if (status != EOS_SUCCESS) { return status; }
    status = invert_sym_matrix(shape.bands, cov, cov_inv);
    if (status != EOS_SUCCESS) { return status; }

    /* Initialize heap with results array */
    heap.capacity = *n_results;
    heap.size = 0;
    heap.data = results;

    /* Compute a score for each pixel and store the top results in the heap */
    for (det.row = 0; det.row < shape.rows; det.row++) {
        for (det.col = 0; det.col < shape.cols; det.col++) {
            for (b = 0; b < shape.bands; b++) {
                mean_sub[b] = data[(det.row * shape.cols + det.col)
                                   * shape.bands + b] - mean_pixel[b];
            }
            status = _rx_score(mean_sub, cov_inv, shape, temp, &score);
            if (status != EOS_SUCCESS) { return status; }
            det.score = score;

            status = detection_heap_push(&heap, det);
            if (status != EOS_SUCCESS) { return status; }
        }
    }

    status = detection_heap_sort(&heap);
    if (status != EOS_SUCCESS) { return status; }

    /* Update n_results with the number of actual detections returned */
    *n_results = heap.size;

    // Deallocate memory in LIFO order
    status = lifo_deallocate_buffer(cov_inv_buffer);
    if (status != EOS_SUCCESS) { return status; }
    status = lifo_deallocate_buffer(cov_buffer);
    if (status != EOS_SUCCESS) { return status; }
    status = lifo_deallocate_buffer(temp_buffer);
    if (status != EOS_SUCCESS) { return status; }
    status = lifo_deallocate_buffer(mean_sub_buffer);
    if (status != EOS_SUCCESS) { return status; }
    status = lifo_deallocate_buffer(mean_pixel_buffer);
    if (status != EOS_SUCCESS) { return status; }

    return status;
}

