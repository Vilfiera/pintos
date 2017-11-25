#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/off_t.h"

static void syscall_handler (struct intr_frame *);

bool create (const char *file, unsigned initial_size); 
bool remove (const char *file); 

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
		if (is_user_vaddr((char **) ((f -> esp) + 4))
		{
			file = *(char **) ((f -> esp) + 4);
			exec (file);
		}
		else thread_exit();
		break;
	case SYS_WAIT:
		wait (pid_t);
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
	default:	
		printf("not\n");
	
  }
  thread_exit ();
}

void halt (void) 
{
 shutdown_power_off ();
}
void exit (int status) 
{
  thread_current ()->return_value = status;
  thread_exit ();
}
pid_t exec (const char *cmd_line) 
{
 //return +ve value if success otherwise -1
 return process_execute(file);
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
  struct *t = thread_current();
  struct *file currentFile = filesys_open (file);
  if ( currentFile == NULL)
   return -1;
  t -> total_fd = t -> total_fd + 1;
  currentfile -> fd = t -> total_fd;
  list_push_back(&(t-> fd), &(currentFile->file_elem));
  return total_fd;
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
   struct thread *t = thread_current();
   struct list processfd = t -> fd;
   struct file *tempfile;
   tempfile = file_ptr(fd);
   if (tempfile != NULL)
 	return file_read (tempfile, buffer, length);
   return -1;
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
  if (tempfile != NULL)
 	return file_write (tempfile, buffer, length);
  return -1;
 }
 else {
 	putbuf();
 }
}
void seek (int fd, unsigned position) 
{
  struct file *tempfile;
  tempfile = file_ptr(fd);
  if (tempfile != NULL)
 	file_seek (tempfile, position);
}
unsigned tell (int fd) 
{
  struct file *tempfile;
  tempfile = file_ptr(fd);
  if (tempfile != NULL)
 	return file_tell (tempfile);
  return -1;
}
void close (int fd) 
{
   struct file *tempfile;
   tempfile = file_ptr(fd);
   if (tempfile != NULL)
   {
   struct list_elem *e = &tempfile -> file_elem;
   file_close (tempfile);
   list_remove(e);
   }
}

struct file * file_ptr(int fd)
{
   struct thread *t = thread_current();
   struct list processfd = t -> fd;
   struct file *tempfile;
   struct list_elem *e;
   for (e = list_begin (&foo_list); e != list_end (&foo_list);
           e = list_next (e))
   {
	  tempfile = list_entry(e,struct file, elem);
	  if ( tempfile->fd == fd)
	      {
		return tempfile;
 	      }
   }
  thread_exit();
  return NULL;
}








