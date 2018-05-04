#define test_H
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

struct mem_block {
  size_t size;
  struct mem_block *prev;
  struct mem_block *next;
  int free;
  char header_end[1]; //marks the end of the header and the beginning of addresses available to heap
};

//Global HEAD NODE
struct mem_block * base;

//Global lock
static pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;

//Global thread_local HEAD NODE
__thread struct mem_block * thread_base;


//GLobal count of data segments

unsigned long printList(struct mem_block *head);

void single_merge(struct mem_block * mynode);

void sortedInsert(struct mem_block **head, struct mem_block **new_node);

void RemoveNode(struct mem_block **head, struct mem_block ** removal);

struct mem_block * BestFitFind(struct mem_block **free_head, size_t mysize);

void partition(struct mem_block **head_node, struct mem_block **oversized, size_t size);

struct mem_block * addblock(size_t size);

void printblock(struct mem_block * A);

size_t align(size_t x);

void *ts_malloc_lock(size_t size);

void *ts_malloc_nolock(size_t size);

void ts_free_lock(void * ptr);

void ts_free_nolock(void * ptr);
