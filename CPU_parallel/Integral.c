#include "Integral.h"
#include "CPU_info.h"

const double DELTA = 0.0001;

struct IntegralInfo {
	double begin;
	double end;
	double delta;
	double (*func) (double x);
};

struct ThreadInfo {
	double result;
	struct IntegralInfo integral_info;
};

struct CoreInfo {
	size_t core_id;
	size_t num_cpu;
	size_t* cpus_num;
	size_t num_alloc_cpus;
	size_t num_working_cpu;
};

//---------------------------------get_ Thread Info-----------------------------------

static void* get_threads_info(size_t num_threads, size_t* size_threads_info);
static void split_load_threads_(void* threadsInfo, size_t size_threads_info, size_t num_thread,
								   size_t num_real_thread, struct IntegralInfo* integral_info);
static void calculate(struct ThreadInfo* thread_info);
static int init_attr(struct CoreInfo* core_info, pthread_attr_t* attr);
static size_t get_num_hyper_threads(struct CoreInfo* core_info, size_t num_core);
static size_t get_core_id(struct CoreInfo* core_info, size_t num_core, size_t thread_num);
static size_t get_rounded_thread(struct CoreInfo* cores_info, size_t num_core, size_t num_threads);
//---------------------------------get_ Thread Info-----------------------------------


static void* get_threads_info(size_t num_threads, size_t* size_threads_info) {
	assert(size_threads_info);

	long page_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	if (page_size < 0) {
		perror("_SC_LEVEL1_DCACHE_LINESIZE");
		return NULL;
	}

	*size_threads_info = (sizeof(struct ThreadInfo) / page_size + 1) * page_size;
	void* threadsInfo = calloc(num_threads, *size_threads_info);
	return threadsInfo;
}


//Expect that end > begin
static void split_load_threads_(void* threadsInfo, size_t size_threads_info, size_t num_thread,
								   size_t num_real_thread, struct IntegralInfo* integral_info) {
	assert(threadsInfo);
	assert(integral_info);
	assert(integral_info->end - integral_info->begin > 0);

	double data_step = (integral_info->end - integral_info->begin) / num_thread;

	for (size_t it_thread = 0; it_thread < num_thread; ++it_thread) {
		((struct ThreadInfo*)(&threadsInfo[it_thread*size_threads_info]))->integral_info.begin =
				integral_info->begin + it_thread * data_step;
		((struct ThreadInfo*)(&threadsInfo[it_thread*size_threads_info]))->integral_info.end =
				integral_info->begin + (it_thread + 1) * data_step;
		((struct ThreadInfo*)(&threadsInfo[it_thread*size_threads_info]))->integral_info.delta = integral_info->delta;
		((struct ThreadInfo*)(&threadsInfo[it_thread*size_threads_info]))->integral_info.func = integral_info->func;
	}
	for (size_t itFictThread = num_thread; itFictThread < num_real_thread; ++itFictThread) {
		memcpy(&threadsInfo[itFictThread*size_threads_info], threadsInfo, sizeof(struct ThreadInfo));
	}
}

static void print_thread_info(void* threadsInfo, size_t size_threads_info, size_t num_thread) {
	assert(threadsInfo);

	fprintf(stderr, "\n==== Dump TI ====\n");
	for (size_t it_thread = 0; it_thread < num_thread; it_thread++) {
		struct ThreadInfo* curthread_info = threadsInfo + it_thread * size_threads_info;
		fprintf(stderr, "---- num_thread = %zu:", it_thread);
		fprintf(stderr, "from [%f] to [%f] with delta = %f\n",
				curthread_info->integral_info.begin, curthread_info->integral_info.end, curthread_info->integral_info.delta);
		fprintf(stderr, "res = %f\n", curthread_info->result);
	}
}

//---------------------------------API functions------------------------------------------

enum INTEGRAL_ERROR_t integral_calc(struct Integral integral, size_t num_threads, double* res) {
	if (!res || !integral.func) {
		return NULL_POINTER_ARG;
	}

	size_t num_cores = 0;
	struct CoreInfo* cores_info = get_cores_info(&num_cores);
	if (!cores_info) {
		return CORES_INFO_ERROR;
	}

//	print_cores_info(cores_info, num_cores);

	size_t sizethread_info = 0;
	size_t num_real_thread = get_rounded_thread(cores_info, num_cores, num_threads);

	void* threadsInfo = get_threads_info(num_real_thread, &sizethread_info);
	if (!threadsInfo) {
		return THREADS_INFO_ERROR;
	}
	struct IntegralInfo integral_info = {.begin = integral.begin,
											.end = integral.end,
											.delta = DELTA,
											.func = integral.func};
	split_load_threads_(threadsInfo, sizethread_info, num_threads,
						   num_real_thread, &integral_info);

	//print_thread_info(threadsInfo, sizethread_info, num_real_thread);

	pthread_t* pthreads = (pthread_t*)calloc(num_real_thread, sizeof(pthread_t));
	if (!pthreads) {
		perror("calloc");
		return SYSTEM_ERROR;
	}

	for (size_t it_thread = 0; it_thread < num_real_thread; ++it_thread) {
		size_t cur_core = get_core_id(cores_info, num_cores, it_thread);
		struct CoreInfo* cur_core_info = get_core_info_by_id(cores_info, num_cores, cur_core);
		if (!cur_core_info) {
			return CORES_INFO_ERROR;
		}

		pthread_attr_t pthread_attr = {};
		if (init_attr(cur_core_info, &pthread_attr) < 0) {
			return SYSTEM_ERROR;
		}

		if (pthread_create(&pthreads[it_thread], &pthread_attr, calculate,
						   threadsInfo + it_thread * sizethread_info) != 0) {
			return SYSTEM_ERROR;
		}

		if (pthread_attr_destroy(&pthread_attr) != 0) {
			return SYSTEM_ERROR;
		}
	}


	for (size_t it_thread = 0; it_thread < num_threads; ++it_thread) {
		if (pthread_join(pthreads[it_thread], NULL) != 0) {
			return SYSTEM_ERROR;
		}
		*res += ((struct ThreadInfo*)(threadsInfo + it_thread * sizethread_info))->result;
	}
	for (size_t itFictThread = num_threads; itFictThread < num_real_thread; ++itFictThread) {
		if (pthread_join(pthreads[itFictThread], NULL) != 0) {
			return SYSTEM_ERROR;
		}
	}

	//print_thread_info(threadsInfo, sizethread_info, num_threads);

	//print_cores_info(cores_info, num_cores);

	free(pthreads);
	free(threadsInfo);
	free_cores_info(cores_info, num_cores);
	return SUCCESS;
}


//---------------------------------Threads------------------------------------------------

static void calculate(struct ThreadInfo* thread_info) {
	assert(thread_info);

	double begin = thread_info->integral_info.begin;
	double end = thread_info->integral_info.end;
	double delta = thread_info->integral_info.delta;

	double result = 0.0;
	for (double x = begin; x < end; x += delta) {
		result += thread_info->integral_info.func(x) * delta;
	}

	thread_info->result = result;
	return;
}

static int init_attr(struct CoreInfo* core_info, pthread_attr_t* attr) {
	assert(core_info);
	assert(attr);
	assert(core_info->cpus_num);

	if (pthread_attr_init(attr) != 0) {
		return -1;
	}

	size_t it_cpu = core_info->num_working_cpu % core_info->num_cpu;
	size_t cpuId = core_info->cpus_num[it_cpu];

	cpu_set_t cpu_set = {};
	CPU_ZERO(&cpu_set);
	CPU_SET(cpuId, &cpu_set);

	if (pthread_attr_setaffinity_np(attr, sizeof(cpu_set), &cpu_set) != 0) {
		return -1;
	}

	core_info->num_working_cpu++;
	return 1;
}

static size_t get_num_hyper_threads(struct CoreInfo* core_info, size_t num_core) {
	size_t res = 0;
	for (size_t it_core = 0; it_core < num_core; ++it_core) {
		res += core_info[it_core].num_cpu;
	}
	return res;
}

static size_t get_core_id(struct CoreInfo* core_info, size_t num_core, size_t thread_num) {
	return thread_num % num_core;
}

static size_t get_rounded_thread(struct CoreInfo* cores_info, size_t num_core, size_t num_threads) {
	size_t num_hyper_thread = get_num_hyper_threads(cores_info, num_core);
	if (num_threads % num_hyper_thread != 0) {
		return num_threads + num_hyper_thread - (num_threads % num_hyper_thread);
	} else {
		return num_threads;
	}
}

//---------------------------------Test(debug) function-----------------------------------

//void test() {
//	size_t sizethread_info = 0;
//	struct IntegralInfo integral_info = {0, 8, 1};
//	void* threadsInfo = get_threads_info(8, &sizethread_info);
//	split_load_threads_(threadsInfo, sizethread_info, 8, &integral_info);
//	print_thread_info(threadsInfo, sizethread_info, 8);
//}