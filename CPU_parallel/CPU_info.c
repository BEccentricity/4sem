#include "CPU_info.h"

struct CoreInfo {
	size_t core_id;
	size_t num_cpu;
	size_t* cpus_num;
	size_t num_alloc_cpus;
	size_t num_working_cpu;
};


//-----------------------------------------------------------------------------------

static const char TOPOLOGY_PATH_[] = "/sys/devices/system/cpu/cpu%zu/topology/core_id";
enum {PARSE_ERROR = -1};

//---------------------------------Parse Core Info-----------------------------------

static size_t get_core_id_(size_t core_num);
static struct CoreInfo* update_core_info(struct CoreInfo* cores_info, size_t size, size_t core_id, size_t num_cpu);
static struct CoreInfo* create_core_info(struct CoreInfo* cores_info, size_t size);
static size_t reduce_core_info(struct CoreInfo** cores_info, size_t cur_size);

//---------------------------------Parse Core Info-----------------------------------

struct CoreInfo* get_cores_info(size_t* size) {
	assert(size);
	long ret = sysconf(_SC_NPROCESSORS_CONF);
	if (ret < 0) {
		perror("sysconf(_SC_NPROCESSORS_CONF):");
		return NULL;
	}

	size_t num_cpu = (size_t) ret;
	struct CoreInfo *cores_info = (struct CoreInfo*) calloc(num_cpu, sizeof(struct CoreInfo));
	if (!cores_info) {
		perror("calloc");
		return NULL;
	}

	for (size_t it_cpu = 0; it_cpu < num_cpu; ++it_cpu) {
		size_t cur_core_id = get_core_id_(it_cpu);
		if (cur_core_id == PARSE_ERROR) {
			//TODO: free cores info
			return NULL;
		}

		struct CoreInfo* cur_core_info = update_core_info(cores_info, num_cpu, cur_core_id, it_cpu);
		if (!cur_core_info) {
			//TODO: free cores info
			return NULL;
		}
	}
	*size = reduce_core_info(&cores_info, num_cpu);

	return cores_info;
}

static size_t get_core_id_(size_t core_num) {
	size_t core_id = PARSE_ERROR;
	char cur_path[sizeof(TOPOLOGY_PATH_)] = {};
	sprintf(cur_path, TOPOLOGY_PATH_, core_num);

	FILE* topology_file = fopen(cur_path, "r");
	if (!topology_file) {
		fprintf(stderr, "ParseError");
		return PARSE_ERROR;
	}

	fscanf(topology_file, "%zu", &core_id);
	fclose(topology_file);
	return core_id;
}

static struct CoreInfo* update_core_info(struct CoreInfo* cores_info, size_t size, size_t core_id, size_t num_cpu) {
	
    assert(cores_info);

	struct CoreInfo* cur_core_info = get_core_info_by_id(cores_info, size, core_id);

	if (cur_core_info == NULL || cur_core_info->num_cpu == 0) {
		cur_core_info = create_core_info(cores_info, size);
		if (cur_core_info == NULL) {
			return NULL;
		}
		cur_core_info->core_id = core_id;
	}

	if (cur_core_info->num_cpu == cur_core_info->num_alloc_cpus) {
		cur_core_info->cpus_num = (size_t*) realloc(cur_core_info->cpus_num, 2 * cur_core_info->num_alloc_cpus * sizeof(size_t));
		if (cur_core_info->cpus_num == NULL) {
			perror("calloc");
			return NULL;
		}
		cur_core_info->num_alloc_cpus = 2 * cur_core_info->num_alloc_cpus;
	}

	cur_core_info->cpus_num[cur_core_info->num_cpu] = num_cpu;
	cur_core_info->num_cpu++;
	return cur_core_info;
}

struct CoreInfo* get_core_info_by_id(struct CoreInfo* cores_info, size_t size, size_t core_id) {
	assert(cores_info);
	for (size_t it_core = 0; it_core < size; ++it_core) {
		if (cores_info[it_core].core_id == core_id) {
			return &cores_info[it_core];
		}
	}
	return NULL;
}

static struct CoreInfo* create_core_info(struct CoreInfo* cores_info, size_t size) {
	assert(cores_info);
	struct CoreInfo* cur_core_info = NULL;

	for (size_t it_core_info = 0; it_core_info < size; ++it_core_info) {
		if (cores_info[it_core_info].num_cpu == 0) {
			cur_core_info = &cores_info[it_core_info];
			break;
		}
	}
	cur_core_info->cpus_num = (size_t*)calloc(1, sizeof(size_t));
	if (cur_core_info->cpus_num == NULL) {
		perror("calloc");
		return  NULL;
	}
	cur_core_info->num_alloc_cpus = 1;
	return cur_core_info;
}

static size_t reduce_core_info(struct CoreInfo** ptr_cores_info, size_t cur_size) {
   assert(ptr_cores_info);

   struct CoreInfo* core_info = *ptr_cores_info;
	size_t new_size = cur_size;
	for (size_t it_core_info = 0; it_core_info < cur_size; ++it_core_info) {
		if (core_info[it_core_info].num_cpu == 0) {
			free(core_info[it_core_info].cpus_num);
			new_size--;
		}
	}
	core_info = (struct CoreInfo*)realloc(core_info, new_size * sizeof(struct CoreInfo));
	if (core_info == NULL) {
		perror("realloc");
		return 0;
	}
	*ptr_cores_info = core_info;
	return new_size;
}

void free_cores_info(struct CoreInfo* core_info, size_t size) {
	assert(core_info);

	for (size_t it_core_info = 0; it_core_info < size; ++it_core_info) {
		free(core_info[it_core_info].cpus_num);
	}
	free(core_info);
}

void print_cores_info(const struct CoreInfo* core_info, size_t size)
{
	assert(core_info);

	fprintf(stderr, "\n====== Cores Info ======\n");
	for (size_t it_cores_info = 0; it_cores_info < size; ++it_cores_info) {
		fprintf(stderr, "\n--- Core id = %zu ---\n", core_info[it_cores_info].core_id);
		fprintf(stderr, "num CPUs = %zu, ",	  core_info[it_cores_info].num_cpu);
		fprintf(stderr, "num alloc CPUs = %zu, ", core_info[it_cores_info].num_alloc_cpus);
		fprintf(stderr, "num working CPUs = %zu, ", core_info[it_cores_info].num_working_cpu);
		fprintf(stderr, "num_cpus : [");
		for (size_t it_cpu = 0; it_cpu < core_info[it_cores_info].num_cpu; it_cpu++)
			fprintf(stderr, " %zu",  core_info[it_cores_info].cpus_num[it_cpu]);
			fprintf(stderr, "]\n");
	}
}