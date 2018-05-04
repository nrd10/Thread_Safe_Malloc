//Code for functions
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "my_malloc.h"
#include <pthread.h>

//28 as that's how many bits each metadata block took up
#define HEADER 28 

//print out Linked List
unsigned long printList(struct mem_block *head) {
  struct mem_block *temp = (head);
  int count = 0;
  while (temp != NULL) {
    printf("Block %d data starts at address:%p\n", count, temp->header_end);
    printf("This block has size:%d\n", temp->size);
    printf("This block's free bit is:%d\n",temp->free);
    temp = temp->next;
    count++;
  }
}
   

//Call this on previous node and next node
void single_merge(struct mem_block * mynode) {

  struct mem_block *temp = mynode;
  int length = temp->size + HEADER;
  int difference = temp->next->header_end - temp->header_end;
  if (length == difference) {
    temp->size += HEADER + temp->next->size;
     temp->next = temp->next->next;
     if (temp->next!= NULL) {
       temp->next->prev = temp;
     }
  }
}

void merge_blocks(struct mem_block* head) {
  struct mem_block * temp = (head);

  while (temp!= NULL) {
    int length = temp->size + HEADER;

    int difference = temp->next->header_end - temp->header_end;

    if (length==difference) {
      temp->size += HEADER + temp->next->size;
      temp->next = temp->next->next;
      if (temp->next != NULL) {
	temp->next->prev = temp;
      }
      continue;
    }
    temp = temp->next;
  }
}


//sorted insert into Free List (Doubly Linked List)
void sortedInsert(struct mem_block **head, struct mem_block **new_node) {
  struct mem_block *current = NULL;
  //case for  No head
  if ((*head) == NULL){
    (*new_node)->next = NULL;
    (*new_node)->prev = NULL;
    (*head) = (*new_node);
  }
  //case for adding to front
  else if ((*head)->header_end >= (*new_node)->header_end) {
    (*head)->prev = (*new_node);
    (*new_node)->next = (*head);
    (*new_node)->prev = NULL;
    (*head) = (*new_node);
  }
  else {
    (current) = (*head);
    while ((current)->next != NULL && ((current)->next->header_end < (*new_node)->header_end)) {
      current = current->next;
    }
    //case for adding to end
    if (current->next == NULL) {
      (*new_node)->next = current->next;
      (*new_node)->prev = current;
      current->next = (*new_node);
    }
    //case for adding to middle
    else {
      (*new_node)->next = current->next;
      (*new_node)->prev = current;
      current->next->prev = (*new_node);
      current->next = (*new_node);
    }
  }
}

//removes Node from Free List
void RemoveNode(struct mem_block ** head, struct mem_block ** removal) { 
  if (head == NULL) {
    return;
  }
  //Node to remove is the head
  if ((*head) == (*removal)) {
    ((*head) = (*removal)->next);
  }
  //Change next if removal is NOT last node
  if ((*removal)->next != NULL) {
    (*removal)->next->prev = (*removal)->prev;
  }
  //Change prev if removal is NOT 1st node
  if ((*removal)->prev != NULL) {
    (*removal)->prev->next = (*removal)->next;
  }
  //make Node that will now be malloced, have NULL next and prev pointers
  (*removal)->free = 0; //not free anymore!
  (*removal)->next = NULL;
  (*removal)->prev = NULL;
}



struct mem_block * BestFitFind(struct mem_block **head, size_t size){
  int min = 1000;
  int index = 0;
  int final_index = -1;
  struct mem_block * curr = (*head);
  while (curr != NULL) {
    if (((curr->size - size) <= min) && ((curr->size - size) >= 0)) {
      min = curr->size - size;
      final_index = index;
    }
    index++;
    curr = curr->next;
  }
  struct mem_block * new_curr = (*head);
  //no block big enough
  if (final_index == -1) {
    //printf("No block big enough for best fit!\n");
    return NULL;
  }
  else {
    for (int i =0; i < final_index; i++) {
      new_curr = new_curr->next;
    }
    return new_curr;
  }
}

//size is length of the original allocation block e.g. 1000 byte block split into
//size of 800 (requested) and remainder of 200 bytes
void partition(struct mem_block  **head_node, struct mem_block **oversized, size_t size) {

  struct mem_block *remainder = NULL;
  //points at size bytes after data field
  char * pointer = (char *) (*oversized)->header_end;
  for (int i = 0; i < size; i++) {
    pointer++;
  }
  //size of partition is original size - requested size - size of HEADER
  remainder = (struct mem_block *) pointer;
  (remainder)->size = (*oversized)->size - size - HEADER;
  (remainder)->prev = NULL;
  (remainder)->next = NULL;
  (remainder)->free = 1;

  //Acquire LOCK HERE
  pthread_mutex_lock(&mutex);

  //ADD remainder to Free LinkedList
  sortedInsert(head_node, &remainder);

  //Release LOCK HERE
  pthread_mutex_unlock(&mutex);

  //set size of original block
  (*oversized)->size = size;
}


void partition_no_lock(struct mem_block  **head_node, struct mem_block **oversized, size_t size) {

  struct mem_block *remainder = NULL;
  //points at size bytes after data field
  char * pointer = (char *) (*oversized)->header_end;
  for (int i = 0; i < size; i++) {
    pointer++;
  }
  //size of partition is original size - requested size - size of HEADER
  remainder = (struct mem_block *) pointer;
  (remainder)->size = (*oversized)->size - size - HEADER;
  (remainder)->prev = NULL;
  (remainder)->next = NULL;
  (remainder)->free = 1;


  //ADD remainder to Free LinkedList
  sortedInsert(head_node, &remainder);


  //set size of original block
  (*oversized)->size = size;
}




//call this function when we don't find the right fit for a block
struct mem_block * addblock(size_t my_size) {
  struct mem_block * break_point;

  //ACQUIRE LOCK: SBRK IS NOT THREAD SAFE
  pthread_mutex_lock(&mutex);
  //increments program break by the size of HEADER and amount of mem. requested
  void * result = sbrk(HEADER + my_size);
  //RELEASE LOCK: lose lock for sbrk call
  pthread_mutex_unlock(&mutex);

  break_point = (struct mem_block *) result;
    //error checking
    if (result == (void*) - 1) { 
      perror("sbrk");
      return NULL;
    }
    else {
      //set fields of header of new memory block
      break_point->size = my_size;
      break_point->next = NULL;
      break_point->prev = NULL;
      break_point->free = 0;   
      return break_point;
    }
}

void printblock(struct mem_block * A) {
  printf("Block size is:%lu\n", A->size);
  printf("Block's free bit is:%d\n", A->free);
  printf("Data for this block starts at address:%p\n", A->header_end);
}


size_t align(size_t x) {
  //gets nearest multiple of 4
  size_t y;
  y = (x/4)*4 + 4;
  return y;
}


void *ts_malloc_lock(size_t size) {
  struct mem_block * BLOCK;
  size_t mysize = align(size);
  
  //PLACE LOCK HERE: ACQUIRE LOCK HERE
  pthread_mutex_lock(&mutex);

  //Find the block
  BLOCK = BestFitFind(&base, mysize);

  //split the block if it is not NULL and it has the space for it
  if (BLOCK != NULL) {
    
    //remove the node from the free list
    RemoveNode(&base, &BLOCK);
    //Release LOCK
    pthread_mutex_unlock(&mutex);

    if ((BLOCK->size - mysize) >= (HEADER+4)) {
	partition(&base, &BLOCK, mysize);
      }
  }
  else {

    //UNLOCK here in ELSE case
    pthread_mutex_unlock(&mutex);
    
    //extend the heap as there are no fitting blocks
    BLOCK = addblock(mysize);
  }
  //return start of the memory block!!
  return (BLOCK->header_end);
}

void *ts_malloc_nolock(size_t size) {
  struct mem_block * BLOCK;
  size_t mysize = align(size);
 
  //Find the block using threadsafe HEAD
  BLOCK = BestFitFind(&thread_base, mysize);
  //split the block if it is not NULL and it has the space for it
  if (BLOCK != NULL) {
    if ((BLOCK->size - mysize) >= (HEADER+4)) {
	partition_no_lock(&thread_base, &BLOCK, mysize);
      }
    //remove the node from the free list
    RemoveNode(&thread_base, &BLOCK);
  }
  else {
    //extend the heap as there are no fitting blocks
    BLOCK = addblock(mysize);
  }
  //return start of the memory block!!
  return (BLOCK->header_end);
}


void ts_free_lock(void * ptr) {
  //get to actual HEADER of the block since malloc returns data section
  //where we store memory
  char * pointer = (char *) ptr;
  for (int i =0; i<28; i++) {
    pointer--;
  }
  struct mem_block * correct_start = (struct mem_block *) pointer;
   correct_start->free = 1;
   
   //PLACE LOCK HERE!: we are inserting and merging so free list manipulations
   //require locks
   pthread_mutex_lock(&mutex);

 //insert the memory block back into the Free list
  sortedInsert(&base, &correct_start);
 
  //merge block with one before and one after if possible
  merge_blocks(base);
    
  /*   
   if(correct_start->prev != NULL) {
     single_merge(correct_start->prev);
   }
   if(correct_start->next != NULL) {
     single_merge(correct_start->next);
   }
   
  */
   //UNLOCK HERE
  pthread_mutex_unlock(&mutex);
}

void ts_free_nolock(void *ptr) {
  //get to actual HEADER of the block since malloc returns data section
  //where we store memory
  char * pointer = (char *) ptr;
  for (int i =0; i<28; i++) {
    pointer--;
  }
  struct mem_block * correct_start = (struct mem_block *) pointer;
  correct_start->free = 1;
   
  //insert the memory block back into the Free list
  sortedInsert(&thread_base, &correct_start);
  //merge block with one before and one after if possible
  merge_blocks(thread_base);

  /*
  if(correct_start->prev != NULL) {
    single_merge(correct_start->prev);
   }
  if(correct_start->next != NULL) {
    single_merge(correct_start->next);
  }
  */

}




