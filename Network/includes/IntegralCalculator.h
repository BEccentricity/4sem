#pragma once

#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>

struct CoreInfo_t;
struct ComputerInfo_t;

struct Integral_t {
    double begin;
    double end;
    double (*func) (double x);
};

enum INTEGRAL_ERROR_t{
    NULL_POINTER_ARG,
    CORES_INFO_ERROR,
    THREADS_INFO_ERROR,
    SYSTEM_ERROR,
    INTEGRAL_SUCCESS
};

enum INTEGRAL_ERROR_t IntegralCalculate(struct CoreInfo_t* coresInfo, struct ComputerInfo_t* computerInfo,
                                        struct Integral_t integral, size_t numThreads, double* res);
enum INTEGRAL_ERROR_t IntegralCalculateWithoutCoresInfo(struct Integral_t integral, size_t numThreads, double* res);