#include "IntegralCalculator.h"
#include "ParseInfo.h"

const double DELTA = 0.0001;

struct IntegralInfo_t {
    double begin;
    double end;
    double delta;
    double (*func) (double x);
};

struct ThreadInfo_t {
    double result;
    struct IntegralInfo_t integralInfo;
};

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

//---------------------------------Get Thread Info-----------------------------------

static void* GetThreadsInfo_(size_t numThreads, size_t* sizeThreadsInfo);
static void DistributeLoadThreads_(void* threadsInfo, size_t sizeThreadsInfo, size_t numThread,
                                   size_t numRealThread, struct IntegralInfo_t* integralInfo);
static void Calculate(struct ThreadInfo_t* threadInfo);
static int InitPthreadAttr(struct CoreInfo_t* coreInfo, pthread_attr_t* attr);
static size_t GetNumHyperThreads_(struct CoreInfo_t* coreInfo, size_t numCore);
static size_t GetCoreId_(struct CoreInfo_t* coreInfo, size_t numCore, size_t threadNum);
static size_t GetRoundedThread_(struct CoreInfo_t* coresInfo, size_t numCore, size_t numThreads);
//---------------------------------Get Thread Info-----------------------------------


static void* GetThreadsInfo_(size_t numThreads, size_t* sizeThreadsInfo) {
    assert(sizeThreadsInfo);

    long sizeCacheLine = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (sizeCacheLine < 0) {
        perror("_SC_LEVEL1_DCACHE_LINESIZE");
        return NULL;
    }

    *sizeThreadsInfo = (sizeof(struct ThreadInfo_t) / sizeCacheLine + 1) * sizeCacheLine;
    void* threadsInfo = calloc(numThreads, *sizeThreadsInfo);
    if (!threadsInfo) {
        perror("calloc");
        return NULL;
    }
    return threadsInfo;
}


//Expect that end > begin
static void DistributeLoadThreads_(void* threadsInfo, size_t sizeThreadsInfo, size_t numThread,
                                   size_t numRealThread, struct IntegralInfo_t* integralInfo) {
    assert(threadsInfo);
    assert(integralInfo);
    assert(integralInfo->end - integralInfo->begin > 0);

    double dataStep = (integralInfo->end - integralInfo->begin) / numThread;

    for (size_t itThread = 0; itThread < numThread; ++itThread) {
        ((struct ThreadInfo_t*)(&threadsInfo[itThread*sizeThreadsInfo]))->integralInfo.begin =
                integralInfo->begin + itThread * dataStep;
        ((struct ThreadInfo_t*)(&threadsInfo[itThread*sizeThreadsInfo]))->integralInfo.end =
                integralInfo->begin + (itThread + 1) * dataStep;
        ((struct ThreadInfo_t*)(&threadsInfo[itThread*sizeThreadsInfo]))->integralInfo.delta = integralInfo->delta;
        ((struct ThreadInfo_t*)(&threadsInfo[itThread*sizeThreadsInfo]))->integralInfo.func = integralInfo->func;
    }
    for (size_t itFictThread = numThread; itFictThread < numRealThread; ++itFictThread) {
        memcpy(&threadsInfo[itFictThread*sizeThreadsInfo], threadsInfo, sizeof(struct ThreadInfo_t));
    }
}

static void PrintThreadInfo_(void* threadsInfo, size_t sizeThreadsInfo, size_t numThread) {
    assert(threadsInfo);

    fprintf(stderr, "\n==== Dump TI ====\n");
    for (size_t itThread = 0; itThread < numThread; itThread++) {
        struct ThreadInfo_t* curThreadInfo = threadsInfo + itThread * sizeThreadsInfo;
        fprintf(stderr, "---- numThread = %zu:", itThread);
        fprintf(stderr, "from [%f] to [%f] with delta = %f\n",
                curThreadInfo->integralInfo.begin, curThreadInfo->integralInfo.end, curThreadInfo->integralInfo.delta);
        fprintf(stderr, "res = %f\n", curThreadInfo->result);
    }
}

//---------------------------------API functions------------------------------------------

enum INTEGRAL_ERROR_t IntegralCalculate(struct CoreInfo_t* coresInfo, struct ComputerInfo_t* computerInfo,
                                        struct Integral_t integral, size_t numThreads, double* res) {
    if (!res || !integral.func || !coresInfo || !computerInfo) {
        return NULL_POINTER_ARG;
    }

//    PrintCoresInfo(coresInfo, numCores);

    size_t sizeThreadInfo = 0;
    size_t numRealThread = GetRoundedThread_(coresInfo, computerInfo->numCores, numThreads);

    void* threadsInfo = GetThreadsInfo_(numRealThread, &sizeThreadInfo);
    if (!threadsInfo) {
        return THREADS_INFO_ERROR;
    }
    struct IntegralInfo_t integralInfo = {.begin = integral.begin,
            .end = integral.end,
            .delta = DELTA,
            .func = integral.func};
    DistributeLoadThreads_(threadsInfo, sizeThreadInfo, numThreads,
                           numRealThread, &integralInfo);

    //PrintThreadInfo_(threadsInfo, sizeThreadInfo, numRealThread);

    pthread_t* pthreads = (pthread_t*)calloc(numRealThread, sizeof(pthread_t));
    if (!pthreads) {
        perror("calloc");
        return SYSTEM_ERROR;
    }

    for (size_t itThread = 0; itThread < numRealThread; ++itThread) {
        size_t curCore = GetCoreId_(coresInfo, computerInfo->numCores, itThread);
        struct CoreInfo_t* curCoreInfo = GetCoreInfoById(coresInfo, computerInfo->numCores, curCore);
        if (!curCoreInfo) {
            return CORES_INFO_ERROR;
        }

        pthread_attr_t pthreadAttr = {};
        if (InitPthreadAttr(curCoreInfo, &pthreadAttr) < 0) {
            return SYSTEM_ERROR;
        }

        if (pthread_create(&pthreads[itThread], &pthreadAttr, Calculate,
                           threadsInfo + itThread * sizeThreadInfo) != 0) {
            return SYSTEM_ERROR;
        }

        if (pthread_attr_destroy(&pthreadAttr) != 0) {
            return SYSTEM_ERROR;
        }
    }


    for (size_t itThread = 0; itThread < numThreads; ++itThread) {
        if (pthread_join(pthreads[itThread], NULL) != 0) {
            return SYSTEM_ERROR;
        }
        *res += ((struct ThreadInfo_t*)(threadsInfo + itThread * sizeThreadInfo))->result;
    }
    for (size_t itFictThread = numThreads; itFictThread < numRealThread; ++itFictThread) {
        if (pthread_join(pthreads[itFictThread], NULL) != 0) {
            return SYSTEM_ERROR;
        }
    }

    //PrintThreadInfo_(threadsInfo, sizeThreadInfo, numThreads);

    //PrintCoresInfo(coresInfo, numCores);

    free(pthreads);
    free(threadsInfo);
    return INTEGRAL_SUCCESS;
}

enum INTEGRAL_ERROR_t IntegralCalculateWithoutCoresInfo(struct Integral_t integral, size_t numThreads, double* res) {

    struct ComputerInfo_t computerInfo = {};
    struct CoreInfo_t* coresInfo = GetCoresInfo(&computerInfo);
    if (!coresInfo) {
        return CORES_INFO_ERROR;
    }
    return IntegralCalculate(coresInfo, &computerInfo, integral, numThreads, res);
}


//---------------------------------Threads------------------------------------------------

static void Calculate(struct ThreadInfo_t* threadInfo) {
    assert(threadInfo);

    double begin = threadInfo->integralInfo.begin;
    double end = threadInfo->integralInfo.end;
    double delta = threadInfo->integralInfo.delta;

    double result = 0.0;
    for (double x = begin; x < end; x += delta) {
        result += threadInfo->integralInfo.func(x) * delta;
    }

    threadInfo->result = result;
    return;
}

static int InitPthreadAttr(struct CoreInfo_t* coreInfo, pthread_attr_t* attr) {
    assert(coreInfo);
    assert(attr);
    assert(coreInfo->cpusNum);

    if (pthread_attr_init(attr) != 0) {
        return -1;
    }

    size_t itCpu = coreInfo->numWorkingCpu % coreInfo->numCpu;
    size_t cpuId = coreInfo->cpusNum[itCpu];

    cpu_set_t cpuSet = {};
    CPU_ZERO(&cpuSet);
    CPU_SET(cpuId, &cpuSet);

    if (pthread_attr_setaffinity_np(attr, sizeof(cpuSet), &cpuSet) != 0) {
        return -1;
    }

    coreInfo->numWorkingCpu++;
    return 1;
}

static size_t GetNumHyperThreads_(struct CoreInfo_t* coreInfo, size_t numCore) {
    size_t res = 0;
    for (size_t itCore = 0; itCore < numCore; ++itCore) {
        res += coreInfo[itCore].numCpu;
    }
    return res;
}

static size_t GetCoreId_(struct CoreInfo_t* coreInfo, size_t numCore, size_t threadNum) {
    return threadNum % numCore;
}

static size_t GetRoundedThread_(struct CoreInfo_t* coresInfo, size_t numCore, size_t numThreads) {
    size_t numHyperThread = GetNumHyperThreads_(coresInfo, numCore);
    if (numThreads % numHyperThread != 0) {
        return numThreads + numHyperThread - (numThreads % numHyperThread);
    } else {
        return numThreads;
    }
}

//---------------------------------Test(debug) function-----------------------------------

//void test() {
//    size_t sizeThreadInfo = 0;
//    struct IntegralInfo_t integralInfo = {0, 8, 1};
//    void* threadsInfo = GetThreadsInfo_(8, &sizeThreadInfo);
//    DistributeLoadThreads_(threadsInfo, sizeThreadInfo, 8, &integralInfo);
//    PrintThreadInfo_(threadsInfo, sizeThreadInfo, 8);
//}


