#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "filesys/off_t.h"

static void syscall_handler (struct intr_frame *);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t current_syscall = *((uint32_t *) f -> esp);
  char *file;
  off_t initial_size;
  int fd;
  switch(current_syscall)
  {
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
                
		exit (f -> eax);
		break;
	case SYS_EXEC:
		if (is_user_vaddr((char **) ((f -> esp) + 4)))
		{
			file = *(char **) ((f -> esp) + 4);
			/* TODO: SEMA DOWN TO WAIT FOR CHILD TO LOAD */
      exec (file);
		}
		else thread_exit();
		break;
	case SYS_WAIT:
		
		if (is_user_vaddr((char **) ((f -> esp) + 4)))
		{
      wait (*(pid_t*) ((f->esp) + 4));
		}
    else thread_exit();
		break;
  case SYS_CREATE:
		if (is_user_vaddr((char **) ((f -> esp) + 4)) && is_user_vaddr((char **) ((f -> esp) + 8)))
		{
    			file = *(char **) ((f -> esp) + 4);
    			initial_size = *(off_t*) ((f -> esp) + 8);
  			create(file, initial_size);
		}
		else thread_exit();
    		break;
	case SYS_REMOVE:
		if (is_user_vaddr((char **) ((f -> esp) + 4)))
		{
			file = *(char **) ((f -> esp) + 4);
			remove (file);
		}
		else thread_exit();
		break;
	case SYS_OPEN:
		if (is_user_vaddr((char **) ((f -> esp) + 4)))
		{
			file = *(char **) ((f -> esp) + 4);
			open (file);
		}
		else thread_exit();
		break;
	case SYS_FILESIZE:
		if (is_user_vaddr((char **) ((f -> esp) + 4)) && is_user_vaddr((char **) ((f -> esp) + 8)))
		{
			file = *(char **) ((f -> esp) + 4);
			fd = *(int*) ((f -> esp) + 8);
			filesize (fd);
		}
		else thread_exit();
		break;
	case SYS_READ:
		if (is_user_vaddr((char **) ((f -> esp) + 4)) && is_user_vaddr((char **) ((f -> esp) + 8)) && is_user_vaddr((char **) ((f -> esp) + 12)))
		{
			file = *(char **) ((f -> esp) + 4);
			void *buf = *(void **) ((f -> esp) + 8);
			unsigned size = *(unsigned*) ((f -> esp) + 12);
			read (file, buf, size);
		}
		else thread_exit();
		break;
	case SYS_WRITE:
		if (is_user_vaddr((char **) ((f -> esp) + 4)) && is_user_vaddr((char **) ((f -> esp) + 8)) && is_user_vaddr((char **) ((f -> esp) + 12)))
		{
			file = *(char **) ((f -> esp) + 4);
			const void *buf = *(void **) ((f -> esp) + 8);
			unsigned size = *(unsigned*) ((f -> esp) + 12);
			write (file, buf, size);
		}
		else thread_exit();
		break;
	case SYS_SEEK:
		if (is_user_vaddr((char **) ((f -> esp) + 4)) && is_user_vaddr((char **) ((f -> esp) + 8)))
		{
			int fd = *(int *) ((f -> esp) + 4);
			unsigned pos = *(unsigned *) ((f -> esp) + 8);
			seek (fd, pos);
		}
		else thread_exit();
		break;
	case SYS_TELL:
		if (is_user_vaddr((char **) ((f -> esp) + 4)))
		{
			int fd = *(int *) ((f -> esp) + 4);
			tell (fd);
		}
		else thread_exit();
		break;
	case SYS_CLOSE:
		if (is_user_vaddr((char **) ((f -> esp) + 4)))
		{
			int fd = *(int *) ((f -> esp) + 4);
			close (fd);
		}
		else thread_exit();
		break;
	default:	
		printf("not\n");
	
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
      if (cr->child->tid == t->tid) {
        if (status == 1) {
          cr->retVal = -1;
          break;	
        }
        cr->retVal = status;
        break;
      }
      e = list_next(e);
    }
  }
  printf("%s: exit(%d)\n", t->name, status);
  if ( t->parent_wait)
   sema_up(&(t->parent->child_sema));
  thread_exit ();
}
pid_t exec (const char *cmd_line) 
{
 //return +ve value if success otherwise -1
 return process_execute(cmd_line);
}
int wait (pid_t pid) 
{
 return process_wait (pid);
}
bool create (const char *file, unsigned initial_size) 
{
  return filesys_create(file, initial_size); 
}
bool remove (const char *file) 
{
  return filesys_remove(file);
}
int open (const char *file) 
{
  struct file_record *cfileRecord;
  cfileRecord = malloc ( sizeof (struct file_record));
  struct thread *t = thread_current();
  struct file *currentfile = filesys_open (file);
  if ( currentfile == NULL)
   return -1;
  cfileRecord -> cfile = currentfile;
  cfileRecord -> fd = t -> total_fd;
  t -> total_fd = t -> total_fd + 1;
  list_push_back(&(t-> fd_entries), &(cfileRecord ->elem));
  return cfileRecord -> fd;
}
int filesize (int fd) 
{
  struct file *tempfile;
  tempfile = file_ptr(fd);
  if (tempfile != NULL)
  	return file_length(tempfile);
  return -1;
}
int read (int fd, void *buffer, unsigned length) 
{
 if (fd != 0)
 {
   struct file *tempfile = NULL;
   tempfile = file_ptr(fd);
   if (tempfile == NULL) {
 	    return -1;
  }
   return file_read (tempfile, buffer, length);
 }
 else{
 	 input_getc();
	}
}
int write (int fd, const void *buffer, unsigned length)
{
if (fd != 1)
 {
  struct file *tempfile;
  tempfile = file_ptr(fd);
  if (tempfile == NULL) {
    return -1;
  }
 	return file_write (tempfile, buffer, length);
 }
 else {
 	putbuf(buffer, length);
 }

}
void seek (int fd, unsigned position) 
{
  struct file *tempfile;
  tempfile = file_ptr(fd);
 	if (tempfile != NULL) {
    file_seek (tempfile, position);
  }
}
unsigned tell (int fd) 
{
  struct file *tempfile;
  tempfile = file_ptr(fd);
  if (tempfile == NULL) {
    return -1;
  }
 	return file_tell (tempfile);
}
void close (int fd) 
{
   struct thread *t = thread_current();
   struct list *templist = &(t -> fd_entries);
   struct file_record *tempfileRd;
   struct list_elem *e;
   for (e = list_begin (templist); e != list_end (templist);
           e = list_next (e))
   {
	  tempfileRd = list_entry(e,struct file_record, elem);
	  if ( tempfileRd->fd == fd)
	      {
		file_close(tempfileRd -> cfile);
		list_remove(e);
		free(tempfileRd);
		break;
		}
   }
}

struct file * file_ptr(int fd)
{
   struct thread *t = thread_current();
   struct list *templist = &(t -> fd_entries);
   struct file_record *tempfileRd;
   struct list_elem *e;
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







