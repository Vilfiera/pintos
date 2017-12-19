#include "page.h"
struct hash sup_page_table;

// Returns a hash value for page p.
unsigned page_hash(const struct hash_elem *p_, void *aux) {
  const struct spt_record *rec = hash_entry(p_, struct spt_record,
                                            hash_ele);
  return hash_bytes(&rec->user_page, sizeof(rec->user_page));
}

// Returns true if page a precedes page b.
bool page_less(const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux) {
  const struct spt_record *a = hash_entry(a_, struct spt_record, hash_ele);
  const struct spt_record *b = hash_entry(b_, struct spt_record, hash_ele);
  return a->user_page < b->user_page;
}

// Returns the ft_record containing the given frame,
// or a null pointer if no such frame exists.
struct spt_record *page_lookup(const void *user_page) {
  struct spt_record sp_record;
  struct hash_elem *e;
 
  sp_record.user_page = user_page;
  e = hash_find(&sup_page_table, &sp_record.hash_ele);
  return e != NULL ? hash_entry(e, struct spt_record, hash_ele) : NULL;
}

void spt_init() {
  hash_init(&sup_page_table, page_hash, page_less, NULL);
}

// Adds page to supplementary page table, along with its
// corresponding frame.
void spt_addPage(void *user_page, void *frame_addr) {
  struct spt_record *sp_record = malloc(sizeof(struct spt_record));
  sp_record->user_page = user_page;
  sp_record->frame_addr = frame_addr; 

  hash_insert(&sup_page_table, &sp_record->hash_ele);
}
