
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
int main(){
    char *buffer[5] = {"ls"};
      pid_t cpid;
        int status;
        int fd[2];

  //
  //fork//
  cpid = fork();
  if(cpid == 0){
    fd[1] = open("test.txt",O_WRONLY,0);
    dup2(fd[1], STDOUT_FILENO);
    close(fd[1]);
    execvp(buffer[0], buffer);
    
  }else{
    waitpid(-1,&status,0);
  }
}