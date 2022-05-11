#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/page.h"

typedef int mapid_t;
typedef int off_t;

static void syscall_handler (struct intr_frame *);
void halt (void);

void exit (int status);

tid_t exec (const char *cmd_line);

int wait (tid_t tid);

int read (int fd, void* buffer, unsigned size);

int write (int fd, const void *buffer, unsigned size);

bool create (const char *file, unsigned initial_size);

int open (const char *file);

bool remove(const char *file);

void close(int fd);

int filesize(int fd);

void seek(int fd, unsigned position);

unsigned tell(int fd);

void sigaction (int signum, void (*handler) (void));

void sendsig (tid_t tid, int signum);

void sched_yield (void);

int mmap(int fd, void *addr);

void munmap(mapid_t mapid);

struct vm_entry *check_address(void* addr, void* esp);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f) 
{
  
  if(!is_user_vaddr(f->esp))
    exit(-1);
  int *number = f->esp;
   // printf("%x esp syscall! :\n", f->esp);
  thread_current()->syscall_esp = number;
  switch(*(uint32_t *) number){
  case SYS_HALT:
  //printf("a\n"); 
    halt();   
    break;      
  case SYS_EXIT:
   //printf("b\n"); 
    if(!is_user_vaddr(number + 1))
      exit(-1);
    exit(*(uint32_t *)(number + 1));
    break;                  
  case SYS_EXEC:          
  //printf("c\n");
    if(!is_user_vaddr(number + 1))
      exit(-1);
    f->eax = exec(*(uint32_t *)(number + 1));
    break;
  case SYS_WAIT:
   //printf("d\n");
    if(!is_user_vaddr(number + 1))
     exit(-1);
    f->eax = wait(*(uint32_t *)(number + 1));
    break;                         
  case SYS_READ:
  //printf("e\n");

    if(!is_user_vaddr(number + 3) || !is_user_vaddr(*(uint32_t *)(number + 2)) || *(uint32_t *)(number + 1) == 1 || *(uint32_t *)(number + 1) <0)
      exit(-1);
    lock_acquire(&filesys_lock);
    f->eax = read(*(uint32_t *)(number + 1),*(uint32_t *)(number + 2),*(uint32_t *)(number + 3));
    lock_release(&filesys_lock);
    break;                   
  case SYS_WRITE:
   //printf("f\n");
   //printf("%s\n",*(uint32_t *)(number + 2));
  // hex_dump((uintptr_t) f->esp , f->esp , PHYS_BASE - f->esp , true);
    if(!is_user_vaddr(number + 3))
      exit(-1);
    lock_acquire(&filesys_lock);  
      f->eax = write(*(uint32_t *)(number + 1),*(uint32_t *)(number + 2),*(uint32_t *)(number + 3));
    lock_release(&filesys_lock);  
   break;     
  case SYS_CREATE:
   //printf("g\n");
    if(!is_user_vaddr(number + 2) || *(uint32_t *)(number + 1)==NULL || !strcmp(*(uint32_t *)(number + 1),""))
    exit(-1);
   // hex_dump((uintptr_t) f->esp , f->esp , PHYS_BASE - f->esp , true);
    lock_acquire(&filesys_lock); 
    f->eax = create(*(uint32_t *)(number + 1), *(uint32_t *)(number + 2));
    lock_release(&filesys_lock);  
    //printf("%x,%d\n",f->eip,*(uint32_t *)f->eip);
     //hex_dump((uintptr_t)  0xbffffec0 ,  0xbffffec0 , PHYS_BASE - 0xbffffec0 , true);
    break;   
  case SYS_OPEN:
  // printf("h\n");
   if(!is_user_vaddr(number + 1) || *(uint32_t *)(number + 1) == NULL)
      exit(-1);
    lock_acquire(&filesys_lock);   
    f->eax = open(*(uint32_t *)(number + 1));
    lock_release(&filesys_lock);  
    break;       
  case SYS_CLOSE:
 // printf("i\n");
    if(!is_user_vaddr(number + 1))
      exit(-1);
    lock_acquire(&filesys_lock);     
    close(*(uint32_t *)(number + 1));
    lock_release(&filesys_lock);  
    break;             
  case SYS_REMOVE:
   //printf("j\n");
   if(!is_user_vaddr(number + 1))
      exit(-1);
    lock_acquire(&filesys_lock);       
    f->eax = remove(*(uint32_t *)(number + 1));
    lock_release(&filesys_lock);  
    break;    
  case SYS_FILESIZE:
   //printf("k\n");
  if(!is_user_vaddr(number + 1))
      exit(-1);
    lock_acquire(&filesys_lock);         
    f->eax = filesize(*(uint32_t *)(number + 1));
    lock_release(&filesys_lock);  
    break;      
  case SYS_SEEK:
   //printf("l\n");
   if(!is_user_vaddr(number + 2))
      exit(-1);
    lock_acquire(&filesys_lock);           
    seek(*(uint32_t *)(number + 1), *(uint32_t *)(number + 2));
    lock_release(&filesys_lock);  
    break;      
  case SYS_TELL:
   //printf("m\n");
   if(!is_user_vaddr(number + 1))
      exit(-1);
    lock_acquire(&filesys_lock);             
    f->eax = tell(*(uint32_t *)(number + 1));
    lock_release(&filesys_lock);  
    break;
  case SYS_SIGACTION:
   //printf("n\n");
   if(!is_user_vaddr(number + 2))
      exit(-1);
    sigaction(*(uint32_t *)(number + 1), *(uint32_t *)(number + 2));
    break;     
  case SYS_SENDSIG:
   //printf("o\n");
   if(!is_user_vaddr(number + 2))
      exit(-1);
    sendsig(*(uint32_t *)(number + 1), *(uint32_t *)(number + 2));
    break;  
  case SYS_YIELD:
   //printf("p\n");
   if(!is_user_vaddr(number))
      exit(-1);
      sched_yield();
    break;      
  case SYS_MMAP:
   if(!is_user_vaddr(number))
      exit(-1);
     f->eax = mmap(*(uint32_t *)(number + 1), *(uint32_t *)(number + 2));
    break;      
  case SYS_MUNMAP:
   if(!is_user_vaddr(number))
      exit(-1);
      munmap(*(uint32_t *)(number + 1));
    break;                                                                       
  } 
  thread_current()->syscall_esp = NULL;

}

void halt (void) {
 shutdown_power_off();
}

void exit (int status) {
   printf("%s: exit(%d)\n" , thread_current() -> name , status);
   thread_current()->exit_status = status;
  thread_exit ();
}

tid_t exec (const char *cmd_line) {
  return process_execute(cmd_line);
}

int wait (tid_t tid) {
  return process_wait(tid);
}

int read (int fd, void* buffer, unsigned size) {
  if (fd == 0)
    return input_getc();
  else{
    return file_read(thread_current()->fdt[fd], buffer, size);
  }
}

int write (int fd, const void *buffer, unsigned size) {

  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  else {
    if(thread_current()->fdt[fd]!=NULL)
   return file_write(thread_current()->fdt[fd], buffer, size);
  }
}

bool create (const char *file, unsigned initial_size) {
   return filesys_create(file, initial_size);
   return 0;
}

bool remove(const char *file){
  return filesys_remove(file);
}


int open (const char *file){
  if(thread_current()->next_fd>63)
  return -1;
  struct file *f = filesys_open(file);
  if(f!=NULL){
  thread_current()->fdt[thread_current()->next_fd++] = f;
  return (thread_current()->next_fd - 1 );
  }
  return -1;
  
}

void close(int fd){
  if(fd>1){
  file_close(thread_current()->fdt[fd]);
  thread_current()->fdt[fd] = NULL;
  }
}

int filesize(int fd){
return file_length(thread_current()->fdt[fd]);
}

void seek(int fd, unsigned position){
   file_seek(thread_current()->fdt[fd], position);
}

unsigned tell(int fd){
  return file_tell(thread_current()->fdt[fd]);
}

void sigaction (int signum, void (*handler) (void)){
struct thread *cur = thread_current ();

  cur->signal_handler[signum - 1] = handler;
}

void sendsig (tid_t tid, int signum){
struct thread *cur = thread_current ();
  struct thread *t = NULL;
  struct list_elem *e;

  for (e = list_begin (&cur->child_list); e != list_end (&cur->child_list); e = list_next (e)) 
  {
    t = list_entry (e, struct thread, child_elem);
    if (t->tid == tid)
      break;
  }

  if (t->signal_handler[signum - 1] != NULL)
    printf("Signum: %d, Action: 0x%x\n", signum, t->signal_handler[signum-1]);
}

void sched_yield (void)
{
  thread_yield ();
}

int mmap (int fd, void *addr){
  if(addr <(void *) 0x08048000 || addr >= (void *) 0xc0000000 || addr == NULL || pg_ofs(addr) != 0)
  return -1;
struct vm_entry *vme;
struct mmap_file *mf;
off_t ofs = 0;
struct file* file = file_reopen(thread_current()->fdt[fd]);
uint32_t read_bytes = file_length(file), zero_bytes = 0;
mf = (struct mmap_file *) malloc(sizeof (struct mmap_file));
mf->mapid = thread_current()->mapping_id++;
mf->file = file;
list_init(&mf->vme_list);
while (read_bytes > 0 || zero_bytes > 0) 
    {
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;
      struct vm_entry *vme;
      vme = (struct vm_entry *) malloc(sizeof (struct vm_entry));
      vme->type = VM_FILE;
      vme->file = file;
      vme->offset = ofs;
      vme->read_bytes = page_read_bytes;
      vme->zero_bytes = page_zero_bytes;
      vme->writable = true;
      vme->vaddr = addr;
      vme->is_loaded = false;
      if(!insert_vme(&thread_current()->vm, vme))
      return -1;
      /* Advance. */
      read_bytes -= page_read_bytes;
      addr += PGSIZE;
      ofs += page_read_bytes;
      list_push_back(&mf->vme_list, &vme->mmap_elem);
    }

list_push_back(&thread_current()->mmap_list, &mf->elem);


return mf->mapid;
}

void munmap (mapid_t mapid){
struct list_elem *e, *f;
struct mmap_file *mf;
struct vm_entry *vme;
    for (e = list_begin (&thread_current()->mmap_list); e != list_end (&thread_current()->mmap_list);
       e = list_next (e))
    {
      mf = list_entry (e, struct mmap_file, elem); 
      if(mf->mapid == mapid)
      {
        for (f = list_begin (&mf->vme_list); f != list_end (&mf->vme_list);
       )
    {
      vme = list_entry (f, struct vm_entry, mmap_elem);
      f = list_remove (f);
      if(vme->is_loaded){
        if(pagedir_is_dirty(thread_current()->pagedir, vme->vaddr)){
          lock_acquire(&filesys_lock);
          file_write_at(mf->file, pagedir_get_page(thread_current()->pagedir, vme->vaddr) , PGSIZE, vme->offset);
          lock_release(&filesys_lock);
        }
        free_page(pagedir_get_page(thread_current()->pagedir, vme->vaddr));  
        pagedir_clear_page(thread_current()->pagedir, vme->vaddr);  
      }
      delete_vme(&thread_current()->vm, vme);
      
    }  
      }
      file_close(mf->file);
      list_remove(&mf->elem);
      free(mf);
      break;
    }  
}

struct vm_entry *check_address(void *addr, void* esp){
  return find_vme(addr);
}