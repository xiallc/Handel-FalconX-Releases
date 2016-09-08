/* Sets parameters and tests saving the system to a copy of the ini file. */

/*
 * This code accompanies the XIA Code and tests Handel via C.
 *
 * Copyright (c) 2005-2012 XIA LLC
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


typedef struct {
  double value;
  const char* name;
} acq_setting;

static const acq_setting falconx_acq_settings[] = {
    {    1.0,    "invert_input" },
    {    0.05,   "cal_noise_floor" },
    {   41.2345, "dynamic_range" },
    {    5.678,  "preamp_gain" },
    {   -0.2,    "dc_offset" },
    { 1234.0,    "analog_offset" },
    {    0.25,   "cal_max_pulse_amp" },
    {    0.0,    NULL }
};

static const acq_setting falconxn_acq_settings[] = {
    {    3.0,    "analog_gain" },
    {   12.3,    "analog_offset" },
    {    1.0,    "detector_polarity" },
    {    0.0,    "termination" },
    {    0.0,    "attenuation" },
    {    0.0,    "coupling" },
    {    0.0,    "decay_time" },
    { 2048.0,    "number_mca_channels" },
    {    0.0,    NULL }
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
    double value;
    int channels;
    int channel;
    const acq_setting* settings = falconxn_acq_settings;

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

    printf("Loading the .ini file.\n");
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
        settings = falconx_acq_settings;
    } else if (strcmp(argv[1], "falconxn") == 0) {
        settings = falconxn_acq_settings;
    }

    printf("Channel count: %d.\n", channels);

    /* Set some values in all channels. */
    for (channel = 0; channel < channels; ++channel) {
        const acq_setting* channel_settings = settings;
        while (channel_settings->name) {
            value = channel_settings->value;;
            status = xiaSetAcquisitionValues(0,
                                             channel_settings->name,
                                             &value);
            CHECK_ERROR(status);
            ++channel_settings;
        }
    }

    printf("Saving the .ini file.\n");
    status = xiaSaveSystem("handel_ini", "t_api/sandbox/xia_test_helper.gen.ini");
    CHECK_ERROR(status);

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
