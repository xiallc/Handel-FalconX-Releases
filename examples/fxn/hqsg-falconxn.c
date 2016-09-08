/**
 * @example hqsg-falconxn.c
 *
 * @brief This code accompanies the XIA Application Note "Handel Quick Start
 * Guide: FalconXn". This sample code shows how to start and manually stop
 * an MCA data acquisition run.
 */

/*
 * Copyright (c) 2016 XIA LLC
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

#pragma warning(disable : 4115)

/* For Sleep() */
#include <windows.h>

#include "handel.h"
#include "handel_errors.h"
#include "handel_constants.h"
#include "md_generic.h"


static void CHECK_ERROR(int status);


int main(int argc, char *argv[])
{
    int status;

    /* Set up logging */
    printf("Configuring the Handel log file.\n");
    xiaSetLogLevel(MD_WARNING);
    xiaSetLogOutput("handel.log");

    printf("Loading the .ini file.\n");
    status = xiaInit("falconxn.ini");
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    printf("Setting the acquisition values.\n");

    /* [Configure acquisition values] */
    double detection_threshold = 0.010;
    status = xiaSetAcquisitionValues(0, "detection_threshold", &detection_threshold);
    CHECK_ERROR(status);

    double min_pulse_pair_separation = 25.0;
    status = xiaSetAcquisitionValues(0, "min_pulse_pair_separation",
                                     &min_pulse_pair_separation);
    CHECK_ERROR(status);

    double detection_filter = XIA_FILTER_MID_RATE;
    status = xiaSetAcquisitionValues(0, "detection_filter", &detection_filter);
    CHECK_ERROR(status);

    double scale_factor = 2.0;
    status = xiaSetAcquisitionValues(0, "scale_factor", &scale_factor);
    CHECK_ERROR(status);
    /* [Configure acquisition values] */

    /* Start a run w/ the MCA cleared */
    printf("Starting the run.\n");
    status = xiaStartRun(0, 0);
    CHECK_ERROR(status);

    printf("Waiting 5 seconds to collect data.\n");
    Sleep((DWORD)5000);

    printf("Stopping the run.\n");
    status = xiaStopRun(0);
    CHECK_ERROR(status);

    /* [Read MCA spectrum] */
    printf("Getting the MCA length.\n");

    unsigned long mca_len = 0;
    status = xiaGetRunData(0, "mca_length", &mca_len);
    CHECK_ERROR(status);

    /* If you don't want to dynamically allocate memory here,
     * then be sure to declare mca as an array of length 8192,
     * since that is the maximum length of the spectrum.
     */
    printf("Allocating memory for the MCA data.\n");
    unsigned long *mca = malloc(mca_len * sizeof(unsigned long));

    if (!mca) {
        /* Error allocating memory */
        exit(1);
    }
    
    printf("Reading the MCA.\n");
    status = xiaGetRunData(0, "mca", mca);
    CHECK_ERROR(status);

    /* Display the spectrum, write it to a file, etc... */
    
    printf("Release MCA memory.\n");
    free(mca);

    /* [Read MCA spectrum] */
    
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
        printf("Error encountered! Status = %d\n", status);
        getchar();
        exit(status);
    }
}
