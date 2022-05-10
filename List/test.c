#include "headers/List.h"
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

#define TEST_CONDITION(condition) {\
	\
	if(!(condition)){\
		fprintf(stderr, "%s : %d ", __FILE__, __LINE__);\
		fprintf(stderr, "Test failed\n");\
	}\
}

void list_test_arch() {

	List list;
	Node node[3];

	TEST_CONDITION(node_get_data(NULL) == NULL);

	TEST_CONDITION(list_get_head(NULL) == (Node) -1);
	TEST_CONDITION(list_get_tail(NULL) == (Node) -1);
	TEST_CONDITION(list_get_size(NULL) == (size_t) -1);

	TEST_CONDITION(list_push_front(NULL, (Node) 1) == -1);
	TEST_CONDITION(list_push_front((List) 1, NULL) == -1);
	TEST_CONDITION(list_push_back((List) 1, NULL) == -1);
	TEST_CONDITION(list_push_back(NULL, (Node) 1) == -1);
	TEST_CONDITION(list_push(NULL, (Node) 1, (Node) 1) == -1);
	TEST_CONDITION(list_push((List) 1, NULL, (Node) 1) == -1);
	TEST_CONDITION(list_push((List) 1, (Node) 1, NULL) == -1);

	TEST_CONDITION((node[0] = node_create((void*) 0xDED)) != NULL);
	TEST_CONDITION(node_get_data(node[0]) == (void*) 0xDED);
	node[1] = node_create((void*) 0xDEF);
	node[2] = node_create((void*) 0xDEC);

	TEST_CONDITION((list = list_create()) != NULL);
	TEST_CONDITION(list_push_front(list, node[0]) == 0)
	TEST_CONDITION(list_get_head(list) == node[0]);
	TEST_CONDITION(list_get_tail(list) == node[0]);
	TEST_CONDITION(list_get_size(list) == (size_t) 1);
	TEST_CONDITION(list_push_front(list, node[1]) == 0)
	TEST_CONDITION(list_get_head(list) == node[1]);
	TEST_CONDITION(list_get_tail(list) == node[0]);
	TEST_CONDITION(list_get_size(list) == (size_t) 2);
	TEST_CONDITION(list_pop_front(list) == node[1]);
	TEST_CONDITION(list_pop_front(list) == node[0]);
	TEST_CONDITION(list_pop_front(list) == (Node) -1);
	TEST_CONDITION(list_push_back(list, node[0]) == 0);
	TEST_CONDITION(list_push_back(list, node[1]) == 0);
	TEST_CONDITION(list_push(list, node[0], node[2]) == 0);
	TEST_CONDITION(list_get_head(list) == node[0]);
	TEST_CONDITION(list_get_tail(list) == node[1]);
	TEST_CONDITION(list_get_size(list) == (size_t) 3);

	TEST_CONDITION(list_push(list, node[0], node[0]) == -1);


	TEST_CONDITION(list_get_node_byidx(NULL, 1) == (Node) -1);
	TEST_CONDITION(list_get_node_byidx(list, 0) == node[0]);
	TEST_CONDITION(list_get_node_byidx(list, 1) == node[2]);
	TEST_CONDITION(list_get_node_byidx(list, 2) == node[1]);
	TEST_CONDITION(list_get_node_byidx(list, 3) == (Node) -1);
	
	list_pop_front(list);
	list_pop_front(list);
	list_pop_front(list);
			

	node_delete(node[0]);
	node_delete(node[1]);
	node_delete(node[2]);
	list_delete(list);

}

static int list_foreach_sum(void* data, void* retval) {

	*((int*) retval) += (uint64_t) data;
	return 0;
}

void list_test_foreach() {
	List list;
	Node node[3];
	int sum = 0;

	TEST_CONDITION(list_foreach(NULL, (void*) 1, (void*) 1) == -1);
	TEST_CONDITION(list_foreach((void*) 1, NULL, (void*) 1) == -1);

	list = list_create();
	for(int i = 0; i < 3; ++i) {
		node[i] = node_create((void*) 0xA+i);
		list_push_front(list, node[i]);
	}

	TEST_CONDITION(list_foreach(list, list_foreach_sum, &sum) == 0)
	TEST_CONDITION(sum = 0x21)

}