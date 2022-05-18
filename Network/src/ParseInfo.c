#include "ParseInfo.h"

struct CoreInfo_t {
    size_t coreId;
    size_t numCpu;
    size_t* cpusNum;
    size_t numAllocCpus;
    size_t numWorkingCpu;
};

struct ComputerInfo_t {
    size_t numCores;
    size_t numCPU;
};


//-----------------------------------------------------------------------------------

static const char TOPOLOGY_PATH[] = "/sys/devices/system/cpu/cpu%zu/topology/core_id";
enum {PARSE_ERROR = -1};

//---------------------------------Parse Core Info-----------------------------------

static size_t GetCoreId_(size_t coreNum);
static struct CoreInfo_t* UpdateCoreInfo_(struct CoreInfo_t* coresInfo, size_t size, size_t coreId, size_t numCpu);
static struct CoreInfo_t* CreateCoreInfo_(struct CoreInfo_t* coresInfo, size_t size);
static size_t ShrinkToFitCoreInfos_(struct CoreInfo_t** coresInfo, size_t curSize);

//---------------------------------Parse Core Info-----------------------------------

struct CoreInfo_t* GetCoresInfo(struct ComputerInfo_t* computerInfo) {
    assert(computerInfo);
    long ret = sysconf(_SC_NPROCESSORS_CONF);
    if (ret < 0) {
        perror("sysconf(_SC_NPROCESSORS_CONF):");
        return NULL;
    }

    computerInfo->numCPU = (size_t) ret;
    struct CoreInfo_t *coresInfo = (struct CoreInfo_t *) calloc(computerInfo->numCPU, sizeof(struct CoreInfo_t));
    if (!coresInfo) {
        perror("calloc");
        return NULL;
    }

    for (size_t itCPU = 0; itCPU < computerInfo->numCPU; ++itCPU) {
        size_t curCoreId = GetCoreId_(itCPU);
        if (curCoreId == PARSE_ERROR) {
            //TODO: free cores info
            return NULL;
        }

        struct CoreInfo_t* curCoreInfo = UpdateCoreInfo_(coresInfo, computerInfo->numCPU, curCoreId, itCPU);
        if (!curCoreInfo) {
            //TODO: free cores info
            return NULL;
        }
    }
    computerInfo->numCores = ShrinkToFitCoreInfos_(&coresInfo, computerInfo->numCPU);

    return coresInfo;
}

static size_t GetCoreId_(size_t coreNum) {
    size_t coreId = PARSE_ERROR;
    char curTopologyPath[sizeof(TOPOLOGY_PATH)] = {};
    sprintf(curTopologyPath, TOPOLOGY_PATH, coreNum);
    //fprintf(stderr, "\n%s\n", curTopologyPath);

    FILE* topologyFile = fopen(curTopologyPath, "r");
    if (!topologyFile) {
        fprintf(stderr, "ParseError");
        return PARSE_ERROR;
    }

    fscanf(topologyFile, "%zu", &coreId);
    fclose(topologyFile);
    return coreId;
}

static struct CoreInfo_t* UpdateCoreInfo_(struct CoreInfo_t* coresInfo, size_t size, size_t coreId, size_t numCpu) {
    assert(coresInfo);

    struct CoreInfo_t* curCoreInfo = GetCoreInfoById(coresInfo, size, coreId);

    if (curCoreInfo == NULL || curCoreInfo->numCpu == 0) {
        curCoreInfo = CreateCoreInfo_(coresInfo, size);
        if (curCoreInfo == NULL) {
            return NULL;
        }
        curCoreInfo->coreId = coreId;
    }

    if (curCoreInfo->numCpu == curCoreInfo->numAllocCpus) {
        curCoreInfo->cpusNum = (size_t*) realloc(curCoreInfo->cpusNum, 2 * curCoreInfo->numAllocCpus * sizeof(size_t));
        if (curCoreInfo->cpusNum == NULL) {
            perror("calloc");
            return NULL;
        }
        curCoreInfo->numAllocCpus = 2 * curCoreInfo->numAllocCpus;
    }

    curCoreInfo->cpusNum[curCoreInfo->numCpu] = numCpu;
    curCoreInfo->numCpu++;
    return curCoreInfo;
}

struct CoreInfo_t* GetCoreInfoById(struct CoreInfo_t* coresInfo, size_t size, size_t coreId) {
    assert(coresInfo);
    for (size_t itCore = 0; itCore < size; ++itCore) {
        if (coresInfo[itCore].coreId == coreId) {
            return &coresInfo[itCore];
        }
    }
    return NULL;
}

static struct CoreInfo_t* CreateCoreInfo_(struct CoreInfo_t* coresInfo, size_t size) {
    assert(coresInfo);
    struct CoreInfo_t* curCoreInfo = NULL;

    for (size_t itCoreInfo = 0; itCoreInfo < size; ++itCoreInfo) {
        if (coresInfo[itCoreInfo].numCpu == 0) {
            curCoreInfo = &coresInfo[itCoreInfo];
            break;
        }
    }
    curCoreInfo->cpusNum = (size_t*)calloc(1, sizeof(size_t));
    if (curCoreInfo->cpusNum == NULL) {
        perror("calloc");
        return  NULL;
    }
    curCoreInfo->numAllocCpus = 1;
    return curCoreInfo;
}

static size_t ShrinkToFitCoreInfos_(struct CoreInfo_t** coresInfo, size_t curSize) {
   assert(coresInfo);

   struct CoreInfo_t* coreInfo = *coresInfo;
    size_t newSize = curSize;
    for (size_t itCoreInfo = 0; itCoreInfo < curSize; ++itCoreInfo) {
        if (coreInfo[itCoreInfo].numCpu == 0) {
            free(coreInfo[itCoreInfo].cpusNum);
            newSize--;
        }
    }
    coreInfo = (struct CoreInfo_t*)realloc(coreInfo, newSize * sizeof(struct CoreInfo_t));
    if (coreInfo == NULL) {
        perror("realloc");
        return 0;
    }
    *coresInfo = coreInfo;
    return newSize;
}

void FreeCoresInfo(struct CoreInfo_t* coreInfo, size_t size) {
    assert(coreInfo);

    for (size_t itCoreInfo = 0; itCoreInfo < size; ++itCoreInfo) {
        free(coreInfo[itCoreInfo].cpusNum);
    }
    free(coreInfo);
}

void PrintCoresInfo(const struct CoreInfo_t* coreInfo, size_t size)
{
    assert(coreInfo);

    fprintf(stderr, "\n====== Cores Info ======\n");
    for (size_t itCoresInfo = 0; itCoresInfo < size; ++itCoresInfo) {
        fprintf(stderr, "\n--- Core id = %zu ---\n", coreInfo[itCoresInfo].coreId);
        fprintf(stderr, "num CPUs = %zu, ",      coreInfo[itCoresInfo].numCpu);
        fprintf(stderr, "num alloc CPUs = %zu, ", coreInfo[itCoresInfo].numAllocCpus);
        fprintf(stderr, "num working CPUs = %zu, ", coreInfo[itCoresInfo].numWorkingCpu);
        //fprintf(stderr, "num busy CPUs = %zu:\n", CIs[iNumCI].nBusyCpus);
        fprintf(stderr, "numCpus : [");
        //assert(coreInfo[itCoresInfo].cpusNum);
        for (size_t itCpu = 0; itCpu < coreInfo[itCoresInfo].numCpu; itCpu++)
            fprintf(stderr, " %zu",  coreInfo[itCoresInfo].cpusNum[itCpu]);
            fprintf(stderr, "]\n");
    }
}

long long int Read_Number_from_Text(const char* text) {
    char **endptr = calloc(1, sizeof(char*));
    long long int num = strtoll(text, endptr, 10);
    if (num < 0) {
        fprintf(stderr, "Less 0");
        exit(EXIT_FAILURE);
    }
    if (num == LONG_MAX) {
        fprintf(stderr, "Big number");
        exit(EXIT_FAILURE);
    }
    if (**endptr != '\0') {
        fprintf(stderr, "Wrong format");
        exit(EXIT_FAILURE);
    }
    return num;
}