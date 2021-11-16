/*
 * This code accompanies the XIA Code and tests Handel via C.
 *
 * Copyright (c) 2005-2019 XIA LLC
 * All rights reserved
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the
 *     following disclaimer.
 *   * Redistributions in binary form must reproduce the
 *     above copyright notice, this list of conditions and the
 *     following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *   * Neither the name of XIA LLC
 *     nor the names of its contributors may be used to endorse
 *     or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Exercises the FalconX's list-mode functionality by repeatedly
 * reading out the buffers as fast as possible.
 */
#ifdef __VLD_MEM_DBG__
#include <vld.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "xia_common.h"

#include "handel.h"
#include "handel_errors.h"

#include "md_generic.h"


static int SEC_SLEEP(double time);
static void print_usage(void);
static void check_error(int status, char* function);
static void clean_up();

#define A 0
#define B 1

#define SWAP_BUFFER(x) ((x) == A ? B : A)

#define MAX_DET_CHANNELS (8)

uint32_t *buffer = NULL;
FILE* fp[MAX_DET_CHANNELS];
int det_channels = 0;

int main(int argc, char *argv[])
{
    const char* ini = "t_api/sandbox/xia_test_helper.ini";
    const char* data_prefix = "test_mm3";

    int status;
    int ignore;

    double mode = 3.0;
    double variant = 0.0;
    double n_secs = 0.0;
    double n_hrs = 0.0;

    time_t start;

    const char *buffer_str[2] = {
        "buffer_a",
        "buffer_b"
    };

    const char *buffer_full_str[2] = {
        "buffer_full_a",
        "buffer_full_b"
    };

    const char buffer_done_char[2] = {
        'a',
        'b'
    };

    int det;

    int current[MAX_DET_CHANNELS];
    int buffer_number[MAX_DET_CHANNELS];

    unsigned long bufferLength = 0;
    size_t bufferSize = 0;


    double wait_period = 0.050; /* 50 msecs */
    int quiet = 0;

    int arg = 1;

    while (arg < argc) {
        if (argv[arg][0] == '-') {
            if (strlen(argv[arg]) != 2) {
                fprintf(stderr, "error: invalid option: %s\n", argv[arg]);
                exit(1);
            }

            switch (argv[arg][1]) {
            case 'f':
                ++arg;
                if (arg >= argc) {
                    fprintf(stderr, "error: -f requires a file\n");
                    exit(1);
                }
                ini = argv[arg];
                ++arg;
                break;
            case 'D':
                ++arg;
                if (arg >= argc) {
                    fprintf(stderr, "error: -D requires a label\n");
                    exit(1);
                }
                data_prefix= argv[arg];
                ++arg;
                break;
            case 'H':
                ++arg;
                if (arg >= argc) {
                    fprintf(stderr, "error: -H requires the hours\n");
                    exit(1);
                }
                sscanf(argv[arg], "%lf", &n_hrs);
                ++arg;
                break;
            case 'S':
                ++arg;
                if (arg >= argc) {
                    fprintf(stderr, "error: -S requires the seconds\n");
                    exit(1);
                }
                sscanf(argv[arg], "%lf", &n_secs);
                ++arg;
                break;
            case 'w':
                ++arg;
                if (arg >= argc) {
                    fprintf(stderr,
                            "error: -w requires the number milli-seconds\n");
                    exit(1);
                }
                sscanf(argv[arg], "%lf", &wait_period);
                ++arg;
                break;
            case 'd':
                ++arg;
                if (arg >= argc) {
                    fprintf(stderr,
                            "error: -d requires the number of detector channels\n");
                    exit(1);
                }
                sscanf(argv[arg], "%d", &det_channels);
                ++arg;
                break;
            case 'q':
                quiet = 1;
                break;
            case '?':
                print_usage();
                exit(0);
            default:
                fprintf(stderr, "error: invalid option; try -?\n");
                exit(1);
            }
        } else {
            fprintf(stderr, "error: invalid option; try -?\n");
            exit(1);
        }
    }

    if (n_secs > 0 && n_hrs > 0) {
        fprintf(stderr, "error: seconds and hours set\n");
        exit(1);
    }
    if (n_hrs > 0)
        n_secs = (double) n_hrs * 60.0 * 60.0;

    if (n_secs == 0.0) {
        n_secs = 30.0;
    }

    fprintf(stdout, "MM3 Capture\n");
    fprintf(stdout, "  INI: %s\n", ini);
    fprintf(stdout, "  Data prefix: %s\n", data_prefix);

    if (n_hrs > 0)
        fprintf(stdout, "  Hours: %d\n", (int) n_hrs);
    else
        fprintf(stdout, "  Seconds: %d\n", (int) n_secs);

    if (quiet == 0.0)
        xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");

    status = xiaInit(ini);
    check_error(status, "initialize Handel");

    status = xiaStartSystem();
    check_error(status, "starting the system");

    if (det_channels == 0) {
        status = xiaGetModuleItem("module1", "number_of_channels", &det_channels);
        check_error(status, "getting number of channels");
    }

    /* Switch to the mode. */
    status = xiaSetAcquisitionValues(-1, "mapping_mode", &mode);
    check_error(status, "setting mapping mode");

    if (variant != 0.0) {
        status = xiaSetAcquisitionValues(-1, "list_mode_variant", &variant);
        check_error(status, "setting list mode variant");
    }

    for (det = 0; det < det_channels; ++det) {
        status = xiaBoardOperation(det, "apply", &ignore);
        check_error(status, "applying the mode settings");
    }

    status = xiaGetRunData(0, "buffer_len", &bufferLength);
    check_error(status, "reading 'buffer_len'");

    bufferSize = bufferLength * sizeof(uint32_t);
    buffer = malloc(bufferSize);

    if (!buffer)
        check_error(XIA_NOMEM, "allocating buffer");

    fprintf(stdout, "  Buffer length: %lu (%zu bytes).\n",
            bufferLength, bufferSize);

    memset(buffer, 0, bufferSize);

    for (det = 0; det < det_channels; ++det) {
        char name[256];

        current[det] = A;
        buffer_number[det] = 0;

        sprintf(name, "%s_d%02d.bin", data_prefix, det);
        fp[det] = fopen(name, "wb");

        if (!fp[det])
            check_error(XIA_OPEN_FILE, "opening file for writing");
    }

    fprintf(stdout, "Starting MM3 run.\n");

    status = xiaStartRun(-1, 0);
    check_error(status, "starting list mode run");

    /* The algorithm here is to read the current buffer, let the
     * hardware know we are done with it, write the raw buffer to disk
     * and then read the other buffer, etc.
     */
    time(&start);

    for (;;) {
        int any_buffer_full = 0;
        int any_running = 0;
        int buffer_full[MAX_DET_CHANNELS];
        unsigned long active[MAX_DET_CHANNELS];
        int polls = 0;

        double now = difftime(time(NULL), start);

        do {
            if (now >= n_secs)
                break;

            any_running = 0;

            for (det = 0; det < det_channels; ++det) {
                int buffer_overrun = 0;

                active[det] = 0;
                buffer_full[det] = 0;

                status = xiaGetRunData(det, "run_active", &active[det]);
                check_error(status, "getting run_active");

                if (active[det]) {
                    any_running = 1;
                }

                status = xiaGetRunData(det,
                                       buffer_full_str[current[det]],
                                       &buffer_full[det]);
                check_error(status, "getting the status of buffer");


                status = xiaGetRunData(det, "buffer_overrun", &buffer_overrun);
                check_error(status, "getting the overrun status of buffer");

                if (buffer_overrun)
                    check_error(XIA_UNKNOWN, "buffer overrun");

                if (buffer_full[det])
                    any_buffer_full = 1;
            }

            if (!any_buffer_full)
                SEC_SLEEP(wait_period);

            ++polls;

        } while (any_running && !any_buffer_full && (polls < (10 / wait_period)));

        if (now >= n_secs) {
#if 0  /* DISABLE_READING_REMAINDER */
            const char *list_buffer_level_str[2] = {
                "list_buffer_len_a",
                "list_buffer_len_b"
            };

            for (det = 0; det < det_channels; ++det) {
                int length = 0;

                status = xiaGetRunData(det, list_buffer_level_str[current[det]], &length);
                check_error(status, "getting buffer length");

                bufferLength = (unsigned long) length;

                status = xiaGetRunData(det, buffer_str[current[det]], buffer);
                check_error(status, "getting buffer data");

                fprintf(stdout, "Buffer write: det: %d buffer:%d length:%d\n",
                        det, buffer_number[det],
                        (int) bufferLength);

                if (fwrite(&buffer[0], sizeof(uint32_t),
                           bufferLength, fp[det]) != bufferLength)
                    check_error(XIA_BAD_FILE_WRITE, "wrting buffer data to file");
            }
#endif
            break;
        }

        if (!any_buffer_full)
            check_error(XIA_UNKNOWN, "timeout on buffer filling");

        printf("%d ", (int) now);
        for (det = 0; det < det_channels; ++det)
            printf("%d:%s/%s ",
                   det,
                   active[det] ? "ACTIVE" : "ready",
                   buffer_full[det] ? "FULL" : "empty");
        printf("\n");

        for (det = 0; det < det_channels; ++det) {
            if (buffer_full[det]) {
                status = xiaGetRunData(det, buffer_str[current[det]], buffer);
                check_error(status, "reading buffer status");

                char c = buffer_done_char[current[det]];
                status = xiaBoardOperation(det, "buffer_done", (void*) &c);
                check_error(status, "reading buffer_done");

                status = xiaGetRunData(det,
                                       buffer_full_str[current[det]],
                                       &buffer_full[det]);
                check_error(status, "reading buffer status after buffer_done");

                fprintf(stdout, "Buffer write: det: %d buffer:%d/%c full:%d length:%d\n",
                        det, buffer_number[det],
                        buffer_done_char[current[det]],
                        buffer_full[det],
                        (int) bufferLength);

                if (fwrite(&buffer[0], sizeof(uint32_t),
                           bufferLength, fp[det]) != bufferLength)
                    check_error(XIA_BAD_FILE_WRITE, "wrting buffer data to file");

                current[det] = SWAP_BUFFER(current[det]);
                buffer_number[det]++;
            }
        }
    }

    clean_up();
    return 0;
}


static int SEC_SLEEP(double time)
{
#ifdef WIN32
    DWORD wait = (DWORD)(1000.0 * (time));
    Sleep(wait);
#else
    unsigned long secs = (unsigned long) time;
    struct timespec req = {
        .tv_sec = (time_t) secs,
        .tv_nsec = (time_t) ((time - secs) * 1000000000.0)
    };
    struct timespec rem = {
      .tv_sec = 0,
      .tv_nsec = 0
    };
    while (TRUE_) {
        if (nanosleep(&req, &rem) == 0)
            break;
        req = rem;
    }
#endif
    return XIA_SUCCESS;
}


static void print_usage(void)
{
    fprintf(stdout,
            "hd-mm1 [optios]\n" \
            "options and arguments: \n" \
            " -?           : help\n" \
            " -f file      : INI file\n" \
            " -D label     : data prefix label\n" \
            " -H hours     : hours to run the capture\n" \
            " -S seconds   : seconds to run the capture\n" \
            " -w msecs     : wait period in milli-seconds\n" \
            " -d detectors : number of detector channels\n" \
            " -q           : quiet, no Handel debug output\n" \
            "Where:\n" \
            " ListMode data captured for hours which overrides seconds.\n" \
            " Wait time in milli-seconds defines the polling rate.\n");
    return;
}

static void clean_up()
{
    int det;

    printf("\nStopping run.\n");
    xiaStopRun(-1);

    printf("Cleaning up Handel.\n");
    xiaExit();

    for (det = 0; det < det_channels; ++det)
        if (fp[det]) fclose(fp[det]);

    if (buffer)
        free(buffer);
}

static void check_error(int status, char* function)
{
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error in %s, status = %d %s\n", function, status, xiaGetErrorText(status));
        clean_up();
        exit(1);
    }
}
