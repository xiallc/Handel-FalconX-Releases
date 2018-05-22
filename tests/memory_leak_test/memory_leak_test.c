/*
 * handel memory leak test
 * Use Visual Leak Detector to check for memory leaks in common operations
 * Generate a log file with memory leak information in the current path
 *
 * Copyright (c) 2005-2017 XIA LLC
 * All rights reserved
 *
 */

#ifdef __VLD_MEM_DBG__
#include <vld.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#pragma warning(disable : 4115)

#ifdef WIN32
#include <windows.h>
#endif

#include "handel.h"
#include "handel_errors.h"
#include "handel_constants.h"

#include "md_generic.h"

#define MAX_CHANNELS 8

static void CHECK_ERROR(int status, ...);
static int SEC_SLEEP(float *time);

static void print_usage(void);
static void do_run(int runtime);
static void do_sca(int runtime);
static void do_mapping(unsigned long nMapPixels);
static void start_system(char *ini_file);


int main(int argc, char *argv[])
{
    int i;
    int status;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    /* Setup logging here */
    printf("Configuring the Handel log file.\n");
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");

    for (i = 1; i < argc; i++) {
        start_system(argv[i]);

        do_run(10);
        do_sca(10);
        
        /* Check that restarting system works without memory failure */
        CHECK_ERROR(xiaExit());
        start_system(argv[i]);

        printf("Save ini file.\n");
        status = xiaSaveSystem("handel_ini", "memory_leak_test.ini");
        CHECK_ERROR(status);

        printf("Cleaning up Handel.\n");
        xiaExit();
    }

    return 0;
}

static void start_system(char *ini_file)
{
    int status;
    int ignored = 0;

    /* Acquisition Values */
    double bw = 5.0;

    printf("Loading the .ini file.\n");
    status = xiaInit(ini_file);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    /* Configure acquisition values, ignore error for unsupported device */
    printf("Setting the acquisition values.\n");
    status = xiaSetAcquisitionValues(-1, "mca_bin_width", &bw);

    /* Apply new acquisition values */
    printf("Applying the acquisition values.\n");
    status = xiaBoardOperation(0, "apply", &ignored);
    CHECK_ERROR(status);
}

/*
 * This is just an example of how to handle error values.  A program
 * of any reasonable size should implement a more robust error
 * handling mechanism.
 */
static void CHECK_ERROR(int status, ...)
{
    va_list ap;
    int i, paramN;
    void* to_free;
    
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error encountered! Status = %d\n", status);
        xiaExit();
        
        /* an optional list of pointers to free before exit */
        va_start(ap, paramN);
        for (i = 0; i < paramN; i++) {
            to_free = va_arg(ap, void *);
            if (to_free) free(to_free);
        } 
        va_end(ap);
        
        exit(status);
    }
}

/*
 * Do an MCA run then read out MCA data
 */
static void do_run(int durationS)
{
    int       status;
    uint32_t* accepted = NULL;
    int       size = 0;
    int       wait = 1;
    int       s;
    double    stats[MAX_CHANNELS * XIA_NUM_MODULE_STATISTICS];

    /* Start MCA mode */
    printf("Start an MCA run.\n");
    status = xiaStartRun(0, 0);
    CHECK_ERROR(status);

    status = xiaGetRunData(0, "mca_length", &size);
    CHECK_ERROR(status);

    printf("MCA Length: %d\n", size);

    accepted = malloc(sizeof(uint32_t) * (size_t) size);
    if (!accepted) {
        printf("No memory for the accepted array\n");
        status = XIA_NOMEM;
        CHECK_ERROR(status);
    }
    
    /* Number of seconds to display the plot. */
    for (s = 0; s < durationS; s += wait) {
        float  delay = (float)wait;
        double icr = 0;
        double ocr = 0;
        double realtime = 0;

        SEC_SLEEP(&delay);

        status = xiaGetRunData(0, "mca", accepted);
        CHECK_ERROR(status, (void *)accepted);

        status = xiaGetRunData(0, "input_count_rate", &icr);
        CHECK_ERROR(status, (void *)accepted);

        status = xiaGetRunData(0, "output_count_rate", &ocr);
        CHECK_ERROR(status, (void *)accepted);

        status = xiaGetRunData(0, "realtime", &realtime);
        CHECK_ERROR(status, (void *)accepted);

        printf("\n       Input Count Rate: %7.2f   Output Count Rate: %7.2f    Real time: %7.3f\n", icr, ocr, realtime);

        status = xiaGetRunData(0, "module_statistics_2", stats);
        CHECK_ERROR(status, (void *)accepted);

        printf("Module Input Count Rate: %7.2f   Output Count Rate: %7.2f    Real time: %7.3f\n", stats[5], stats[6], stats[0]);

    }

    free(accepted);

    /* Stop MCA mode */
    printf("Stop the MCA run.\n");
    status = xiaStopRun(0);
    CHECK_ERROR(status);
}

/*
 * A fairly thorough SCA operation which sets a few SCA regions across the
 * entires spectrum, do a run, then read out the SCA data.
 */
static void do_sca(int runtime)
{
    int i;
    int status;
    int ignored = 0;

    int maxsca = 0;
    
    double number_scas = 0.0;
    double* sca_values;

    double sca_bound = 0.0;
    double sca_size = 0.0;
    double number_mca_channels = 0.0;

    char scaStr[8];

    /* Set the number of SCAs */
    printf("Set SCAs\n");
    status = xiaGetRunData(0, "max_sca_length", (void *)&maxsca);
    CHECK_ERROR(status);

    number_scas = (double)maxsca;
    printf("Number of SCAs %0.0f\n", number_scas);
    
    status = xiaSetAcquisitionValues(0, "number_of_scas", (void *)&number_scas);
    CHECK_ERROR(status);

    /* Divide the entire spectrum region into equal number of SCAs */
    status = xiaGetAcquisitionValues(0, "number_mca_channels", (void *)&number_mca_channels);
    CHECK_ERROR(status);

    sca_bound = 0.0;
    sca_size = (int)floor(number_mca_channels/number_scas);

    /* Set the individual SCA limits */
    for (i = 0; i < (unsigned short)number_scas; i++) {
        sprintf(scaStr, "sca%d_lo", i);
        printf("%s %0.0f\n", scaStr, sca_bound);
        status = xiaSetAcquisitionValues(0, scaStr, (void *)&sca_bound);
        CHECK_ERROR(status);

        sca_bound += (sca_size - 1);

        sprintf(scaStr, "sca%d_hi", i);
        printf("%s %0.0f\n", scaStr, sca_bound);
        status = xiaSetAcquisitionValues(0, scaStr, (void *)&sca_bound);
        CHECK_ERROR(status);
    }

    /* Apply new acquisition values */
    status = xiaBoardOperation(0, "apply", &ignored);
    CHECK_ERROR(status);

    status = xiaStartRun(0, 0);
    CHECK_ERROR(status);
   
    for (int s = 0; s < runtime; s += 1) {
        float delay = 1.0;
        SEC_SLEEP(&delay);
    }
    
    status = xiaStopRun(0);
    CHECK_ERROR(status);
    
    printf("Read out the SCA values\n");
    sca_values = malloc(sizeof(double) * (size_t)number_scas);

    /* Read out the SCAs from the data buffer */
    status = xiaGetRunData(0, "sca", (void *)sca_values);
    CHECK_ERROR(status, (void *)sca_values);

    for (i = 0; i < (int)number_scas; i++) {
        printf(" SCA%d = %0f\n", i, sca_values[i]);
    }

    free(sca_values);    
}

static void do_mapping(unsigned long nMapPixels)
{
	int status;
	double mappingmode = 1.0;

    double pixPerBuffer = 2.0;
    double mcachannels = 1024.0;
    double pixelAdvanceMode = 1.0;

    unsigned short isFull = 0;
    int ignored = 0;

    unsigned long curPixel = 0;
    unsigned long bufLen = 0;
    unsigned long *databuffer = NULL;

    char curBuffer = 'a';

    char bufferfull[15];
    char buffername[9];

    /* Do mapping loop if supported */
    status = xiaSetAcquisitionValues(0, "mapping_mode", &mappingmode);
    if (status != XIA_SUCCESS) {
        return;
    }

    CHECK_ERROR(xiaBoardOperation(0, "apply", &ignored));
    CHECK_ERROR(xiaSetAcquisitionValues(-1, "pixel_advance_mode", &pixelAdvanceMode));
    CHECK_ERROR(xiaSetAcquisitionValues(-1, "number_mca_channels", &mcachannels));
    CHECK_ERROR(xiaSetAcquisitionValues(-1, "num_map_pixels_per_buffer", &pixPerBuffer));
    CHECK_ERROR(xiaBoardOperation(0, "apply", &ignored));

    CHECK_ERROR(xiaGetRunData(0, "buffer_len", (void *)&bufLen));

    databuffer = (unsigned long*)malloc(bufLen * sizeof(unsigned long));

    printf("Starting mapping loop buffer length %lu.\n", bufLen);
    CHECK_ERROR(xiaStartRun(-1, 0));

    /* Simulate pixel advance by using mapping_pixel_next at every loop. */
    do {
        sprintf(bufferfull, "buffer_full_%c", curBuffer);
        sprintf(buffername, "buffer_%c", curBuffer);

        isFull = 0;
        while (!isFull)
        {
            CHECK_ERROR(xiaBoardOperation(0, "mapping_pixel_next", &ignored));
            CHECK_ERROR(xiaGetRunData(0, bufferfull, &isFull));
        }

        CHECK_ERROR(xiaGetRunData(0, buffername, (void *)databuffer));
        CHECK_ERROR(xiaBoardOperation(0, "buffer_done", &curBuffer));
        CHECK_ERROR(xiaGetRunData(0, "current_pixel", (void *)&curPixel));

        if (curBuffer == 'a')
            curBuffer = 'b';
        else
            curBuffer = 'a';

    } while (curPixel < nMapPixels);

    CHECK_ERROR(xiaStopRun(-1));
    free(databuffer);
}

static void print_usage(void)
{
    fprintf(stdout, "\n");
    fprintf(stdout, "**********************************************************\n");
    fprintf(stdout, "* Memory leak detection test program for Handel library. *\n");
    fprintf(stdout, "* Run from staging folder with argument: [.ini file]     *\n");
    fprintf(stdout, "**********************************************************\n");
    fprintf(stdout, "\n");
    return;
}

static int SEC_SLEEP(float *time)
{
#ifdef WIN32
    DWORD wait = (DWORD)(1000.0 * (*time));
    Sleep(wait);
#else
    unsigned long secs = (unsigned long) *time;
    struct timespec req = {
      .tv_sec = (time_t) secs,
      .tv_nsec = (time_t) ((*time - secs) * 1000000000.0)
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
