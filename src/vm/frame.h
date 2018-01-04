#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "lib/kernel/hash.h"
#include "threads/palloc.h"
#include "threads/synch.h"

struct ft_record {
  void *frame_addr; // frame, created with palloc
  void *user_page; // virtual page occupying the frame
  struct thread *owner; // current thread owner of this frame
  struct hash_elem hash_ele; // hash table element
  struct list_elem list_ele; //for clock algo
  bool pin;
};

unsigned frame_hash(const struct hash_elem *p_, void *aux);
bool frame_less(const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux);
struct ft_record *frame_lookup(const void *frame_addr);
void frame_delete(const void *frame_addr, bool deFrame);
void ft_init(void);
void* allocFrame(enum palloc_flags flags, void *upage);
void freeFrame(void *frame_addr, bool deFrame);

#endif // vm/frame.h
