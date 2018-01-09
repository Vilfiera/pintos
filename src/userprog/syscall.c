#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "filesys/off_t.h"
#include "lib/string.h"
#include "threads/malloc.h";
#include "threads/palloc.h";
#include "threads/vaddr.h";
#include "lib/kernel/list.h";
#include "threads/thread.h";
/* Typical return values from main() and arguments to exit(). */
#define EXIT_SUCCESS 0          /* Successful execution. */
#define EXIT_FAILURE 1          /* Unsuccessful execution. */
#define MAX_STACK_SIZE 0x800000

struct lock filesys_mutex;

static void syscall_handler (struct intr_frame *);

static void parse_args(void* esp, int* argBuf, int numToParse);
static void valid_ptr(void* user_ptr, void* esp);
static void valid_buf(char* buf, unsigned size, void *esp);
static void valid_string(void* string, void* esp);
static struct mmap_record* find_mmap_record (int id);
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}
 
/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

bool munmap (int id);

void
syscall_init (void) 
{
  lock_init(&filesys_mutex);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  int args[3];
  void* esp = f->esp;
  valid_ptr(esp, esp);
  uint32_t current_syscall = *(uint32_t*)esp;
  
  switch(current_syscall)
  {
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
    parse_args(esp, &args[0], 1);
    exit ((int) args[0]);
		break;
	case SYS_EXEC:
		parse_args(esp, &args[0], 1);
    valid_ptr(args[0], esp);
    valid_string((void*) args[0], esp);
    f->eax = exec ((const char*) args[0]);
		break;
	case SYS_WAIT:
		parse_args(esp, &args[0], 1);
    f->eax = wait ((pid_t) args[0]);
		break;
  case SYS_CREATE:
		parse_args(esp, &args[0], 2);
    valid_ptr(args[0], esp);
    valid_string((void*) args[0], esp);
    f->eax = create((const char*) args[0], (unsigned) args[1]);
    break;
	case SYS_REMOVE:
		parse_args(esp, &args[0], 1);
    valid_ptr(args[0], esp);
    valid_string((void*) args[0], esp);
    f->eax = remove ((const char*) args[0]);
		break;
	case SYS_OPEN:
		parse_args(esp, &args[0], 1);
    valid_ptr(args[0], esp);
    valid_string((void*) args[0], esp);
    f->eax = open ((const char*) args[0]);
		break;
	case SYS_FILESIZE:
		parse_args(esp, &args[0], 1);
    f->eax = filesize ((int) args[0]);
		break;
	case SYS_READ:
		parse_args(esp, &args[0], 3);
    valid_ptr(args[1], esp);
    valid_buf((char*) args[1], (unsigned)args[2], esp);
    f->eax = read ((int) args[0], (void*) args[1], (unsigned) args[2]);
		break;
	case SYS_WRITE:
		parse_args(esp, &args[0], 3);
    valid_ptr(args[1], esp);
    valid_buf((char*) args[1], (unsigned) args[2], esp);
	  f->eax = write((int) args[0], (const void*) args[1], (unsigned) args[2]);
		break;
	case SYS_SEEK:
		parse_args(esp, &args[0], 2);
    seek ((int) args[0], (unsigned) args[1]);
		break;
	case SYS_TELL:
		parse_args(esp, &args[0], 1);
    f->eax = tell ((int) args[0]);
		break;
	case SYS_CLOSE:
		parse_args(esp, &args[0], 1);
    close ((int) args[0]);
		break;
  case SYS_MMAP:
		parse_args(esp, &args[0], 2);
		//valid_ptr(args[1], esp);
		f -> eax = mmap((int) args[0], (void*) args[1], esp);
		break;
	case SYS_MUNMAP:
		parse_args(esp, &args[0], 1);
		munmap((int) args[0]);
		break;
	default:	
		exit(-1);	
  }
  thread_yield();
}

void halt (void) 
{
 shutdown_power_off ();
}
void exit (int status) 
{
  struct thread *t = thread_current();
  if (isAlive(t->parent)) {
    struct list *listOfParent = &(t->parent->childlist);
    struct list_elem *e = list_begin(listOfParent);
    while (e != list_end(listOfParent)) {
      struct child_record *cr = list_entry(e, struct child_record, elem);
      if (cr->id == t->tid) {
        cr->retVal = status;
        cr->child = NULL; 
        break;
      }
      e = list_next(e);
    }
  }
  char *save_ptr;
  char *file_name = strtok_r(t->name, " ", &save_ptr); 
  printf("%s: exit(%d)\n", t->name, status);
  if ( t->parent_wait)
   sema_up(&(t->parent->child_sema));
  file_close (t->exefile);
  // frees list of children.
  struct list *templist = &(t -> childlist);
  while (!list_empty (templist)) {
      struct list_elem *cr_elem = list_pop_front (templist);
      struct child_record *tempCR = list_entry(cr_elem, struct child_record, elem);
      free(tempCR);
  }
  // frees list of file descriptors.
  templist = &(t->fd_entries);
  while (!list_empty(templist)) {
    struct list_elem *fd_elem = list_pop_front(templist);
    struct file_record *tempFR = list_entry(fd_elem, struct file_record, elem);
    file_close(tempFR->cfile);
    free(tempFR);
  }
  thread_exit ();
}
pid_t exec (const char *cmd_line) 
{
  int result = process_execute(cmd_line);
  sema_down(&(thread_current()->child_load_sema));
  if (thread_current()->child_status == -1) {
    return -1;
  }
  return result;
}
int wait (pid_t pid) 
{
 return process_wait (pid);
}
bool create (const char *file, unsigned initial_size) 
{
  lock_acquire(&filesys_mutex);
  bool result = filesys_create(file, initial_size); 
  lock_release(&filesys_mutex);
  return result;
}
bool remove (const char *file) 
{

  lock_acquire(&filesys_mutex);
  bool result = filesys_remove(file);
  lock_release(&filesys_mutex);
  return result;
}
int open (const char *file) 
{
  
  lock_acquire(&filesys_mutex);
  struct file_record *cfileRecord;
  cfileRecord = malloc ( sizeof (struct file_record));
  if (cfileRecord == NULL) {
    lock_release(&filesys_mutex);
    return -1;
  }
  struct file *currentfile = filesys_open (file);
  if ( currentfile == NULL) {
    free(cfileRecord);
    lock_release(&filesys_mutex);
    return -1;
  }
  struct thread *t = thread_current();
  cfileRecord -> cfile = currentfile;
  cfileRecord -> fd = t -> total_fd;
  t -> total_fd = t -> total_fd + 1;
  list_push_back(&(t-> fd_entries), &(cfileRecord ->elem));
  int result = cfileRecord -> fd;
  lock_release(&filesys_mutex);
  return result;
}
int filesize (int fd) 
{
  lock_acquire(&filesys_mutex);
  struct file *tempfile;
  tempfile = file_ptr(fd);
  if (tempfile != NULL) {
    int result = file_length(tempfile);
    lock_release(&filesys_mutex);
  	return result;
  }
  lock_release(&filesys_mutex);
  return -1;
}
int read (int fd, void *buffer, unsigned length) 
{
 lock_acquire(&filesys_mutex);
  if (fd == 1) {
    lock_release(&filesys_mutex);
    return -1;
  }
 if (fd != 0)
 {
   struct file *tempfile = NULL;
   tempfile = file_ptr(fd);
   if (tempfile == NULL) {
      lock_release(&filesys_mutex);
 	    return -1;
   }
   
    load_pin_pages (buffer, length);
    int result = file_read (tempfile, buffer, length);
	unpin_pages (buffer, length);
    lock_release(&filesys_mutex);
    return result;
 }
 else{
    int bytesRead = 0;
    while (bytesRead < length) {
 	    input_getc();
      bytesRead++;
    }
    lock_release(&filesys_mutex);
    return bytesRead; 
	}
}
int write (int fd, const void *buffer, unsigned length)
{
lock_acquire(&filesys_mutex);
  if (fd == 0) {
    lock_release(&filesys_mutex);
    return -1;
  }
if (fd != 1)
 {
  struct file *tempfile;
  tempfile = file_ptr(fd);
  if (tempfile == NULL) {
    lock_release(&filesys_mutex);
    return -1;
  }
  load_pin_pages (buffer, length);
 	int result = file_write (tempfile, buffer, length);
	unpin_pages (buffer, length);
  lock_release(&filesys_mutex);
  return result;
 }
 else {
  putbuf(buffer, length);
  lock_release(&filesys_mutex);
  return length;
 }

}
void seek (int fd, unsigned position) 
{
  lock_acquire(&filesys_mutex);
  struct file *tempfile;
  tempfile = file_ptr(fd);
 	if (tempfile != NULL) {
    file_seek (tempfile, position);
  }
  lock_release(&filesys_mutex);
}
unsigned tell (int fd) 
{
  lock_acquire(&filesys_mutex);
  struct file *tempfile;
  tempfile = file_ptr(fd);
  if (tempfile == NULL) {
    lock_release(&filesys_mutex);
    return -1;
  }
  unsigned result = file_tell(tempfile);
  lock_release(&filesys_mutex);
  return result;
}
void close (int fd) 
{
  lock_acquire(&filesys_mutex);
   struct thread *t = thread_current();
   struct list *templist = &(t -> fd_entries);
   struct file_record *tempfileRd;
   struct list_elem *e;
   for (e = list_begin (templist); e != list_end (templist);
           e = list_next (e))
   {
	  tempfileRd = list_entry(e,struct file_record, elem);
	  if (tempfileRd->fd == fd) {
      file_close(tempfileRd -> cfile);
      list_remove(e);
      free(tempfileRd);
      break;
		}
   }
  lock_release(&filesys_mutex);
}



int mmap(int fd, void *user_page, void* esp){
	if (!is_user_vaddr(user_page) || user_page < USER_VADDR_BOUND ||
       user_page == NULL || pg_ofs (user_page) != 0) return -1;
	if (fd < 1) return -1;
	struct thread *t = thread_current();
	lock_acquire (&filesys_mutex);
	
	//opening file
	struct file *f = file_ptr(fd);
	//struct file_record *file_r = file_ptr (fd);
	if (f != NULL){
		f = file_reopen (f);
	}
	if ( f == NULL ){
		lock_release (&filesys_mutex);
		return -1;
	}
	size_t f_size = file_length (f);
	if ( f_size == 0){
		lock_release(&filesys_mutex);
		return -1;
	}
	
	//mapping to memory
	size_t offset;
	for ( offset = 0; offset < f_size; offset += PGSIZE){
		valid_buf((char*) user_page, offset, esp);
		void *addr = user_page + offset;
		if ( page_lookup (t -> sup_pt, addr)){
			lock_release (&filesys_mutex);
			return -1;
		}
	}
	//map each page to file system
	for (offset = 0; offset < f_size; offset += PGSIZE){
     void *new_addr = user_page + offset;
     size_t read_bytes = (offset + PGSIZE < f_size ? PGSIZE : f_size - offset);
     size_t zero_bytes = PGSIZE - read_bytes;
     spt_addFile(t -> sup_pt, new_addr, f, offset, read_bytes, zero_bytes, true);
    }
	int id;
	if ( !list_empty (& t -> mmapList)){
		id = list_entry ( list_back (&t -> mmapList), struct mmap_record, elem) -> id+1;
	}
	else id = 1;
	struct mmap_record *mmap_r = (struct mmap_record *) malloc (sizeof (struct mmap_record));
	mmap_r -> id = id;
	mmap_r -> file = f;
	mmap_r -> user_addr = user_page;
	mmap_r -> file_size = f_size;
	list_push_back (&t->mmapList, &mmap_r -> elem);
	
	lock_release (&filesys_mutex);
	return id;
}


bool munmap (int id){
	struct thread *t = thread_current();
	struct mmap_record *mmap_r = find_mmap_record(id);
	if (mmap_r == NULL){
		return false;
	}
	
	lock_acquire(&filesys_mutex);
	
	size_t offset;
	size_t file_size = mmap_r -> file_size;
	for (offset = 0; offset > file_size; offset += PGSIZE){
        void *addr = mmap_r -> user_addr + offset;
        size_t bytes = ( offset + PGSIZE < file_size ? PGSIZE : filesize - offset);
        spt_unmapFile(t -> sup_pt, t -> pagedir, addr, mmap_r -> file, offset, bytes);
        
    }
    list_remove(& mmap_r -> elem);
    file_close (mmap_r -> file);
    free(mmap_r);
    
    lock_release (&filesys_mutex);
    return true;
	
}

static struct mmap_record* find_mmap_record (int id){
	struct thread *t = thread_current();
	ASSERT ( t != NULL );
	struct list_elem *e;
	if (list_empty (&t->mmapList)){
		for (e = list_begin(&t->mmapList); e != list_end (&t->mmapList); e = list_next(e)){
			struct mmap_record *mmap_r = list_entry (e , struct mmap_record, elem);
			if (mmap_r -> id == id){
				return mmap_r;
			}
		}
	}
	return NULL;
}



struct file * file_ptr(int fd)
{
   struct thread *t = thread_current();
   struct list *templist = &(t -> fd_entries);
   struct file_record *tempfileRd;
   struct list_elem *e;
   if (list_empty(templist))
      return NULL;
   for (e = list_begin (templist); e != list_end (templist);
           e = list_next (e))
   {
	  tempfileRd = list_entry(e,struct file_record, elem);
	  if ( tempfileRd->fd == fd)
	      {
		return tempfileRd -> cfile;
 	      }
   }
  thread_exit();
  return NULL;

}


void load_pin_pages ( const void *buffer, size_t length){
	struct thread *t = thread_current();
	struct hash *spt_pt = t -> sup_pt;
	uint32_t *pagedir = t  -> pagedir;
	
	void *user_page;
	for ( user_page = pg_round_down (buffer); user_page < buffer + length; user_page += PGSIZE){
		spt_load (spt_pt , pagedir, user_page);
		spt_pinPage (spt_pt, user_page);
	}
}


void unpin_pages ( const void *buffer, size_t length){
	struct thread *t = thread_current();
struct hash *spt_pt = t -> sup_pt;
	void *user_page;
	for ( user_page = pg_round_down(buffer); user_page < buffer + length; user_page += PGSIZE){
		spt_unpinPage (spt_pt, user_page);
	}
}


static void parse_args(void* esp, int* argBuf, int numToParse) {
  int i;
  for (i = 0; i < numToParse; i++) {
    valid_ptr(esp + ((i + 1) * 4), esp);
    argBuf[i] = *(int*) (esp + ((i + 1) * 4));
  }
} 

static void valid_ptr(void* user_ptr, void* esp) {
  if (!is_user_vaddr(user_ptr) || user_ptr < USER_VADDR_BOUND) {
    exit(-1);
  }

  struct thread *t = thread_current();
  bool result = false;
  void *user_pg = pg_round_down(user_ptr);
  result = spt_load(t->sup_pt, t->pagedir, user_pg);
  if (!result && user_ptr >= esp - 32) {
    bool canGrowStack;
    canGrowStack = (esp <= user_ptr || user_ptr == esp - 4 
                    || user_ptr == esp - 32) &&
                    (PHYS_BASE - MAX_STACK_SIZE <= user_ptr && user_ptr < PHYS_BASE);;
    if (canGrowStack) {
      result = spt_addZeroPage(t->sup_pt, user_pg);
    }
  }

  if (!result) {
    exit(-1);
  }
}

static void valid_buf(char* buf, unsigned size, void* esp) {
  int i;
  for (i = 0; i < size; i++) {
    valid_ptr(&buf[i], esp);
  }

//  valid_ptr(buf, esp);
//  valid_ptr(buf+size-1, esp);
}

static void valid_string(void* string, void* esp) {
  valid_ptr(string, esp);
  while (*(uint8_t*)string != '\0') {
    string++;
    valid_ptr(string, esp);
  }
}
