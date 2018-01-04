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

void spt_free_elem(struct hash_elem *p_, void *aux UNUSED) {
  struct spt_record *sp_record = hash_entry(p_, struct spt_record, hash_ele);
  if (sp_record->status == STATUS_FRAME && sp_record->frame_addr) {
    freeFrame(sp_record->frame_addr, false);
  } else if (sp_record->status == STATUS_SWAP) {
    //free from swap table
  }
  free(sp_record);
}
// Returns the spt_record containing the given frame,
// or a null pointer if no such frame exists.
struct spt_record *page_lookup(struct hash *spt, const void *user_page) {
  struct spt_record sp_record;
  struct hash_elem *e;
 
  sp_record.user_page = user_page;
  e = hash_find(spt, &sp_record.hash_ele);
  return e != NULL ? hash_entry(e, struct spt_record, hash_ele) : NULL;
}

bool spt_setDirty(struct hash *spt, void *user_page, bool dirtyBit) {
  struct spt_record *sp_record = page_lookup(spt, user_page);
  if (!sp_record) {
    return false;
  }
  sp_record->dirty = dirtyBit;
  return true;

}

struct hash* spt_init() {
  struct hash *sup_page_table = (struct hash*) malloc(sizeof(struct hash));
  hash_init(sup_page_table, page_hash, page_less, NULL);
  return sup_page_table;
}

void spt_free(struct hash *spt) {
  if (spt != NULL) {
    hash_destroy(spt, spt_free_elem);
  }
}
// Adds page to supplementary page table, along with its
// corresponding frame. Returns true if entry was successfully added.
bool spt_addFrame(struct hash *spt, void *user_page, void *frame_addr) {
  struct spt_record *sp_record = malloc(sizeof(struct spt_record));

  sp_record->user_page = user_page;
  sp_record->status = STATUS_FRAME;
  sp_record->dirty = false;  
  sp_record->frame_addr = frame_addr;
  
  // Tracks whether we already have an entry for this page.
  struct hash_elem *result;  
  result = hash_insert(spt, &sp_record->hash_ele);
  if (result) {
    // Entry already exists
    free(sp_record);
    return false;
  } else {
    // Entry was successfully added
    return true;
  }
}

// Adds an all-zero page to supplementary page table.
// Returns true if successfully added
bool spt_addZeroPage(struct hash *spt, void *user_page) {
  struct spt_record *sp_record = malloc(sizeof(struct spt_record));

  sp_record->user_page = user_page;
  sp_record->status = STATUS_ZERO;
  sp_record->dirty = false;
  
  
  // Tracks whether we already have an entry for this page.
  struct hash_elem *result;  
  result = hash_insert(spt, &sp_record->hash_ele);
  if (result) {
    // Entry already exists; should never happen for zeroed pages?
    free(sp_record);
    return false;
  } else {
    // Entry was successfully added
    return true;
  }
}

// Changes a page's entry to indicate that it is now located in a swap slot.
// Returns true if successful.
bool spt_addSwap(struct hash *spt, void *user_page, size_t swap_index) {
  struct spt_record *sp_record;
  sp_record = page_lookup(spt, user_page);

  // Didn't find the page in our spt
  if (!sp_record) {
    return false;
  }

  // Updates info, removes frame association
  sp_record->status = STATUS_SWAP; 
  sp_record->swap_index = swap_index;
  sp_record->frame_addr = NULL;

  return true;  
}

// Adds a page located in a file to the supplementary page table.
// Returns true if successful.
bool spt_addFile(struct hash *spt, void *user_page, struct file *file,
                  off_t offset, size_t read_bytes, size_t zero_bytes, bool writable) {
  struct spt_record *sp_record = malloc(sizeof(struct spt_record));

  sp_record->user_page = user_page;
  sp_record->status = STATUS_FILE;
  sp_record->dirty = false;
  sp_record->file = file;
  sp_record->offset = offset;
  sp_record->read_bytes = read_bytes;
  sp_record->zero_bytes = zero_bytes;
  sp_record->writable = writable;
  
  // Tracks whether we already have an entry for this page.
  struct hash_elem *result;  
  result = hash_insert(spt, &sp_record->hash_ele);
  if (result) {
    // Entry already exists
    free(sp_record);
    return false;
  } else {
    // Entry was successfully added
    return true;
  }
}

// Loads desired page back into a frame, and updates page tables.
// Returns true if successful.
bool spt_load(struct hash* spt, uint32_t pagedir, void* user_page) {
  struct spt_record *sp_record;
  
  // checks if provide address is valid, by checking spt.
  sp_record = page_lookup(spt, user_page);
  if (!sp_record) {
    return false;
  }
  
  // Gets a frame to store the page.
  void *frame_addr = allocFrame(PAL_USER, user_page);
  // Unable to get a frame
  if (!frame_addr) {
    return false;
  } 

  // Keeps track of writable status for pagedir_set_page. True by default.
  bool writable = true;
  uint8_t status = sp_record->status;
  if (status == STATUS_ZERO) {
    // All zero page, zeroes out our frame.
    memset(frame_addr, 0, PGSIZE);
  } else if (status == STATUS_FRAME) {
    // Already in a frame, nothing needs to be done.
  } else if (status == STATUS_SWAP) {
    // Loads page from our swap table to our frame.
    swap_in(sp_record->swap_index, frame_addr);
  } else if (status == STATUS_FILE) {
    // Seeks to correct place in the file.
    file_seek(sp_record->file, sp_record->offset);

    // Reads from file.
    int bytesRead = file_read(sp_record->file, frame_addr, sp_record->read_bytes);
    if (bytesRead != (int) sp_record->read_bytes) {
      // Something went wrong, didn't read correct number of bytes.
      freeFrame(frame_addr, true);
      return false; 
    }
    // Zeroes out the rest of the page.
    memset(frame_addr + bytesRead, 0, sp_record->zero_bytes);

    // Need to keep track of whether file is writable or not.
    writable = sp_record->writable;
  }

  // Maps page to frame in pagedir.
  if (!pagedir_set_page(pagedir, user_page, frame_addr, writable)) {
    freeFrame(frame_addr, true);
    return false;
  }
  // Frame cannot be dirty; we just installed the page.
  pagedir_set_dirty(pagedir, user_page, false);

  // Updates information on our supplementary page table.
  sp_record->frame_addr = frame_addr;
  sp_record->status = STATUS_FRAME;

  frame_unpin(frame_addr);
  return true;
}

// Pins the corresponding frame to the given page.
void spt_pinPage(struct hash *spt, void *user_page) {
  struct spt_record *sp_record = page_lookup(spt, user_page);
  if (sp_record && sp_record->status == STATUS_FRAME)  {
    frame_pin(sp_record->frame_addr);
  }
}

// Unpins the corresponding frame to the given page.
void spt_unpinPage(struct hash *spt, void *user_page) {
  struct spt_record *sp_record = page_lookup(spt, user_page);
  if (sp_record && sp_record->status == STATUS_FRAME)  {
    frame_unpin(sp_record->frame_addr);
  }
}

void spt_unmapFile(struct hash *spt, void *pagedir, void *user_page,
                    struct file *f, off_t offset, size_t num_bytes) {
  struct spt_record *sp_record = page_lookup(spt, user_page);
  if (!sp_record) {
    PANIC("munmap couldn't find the desired page");
  }

  if (sp_record->status == STATUS_FRAME) {
    frame_pin(sp_record->frame_addr);
  }

  
  if (sp_record->status == STATUS_FRAME) {
    // If dirty, write directly from frame to the file.
    if (sp_record->dirty || pagedir_is_dirty(pagedir, sp_record->user_page) ||
        pagedir_is_dirty(pagedir, sp_record->frame_addr)) {
      file_write_at(f, sp_record->user_page, num_bytes, offset);
    }

    frame_delete(sp_record->frame_addr, true);
    pagedir_clear_page(pagedir, sp_record->user_page);
  } else if (sp_record->status == STATUS_SWAP) {
    // If dirty, we need to swap the page back in, then write to the file.
    if (sp_record->dirty || pagedir_is_dirty(pagedir, sp_record->user_page)) {
      // This frame is only for our usage, so it's not allocated with PAL_USER
      void *temp_frame = palloc_get_page(0);

      swap_in(sp_record->swap_index, temp_frame);
      file_write_at(f, temp_frame, num_bytes, offset);
      
      palloc_free_page(temp_frame);
    // If not, free the swap slot
    } else {
      swap_delete(sp_record->swap_index);
    }
  }
  // Remove memory mapped file from our page table.
  hash_delete(spt, sp_record->hash_ele);
}
