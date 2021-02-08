#include "sim_log.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

// No easy way to do closures in C, so use a global variable for the logging
// function
static FILE* _global_log_file = NULL;
static struct timespec _log_start_time;

FILE* open_log_file(const char *outputfile);
void close_log_file(FILE *file);

EosStatus sim_log_init(const char *outputfile) {
    _global_log_file = open_log_file(outputfile);
    if (_global_log_file == NULL) { return EOS_ERROR; }
    clock_gettime(CLOCK_REALTIME, &_log_start_time);
    return EOS_SUCCESS;
}

void sim_log_teardown(void) {
    if (_global_log_file != NULL) {
        close_log_file(_global_log_file);
        _global_log_file = NULL;
    }
}

double compute_elapsed(struct timespec start, struct timespec stop) {
    struct timespec elapsed;
    if ((stop.tv_nsec - start.tv_nsec) < 0) {
        elapsed.tv_sec = stop.tv_sec - start.tv_sec - 1;
        elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec + 1000000000;
    } else {
        elapsed.tv_sec = stop.tv_sec - start.tv_sec;
        elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec;
    }
    return elapsed.tv_sec + (elapsed.tv_nsec / 1e9);
}

void log_function(EosLogType type, const char* message) {
    if (_global_log_file != NULL) {
        char *prefix;
        struct timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        double elapsed = compute_elapsed(_log_start_time, current_time);
        switch (type) {
            case EOS_LOG_DEBUG:
                prefix = "DEBUG";
                break;
            case EOS_LOG_INFO:
                prefix = "INFO";
                break;
            case EOS_LOG_WARN:
                prefix = "WARNING";
                break;
            case EOS_LOG_ERROR:
                prefix = "ERROR";
                break;
            case EOS_LOG_KEY_VALUE:
                prefix = "KEYVAL";
                break;
            default:
                prefix = "LOG";
        }
        fprintf(
            _global_log_file, "[%15.9f] %s: %s\n",
            elapsed, prefix, message
        );
    }
}

FILE* open_log_file(const char *outputfile) {
    if (strcmp("-", outputfile) == 0) {
        return stdout;
    } else {
        FILE *file;
        file = fopen(outputfile, "w");
        if (file == NULL) {
            fprintf(stderr, "Error opening output file \"%s\"\n", outputfile);
        }
        return file;
    }
}

void close_log_file(FILE *file) {
    if (file) {
        fflush(file);
        if (file != stdout) {
            fclose(file);
        }
    }
}

