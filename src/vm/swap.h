#ifndef VM_SWAP_H
#define VM_SWAP_H

//initialize the swap 
void swap_init (void);

//swap out the page to the swap disk
uint32_t swap_out (void *page);

//swap in the page to the main mem location page
void swap_in (uint32_t swap_index, void *page);

//delete the page at swap index
void swap_delete (uint32_t swap_index);

#endif

