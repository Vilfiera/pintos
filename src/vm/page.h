#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "lib/kernel/hash.h"
#include "filesys/off_t.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#define STATUS_ZERO 0
#define STATUS_FRAME 1
#define STATUS_SWAP 2
#define STATUS_FILE 3

struct spt_record {
  void *user_page;
 
  uint8_t status;
  bool dirty;
  bool pinned;
  
  // Swap stuff
  size_t swap_index;

  // Filesys stuff
  struct file *file;
  off_t offset;
  size_t read_bytes;
  size_t zero_bytes;
  bool writable;

  void *frame_addr;

  struct hash_elem hash_ele;
};



unsigned page_hash(const struct hash_elem *p_, void *aux);
bool page_less(const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux);
struct spt_record *page_lookup(struct hash *spt, const void *user_page);
bool spt_setDirty(struct hash *spt, void *user_page, bool dirtyBit);
struct hash* spt_init();

bool spt_addFrame(struct hash *spt, void *user_page, void *frame_addr);
bool spt_addZeroPage(struct hash *spt, void *user_page);
bool spt_addSwap(struct hash *spt, void *user_page, size_t swap_index);
bool spt_addFile(struct hash *spt, void *user_page, struct file *file,
                  off_t offset, size_t read_bytes, size_t zero_bytes, bool writable);

bool spt_load(struct hash* spt, uint32_t pagedir, void* user_page);
#endif // vm/page.h

