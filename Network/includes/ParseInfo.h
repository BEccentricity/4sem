#pragma once

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

struct CoreInfo_t;
struct ComputerInfo_t;

struct CoreInfo_t* GetCoresInfo(struct ComputerInfo_t* computerInfo);
void FreeCoresInfo(struct CoreInfo_t* coreInfo, size_t size);
void PrintCoresInfo(const struct CoreInfo_t* coreInfo, size_t size);
struct CoreInfo_t* GetCoreInfoById(struct CoreInfo_t* coresInfo, size_t size, size_t coreId);
long long int Read_Number_from_Text(const char* text);
