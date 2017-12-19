#include "vm/frame.h"
struct hash frame_table;
struct lock mutex;

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

// Deletes the ft_record containing the given frame,
// or does nothing if no such frame exists.
void frame_delete(const void *frame_addr) {
  struct ft_record frame_record;
  struct hash_elem *e;
  
  frame_record.frame_addr = frame_addr;
  e = hash_delete(&frame_table, &frame_record.hash_ele);
}

void ft_init() {
  hash_init(&frame_table, frame_hash, frame_less, NULL);
  lock_init(&mutex);
}

void* allocFrame(enum palloc_flags flags, void *upage) {
  lock_acquire(&mutex);
  void *frame_addr = palloc_get_page(flags);
  if (frame_addr == NULL) {
    // Out of frames, need to evict a page.
  }
  struct ft_record *resultFrame = malloc(sizeof(struct ft_record));
  if (resultFrame == NULL) {
    lock_release(&mutex);
    free(resultFrame);
    return NULL;
  }
  // Initialize ft entry.
  resultFrame->frame_addr = frame_addr;
  resultFrame->user_page = upage;
  resultFrame->owner = thread_current();

  // Insert ft entry into frame table.
  hash_insert(&frame_table, &resultFrame->hash_ele);

  lock_release(&mutex);
  return frame_addr;
}

void freeFrame(void *frame_addr) {
  lock_acquire(&mutex);
  frame_delete(frame_addr);
  lock_release(&mutex);
}

