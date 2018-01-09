#include <hash.h>
#include <list.h>
#include <stdio.h>
#include "lib/kernel/hash.h"
#include "lib/kernel/list.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "vm/swap.h"
static struct hash frame_table;
static struct lock mutex;

//for clock algorithm
static struct list frame_list;
static struct list_elem *clock;

struct ft_record* clock_next (void);
struct ft_record* select_frame_evict (uint32_t *pagedir);

// Returns a hash value for frame p.
unsigned frame_hash(const struct hash_elem *p_, void *aux) {
  const struct ft_record *rec = hash_entry(p_, struct ft_record,
                                            hash_ele);
  return hash_bytes(&rec->frame_addr, sizeof(rec->frame_addr));
}

// Returns true if frame a precedes frame b.
bool frame_less(const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux) {
  const struct ft_record *a = hash_entry(a_, struct ft_record, hash_ele);
  const struct ft_record *b = hash_entry(b_, struct ft_record, hash_ele);
  return a->frame_addr < b->frame_addr;
}

// Returns the ft_record containing the given frame,
// or a null pointer if no such frame exists.
struct ft_record *frame_lookup(const void *frame_addr) {
  struct ft_record frame_record;
  struct hash_elem *e;
  
  frame_record.frame_addr = frame_addr;
  e = hash_find(&frame_table, &frame_record.hash_ele);
  return e != NULL ? hash_entry(e, struct ft_record, hash_ele) : NULL;
}

//initialize the hash table and mutex
void ft_init() {
  hash_init(&frame_table, frame_hash, frame_less, NULL);
  lock_init(&mutex);
  list_init(&frame_list);
  clock = NULL;
}


void* allocFrame(enum palloc_flags flags, void *upage) {
  lock_acquire(&mutex);
  void *frame_addr = palloc_get_page(PAL_USER | flags);
  if (frame_addr == NULL) {
    // Out of frames, need to evict a page.
    //swap out the page
    struct ft_record *ft_evict = select_frame_evict ( thread_current () -> pagedir);
    pagedir_clear_page (ft_evict -> owner -> pagedir, ft_evict->user_page);
    bool is_dirty = false;
    is_dirty = is_dirty || pagedir_is_dirty(ft_evict -> owner -> pagedir, ft_evict -> user_page);
    is_dirty = is_dirty || pagedir_is_dirty(ft_evict -> owner -> pagedir, ft_evict -> frame_addr);

    uint32_t swap_id = swap_out (ft_evict->frame_addr);
    spt_addSwap (ft_evict -> owner -> sup_pt, ft_evict -> user_page, swap_id);
    spt_setDirty (ft_evict -> owner -> sup_pt, ft_evict -> user_page, is_dirty);
    frame_delete(ft_evict -> frame_addr, true);
    frame_addr = palloc_get_page (PAL_USER | flags);
    ASSERT (frame_addr != NULL);
    
  }
  struct ft_record *resultFrame = malloc(sizeof(struct ft_record));
  if (resultFrame == NULL) {
    lock_release(&mutex);
    return NULL;
  }
  // Initialize ft entry.
  resultFrame->frame_addr = frame_addr;
  resultFrame->user_page = upage;
  resultFrame->owner = thread_current();
  resultFrame->pin = true;

  // Insert ft entry into frame table.
  hash_insert(&frame_table, &resultFrame->hash_ele);
  list_push_back(&frame_list, &resultFrame->list_ele); 

  lock_release(&mutex);
  return frame_addr;
}

// Deletes the ft_record containing the given frame,
// or does nothing if no such frame exists.
void frame_delete(const void *frame_addr, bool deFrame) {
  ASSERT (lock_held_by_current_thread ( &mutex) == true);
  ASSERT (is_kernel_vaddr (frame_addr));
  ASSERT (pg_ofs (frame_addr) == 0);
  struct ft_record f_record;
  f_record.frame_addr = frame_addr;
  struct hash_elem *e = hash_find(&frame_table, & (f_record.hash_ele));
  if (e == NULL){
  	PANIC("Page Not found.");
   }
  
  struct ft_record *nf_record = hash_entry(e , struct ft_record, hash_ele);
  hash_delete(&frame_table, &nf_record->hash_ele);
  list_remove(&nf_record->list_ele);
  if (deFrame) {
    palloc_free_page( frame_addr);
  }
  free(nf_record);
}

struct ft_record* select_frame_evict (uint32_t *pagedir){
 size_t n = hash_size(&frame_table);
 if ( n == 0) PANIC ("Nothing is in frame table.");
 size_t i;
 for ( i = 0; i <= n + n; ++i){
  struct ft_record *ft_record = clock_next();
  //if pinned, continuew
  if ( ft_record -> pin ) continue;
  //if referenced, give it a second chance
  else if (pagedir_is_accessed(pagedir, ft_record->user_page)){
   pagedir_set_accessed(pagedir,ft_record->user_page, false);
   continue;
  }
  return ft_record;
 }
 PANIC ("Can not evict any frame.");
}

struct ft_record* clock_next (){
 if (list_empty(&frame_list)) PANIC("Frame table is empty.");
 if ( clock == NULL)
	clock = list_begin(&frame_list);
 else clock = list_next (clock);
  // If we reach the tail we need to loop back around.
  if (clock == list_end(&frame_list))
    clock = list_begin(&frame_list);

 struct ft_record *ft_record = list_entry(clock, struct ft_record, list_ele);
 return ft_record;
}

static void 
set_pin ( void *frame_addr, bool value){
 lock_acquire (&mutex);
 struct ft_record ft_tmp;
 ft_tmp.frame_addr = frame_addr;
 struct hash_elem *h = hash_find(&frame_table, &(ft_tmp.hash_ele));
 if ( h == NULL){
  PANIC ("Frame does not exist.");
 } 

 struct ft_record *ft_r;
 ft_r = hash_entry (h, struct ft_record, hash_ele);
 ft_r -> pin = value;
 lock_release (&mutex);
}

void frame_unpin (void *frame_addr){
 set_pin(frame_addr, false);
}

void frame_pin (void *frame_addr){
 set_pin (frame_addr, true);
}
void freeFrame(void *frame_addr, bool deFrame) {
  lock_acquire(&mutex);
  frame_delete(frame_addr, deFrame);
  lock_release(&mutex);
}

