#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "lib/kernel/hash.h"
struct spt_record {
  void *user_page;
 
  int status;
  bool dirty;
  
  // Swap stuff

  // Filesys stuff

  void *frame_addr;

  struct hash_elem hash_ele;
};



unsigned page_hash(const struct hash_elem *p_, void *aux);
bool page_less(const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux);
struct spt_record *page_lookup(const void *user_page);
void spt_init();
void spt_addPage(void *user_page, void *frame_addr);

#endif // vm/page.h
