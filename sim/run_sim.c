#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <argp.h>

#ifndef EOS_NO_LIBCONFIG
#include <libconfig.h>
#endif

#include <eos.h>

#include "sim_ethemis.h"
#include "sim_mise.h"
#include "sim_pims.h"

/* Program documentation */
static char doc[] = "EOS Simulation Program";

/* Document positional argument(s) */
static char args_doc[] = "inputfile";

/* Keyed options */
static struct argp_option options[] = {
    {"help",   'h',  0,     0, "Give this help list", -1},
    {"output", 'o', "FILE", 0, "Output to FILE instead of standard output", 0},
    {"config", 'c', "FILE", 0, "Configuration file", 0},
    { 0 }
};

/* Struct to hold argument values */
#define N_ARGS 1
typedef struct {
    char *args[N_ARGS];
    char *outputfile;
    char *configfile;
} Arguments;

/* Parse command-line options */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    Arguments *args = state->input;

    switch (key) {
        case 'o':
            args->outputfile = arg;
            break;

        case 'c':
            args->configfile = arg;
            break;

        case 'h':
            argp_state_help(state, state->out_stream, ARGP_HELP_STD_HELP);
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num >= N_ARGS) {
                /* Too many arguments */
                argp_usage(state);
            }
            args->args[state->arg_num] = arg;
            break;

        case ARGP_KEY_END:
            if (state->arg_num < N_ARGS) {
                /* Not enough arguments. */
                argp_usage(state);
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Define argp parser */
static struct argp argp = {
    options, parse_opt, args_doc, doc, NULL, NULL, NULL
};

int main(int argc, char **argv) {
    /* Default argument values */
    error_t status;
    EosStatus eos_status = EOS_SUCCESS;
    Arguments args = {
        .outputfile = "-",
        .configfile = NULL,
    };

    /* Parse arguments. */
    status = argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &args);
    if (status != 0) {
        /* Terminate with error code. */
        return status;
    }

    /* Parse config, either with custom logic or libconfig. */
    config_ptr config;
    #ifdef EOS_NO_LIBCONFIG
        config = args.configfile;
    #else
        config_t cfg;
        config_t *cfg_ptr = NULL;
        config_init(&cfg);
        if (args.configfile != NULL) {
            if(!config_read_file(&cfg, args.configfile)) {
                fprintf(
                    stderr, "%s (line %d): %s\n", config_error_file(&cfg),
                    config_error_line(&cfg), config_error_text(&cfg)
                );
                config_destroy(&cfg);
                return EXIT_FAILURE;
            }
            cfg_ptr = &cfg;
        }
        config = cfg_ptr;
    #endif

    /* Check extension to see which kind of analysis to run. */
    char* inputfile = args.args[0];
    int len = strlen(inputfile);
    if (len > 4) {
        char* ext = &(inputfile[len-4]);
        printf("%s\n", ext);
        if (strncmp(ext, ".etm", 4) == 0) {
            eos_status = run_ethemis_sim(inputfile, args.outputfile, cfg_ptr);
        } else if (strncmp(ext, ".mis", 4) == 0) {
            eos_status = run_mise_sim(inputfile, args.outputfile, cfg_ptr);
        } else if (strncmp(ext, ".pim", 4) == 0) {
            eos_status = run_pims_sim(inputfile, args.outputfile, config);
        } else {
            fprintf(stderr, "Unknown file type %s\n", ext);
            return EXIT_FAILURE;
        }
    }

    #ifndef EOS_NO_LIBCONFIG
        config_destroy(&cfg);
    #endif

    return eos_status;
}
