#include "headers/List.h"
#include <errno.h>
#include <stdlib.h>

Node node_create(const void* data) {
	Node node = node_calloc();
	if (node) {
		node->data = data;
	}

	return node;
}

void node_delete(Node node) {
	free(node);
}

void* node_get_data(Node node) {
	
	if(!node) 
		return 0;

	return (void*) node->data;

}

List list_create() {
	return list_calloc();
}

void list_delete(List list) {
	free(list);
}

size_t list_get_size(List list) {
	if(!list)
		return -1;
	
	return list->size;
}

Node list_get_head(List list) {
	if(!list)
		return (Node) -1;

	return list->head;
}

Node list_get_node_byidx(List list, size_t index) {

	if(!list || index >= list->size)
		return (Node) -1;

	Node node = list->head;

	for(size_t i = 0; i < index && node; ++i) {
		node = node->next;
	}

	return node;
}

Node list_get_tail(List list) {

	if(!list)
		return (Node) -1;

	return list->tail;
}

int list_push(List list, Node node, Node new) {
	if(!list || !node || !new || node == new)
		return -1;

	new->next = node->next;
	node->next = new;
	++list->size;
	return 0; 
}

int list_push_back(List list, Node new) {

	if(!list || !new)
		return -1;

	if(list->tail) {
		list->tail->next = new;
		list->tail = new;
	} else {
		list->head = new;
		list->tail = new;
	}

	++list->size;
	return 0;  
}

int list_push_front(List list, Node new) {

	if(!list || !new)
		return -1;
	
	new->next = list->head;

	if(!new->next)
		list->tail = new;

	list->head = new;
	++list->size;
	return 0;
}

Node list_pop_front (List list) {

	if(!list || !list->head)
		return (Node) -1;

	Node pop = list->head;
	list->head = list->head->next;
	pop->next = NULL;

	if(list->tail == pop)
		list->tail = NULL;

	--list->size;
	return pop;
}

int list_foreach(List list, int(*callback)(void* data, void* retval), void* retval) {

	if(!list || !callback)
		return -1;

	for(Node node = list->head; node; node = node->next) {

		int ret = callback((void*) node->data, retval);
	
		if(ret)
			return ret;
	}

	return 0;
}


Node node_calloc() {
	if (rand()%1000 == 1)
		return NULL;
	Node node = calloc(1, sizeof(*node));

	return node;
}  

List list_calloc() {
	if (rand()%1000 == 1)
		return NULL;
	List list = calloc(1, sizeof(struct __List));

	return list;
}


