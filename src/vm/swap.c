#include <bitmap.h>
#include "threads/vaddr.h"
#include "devices/block.h"
#include "vm/swap.h"

static struct block *swap_block;
static struct bitmap *swap_present;

static const size_t SECTORS_P_PAGE = PGSIZE/BLOCK_SECTOR_SIZE;

//possible number of pages that can be swap
static size_t swap_size;

void swap_init()
{
 swap_block = block_get_role (BLOCK_SWAP);
 if ( swap_block == NULL){
	NOT_REACHED();
 }
 //num of pages in swap disk
 swap_size = block_size (swap_block) / SECTORS_PER_PAGE;
 //create bitmap for the swap disk
 swap_present = bitmap_create(swap_size);
 bitmap_set_all(swap_present, true);
}

uint32_t swap_out (void *page){

 //is the page in user space 
 ASSERT(page >= PHYS_BASE);
 //find block that can be use for swap
 size_t swap_index = bitmap_scan (swap_present, 0, 1, true);

 for (size_t i = 0; i < SECTORS_P_PAGE; ++i){
  block_write(swap_block, swap_index * SECTORS_PER_PAGE + i,
  		page + (BLOCK_SECTOR_SIZE * i) );
 }
 //the region is not present anymore
 bitmap_set(swap_present, swap_index, false);
 return swap_index;
}

void swap_in ( uint32_t swap_index, void *page){
 //validate the page address
 ASSERT( page >= PHYS_BASE);

 //validate swap_index
 ASSERT (swap_index < swap_size);
 if ( bitmap_test (swap_present, swap_index) == true){
  PANIC("Invalid access in swap disk");
 }

 for (size_t i = 0; i < SECTORS_P_PAGE; ++i)
 {
  block_read (swap_block, swap_index * SECTORS_P_PAGE +i, page + (BLOCK_SECTORS_SIZE * i) );
 }
 bitmap_set (swap_present, swap_index, true);
}


void swap_delete (uint32_t swap_index){
 //validate swap 
 ASSERT (swap_index < swap_size);
 if (bitmap_test (swap_present, swap_index) == true){
  PANIC ( "Invalid access in the swap disk"); 
 }
 bitmap_set (swap_present, swap_index, true);
}

























