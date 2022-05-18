#pragma once

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "IntegralCalculator.h"
#include "ParseInfo.h"

enum DistributionError_t{ DERROR_NULL_ARGUMENT,
                          DERROR_CONNECTION,
                          DERROR_FIND,
                          DERROR_OK,
                          DERROR_CALCULATION,
                          DERROR_KEEPALIVE};
typedef enum DistributionError_t DistributionError;

enum {BROADCAST_PORT = 8354, TCP_PORT = BROADCAST_PORT + 1, SIZE_BUFF = 1024, BACKLOG_LEN = 10, TIMEOUT = 5, LISTEN_TIMEOUT = 5};

DistributionError StartSideNode();
DistributionError StartMainNode(size_t numThreads, size_t numComputers, struct Integral_t integral, double* res);
double func(double x);