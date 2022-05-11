#pragma once

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

struct CoreInfo;

struct CoreInfo* get_cores_info(size_t* size);
void free_cores_info(struct CoreInfo* coreInfo, size_t size);
void print_cores_info(const struct CoreInfo* coreInfo, size_t size);
struct CoreInfo* get_core_info_by_id(struct CoreInfo* coresInfo, size_t size, size_t core_id);