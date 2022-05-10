#include <stddef.h>

struct __Node {
	struct __Node* next;
	const void* data;
};

struct __List {
	struct __Node* head;
	struct __Node* tail;
	size_t size;
};

typedef struct __List* List;
typedef struct __Node* Node;

Node node_create(const void* data);
void node_delete(Node node);
void* node_get_data(Node node);
List list_create();
void list_delete (List list);
size_t list_get_size(List list);
Node list_get_head(List list);
Node list_get_node_byidx(List list, size_t index);
Node list_get_tail(List list);
int list_push(List list, Node node, Node new);
int list_push_front(List list, Node new);
int list_push_back(List list, Node new);
Node list_pop(List list, size_t index);
Node list_pop_back(List list);
Node list_pop_front(List list);
int list_foreach(List list, int(*callback)(void* data, void* retval), void* retval); 


