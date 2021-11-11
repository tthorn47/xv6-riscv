// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"
static char vals[11] = "0123456789";
char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", CONSOLE, 0);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr
  if(open("cycle", O_RDWR) < 0){
    mknod("cycle", CYCLE, 0);
    open("cycle", O_RDWR);
    //printf("cycle opened");
  }
  for (int i = 0; i < 10; i++)
  {
    char name[] = "barrier0";
    name[7] = vals[i];
    //printf("%s\n",name);
    if(open(name, O_RDWR) < 0){
    mknod(name, i+3, 0);
    open(name, O_RDWR);
    //printf("cycle opened");
  }
  }
  
  
  for(;;){
    printf("init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf("init: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      exec("sh", argv);
      printf("init: exec sh failed\n");
      exit(1);
    }

    for(;;){
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *) 0);
      if(wpid == pid){
        // the shell exited; restart it.
        break;
      } else if(wpid < 0){
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}
