#pragma once

#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>

struct Integral {
	double begin;
	double end;
	double (*func) (double x);
};

enum INTEGRAL_ERROR_t{
	NULL_POINTER_ARG,
	CORES_INFO_ERROR,
	THREADS_INFO_ERROR,
	SYSTEM_ERROR,
	SUCCESS
};

enum INTEGRAL_ERROR_t integral_calc(struct Integral integral, size_t num_threads, double* res);