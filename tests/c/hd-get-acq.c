/*
 * This code accompanies the XIA Code and tests Handel via C.
 *
 * Copyright (c) 2005-2013 XIA LLC
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


#include <stdio.h>
#include <stdlib.h>

#include "handel.h"
#include "handel_errors.h"
#include "md_generic.h"


static void CHECK_ERROR(int status);
static void ACQ_GET(int detChan, const char* name);

static const char* falconx_labels[] =
{
    "analog_offset",
    "analog_gain",
    "analog_gain_boost",
    "invert_input",
    "detector_polarity",
    "analog_discharge",
    "analog_discharge_threshold",
    "disable_input",
    "sample_rate",
    "dc_offset",
    "dc_tracking_mode",
    "operating_mode",
    "operating_mode_target",
    "reset_blanking_enable",
    "reset_blanking_threshold",
    "reset_blanking_presamples",
    "reset_blanking_postsamples",
    "min_pulse_pair_separation",
    "detection_threshold",
    "validator_threshold_fixed",
    "validator_threshold_proport",
    "cal_noise_floor",
    "cal_min_pulse_amp",
    "cal_max_pulse_amp",
    "cal_source_type",
    "cal_pulses_needed",
    "cal_filter_cutoff",
    "cal_est_count_rate",
    "hist_bin_count",
    "hist_samples_detected",
    "hist_samples_erased",
    "hist_pulses_detected",
    "hist_pulses_accepted",
    "hist_pulses_rejected",
    "hist_input_count_rate",
    "hist_output_count_rate",
    "hist_dead_time",
    "mapping_mode",
    "preset_type",
    "preset_value",
    "preset_baseline",
    "number_mca_channels",
    "preamp_gain",
    "dynamic_range",
    "adc_percent_rule",
    "calibration_energy",
    "mca_bin_width",
    NULL
};

static const char* falconxn_labels[] =
{
    "analog_gain",
    "analog_offset",
    "detector_polarity",
    "termination",
    "attenuation",
    "coupling",
    "decay_time",
    "dc_offset",
    "reset_blanking_enable",
    "reset_blanking_threshold",
    "reset_blanking_presamples",
    "reset_blanking_postsamples",
    "detection_threshold",
    "min_pulse_pair_separation",
    "detection_filter",
    "decay_time",
    "clock_speed",
    "number_mca_channels",
    "preset_type",
    "preset_value",
    "scale_factor",
    "mca_bin_width",
    "mapping_mode",
    "number_of_scas",
    NULL
};

#define MAXITEM_LEN 256

static void usage(const char* prog)
{
    printf("%s options\n", prog);
    printf(" -f file       : Handel INI file to load\n");
}

int main(int argc, char** argv)
{
    int status;

    int channels = 1;
    int channel;

    const char** labels = falconxn_labels;

    char module_type[MAXITEM_LEN];
    char ini[MAXITEM_LEN] = "t_api/sandbox/xia_test_helper.ini";
    int a;

    for (a = 1; a < argc; ++a) {
        if (argv[a][0] == '-') {
            switch (argv[a][1]) {
                case 'f':
                    ++a;
                    if (a >= argc) {
                        printf("error: no file provided\n");
                        exit (1);
                    }
                    strncpy(ini, argv[a], sizeof(ini) / sizeof(ini[0]));
                    break;

                default:
                    printf("error: invalid option: %s\n", argv[a]);
                    usage(argv[0]);
                    exit(1);
            }
        }
        else {
            printf("error: invalid option: %s\n", argv[a]);
            usage(argv[0]);
            exit(1);
        }
    }

    /* Setup logging here */
    printf("Configuring the Handel log file.\n");
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");


    printf("Loading the .ini file %s.\n", ini);
    status = xiaInit(ini);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    status = xiaGetModuleItem("module1", "module_type", module_type);
    CHECK_ERROR(status);

    status = xiaGetModuleItem("module1", "number_of_channels", &channels);
    CHECK_ERROR(status);

    if (strcmp(module_type, "falconx") == 0) {
        labels = falconx_labels;
        channels = 1;
    } else if (strcmp(module_type, "falconxn") == 0) {
        labels = falconxn_labels;

    }

    /* Get acquisition values */
    for (channel = 0; channel < channels; ++channel) {
        const char** label = labels;
        printf(" Channel: %d\n", channel);
        while (*label) {
            ACQ_GET(channel, *label);
            ++label;
        }
    }

    printf("Cleaning up Handel.\n");
    status = xiaExit();
    CHECK_ERROR(status);

    return 0;
}


/*
 * This is just an example of how to handle error values.  A program
 * of any reasonable size should implement a more robust error
 * handling mechanism.
 */
static void CHECK_ERROR(int status)
{
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        int status2;
        printf("Error encountered (exiting)! Status = %d\n", status);
        status2 = xiaExit();
        if (status2 != XIA_SUCCESS)
            printf("Handel exit failed, Status = %d\n", status2);
        exit(status);
    }
}

/*
 * Get the value and print.
 */
void ACQ_GET(int detChan, const char* name)
{
    int status;
    double value = 0;
    status = xiaGetAcquisitionValues(detChan, name, &value);
    if (status != XIA_SUCCESS) {
        printf("  %-30s:  FAILED (%d)\n", name, status);
    }
    else {
        printf("  %-30s: %13.3f\n", name, value);
    }
}
