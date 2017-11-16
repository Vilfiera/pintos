#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"

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
  switch(current_syscall)
  {
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
		printf("exit/n");
		break;
	case SYS_EXEC:
		printf("exec/n");
		break;
	case SYS_WAIT:
		printf("wait/n");
		break;
	case SYS_CREATE:
		printf("create/n");
		break;
	case SYS_REMOVE:
		printf("remove/n");
		break;
	case SYS_OPEN:
		printf("open/n");
		break;
	case SYS_FILESIZE:
		printf("filesize/n");
		break;
	default:	
		printf("not/n");
	
  }
  thread_exit ();
}

void halt (void) 
{
 shutdown_power_off ();
}
void exit (int status) 
{
 
}
int exec (const char *file) 
{
 printf("exec/n");
}
int wait (pid_t) 
{
 printf("wait/n");
}
bool create (const char *file, unsigned initial_size) 
{
  printf("create/n");
}
bool remove (const char *file) 
{
  printf("remove/n");
}
int open (const char *file) 
{
  printf("open/n");
}
int filesize (int fd) 
{
  printf("filesize/n");
}
int read (int fd, void *buffer, unsigned length) 
{
  printf("read/n");
}
int write (int fd, const void *buffer, unsigned length) 
{
  printf("write/n");
}
void seek (int fd, unsigned position) 
{
  printf("seek/n");
}
unsigned tell (int fd) 
{
  printf("tell/n");
}
void close (int fd) 
{
  printf("close/n");
}
