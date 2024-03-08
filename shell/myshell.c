
#include "myshell_parser.h"
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

void check_background()
{
		int wstat;
		pid_t	pid;

		while (true) {
			pid = waitpid (-1,&wstat, WNOHANG);
			if (pid == 0)
				return;
			else if (pid == -1)
				return;
		}
}

int execute_the_children(bool background, struct pipeline_command *line, bool first, bool last, int pipe_Rd){
  pid_t cpid;
  int status;
  int fd[2];  
  pipe(fd);
  //
  //fork//
  cpid = fork();
  if(cpid == 0){
    //piping
    if(background){
    //do nothing
    }else{
    //first and pipe not single command
    if(first&&(!last)&&(pipe_Rd == 0)){
      close(fd[0]);
      dup2(fd[1],STDOUT_FILENO); //<-set output to pipe
      
    }
    //middle and pipe
    else if((!first)&&(!last)&&(pipe_Rd != 0)){
      close(fd[0]);
      dup2(fd[1],STDOUT_FILENO); // <-set output to pipe
      dup2(pipe_Rd,STDIN_FILENO);// <-set stdin from previous pipe
      
    }
    //single no pipe or end of of pipe(last)
    else{
      //check for redirects
      if((line->redirect_out_path)&&last){
        if((fd[1] = open(line->redirect_out_path,O_WRONLY |O_CREAT|O_TRUNC,0)) ){
          perror("ERROR");
        }
        dup2(fd[1],STDOUT_FILENO);
      }

      if((line->redirect_in_path)&&first){
        if((fd[0] = open(line->redirect_in_path,O_RDONLY,0))){
          perror("ERROR");
        }
        dup2(fd[0],STDIN_FILENO);
      }
      //reroute for pipe
      if(dup2(pipe_Rd,STDIN_FILENO == -1)){
        perror("ERROR");
      }
      
    }   
    }
    if(execvp(line->command_args[0], line->command_args) == -1){
      perror("ERROR:");
    };
    close(fd[1]);
    close(fd[0]);
    
  }else{
    if(!background){
    waitpid(-1,&status,0);
    }
    close(fd[1]);
    
  }
  //check if previous pipe input exists
  if(pipe_Rd != 0){
  close(pipe_Rd);
  }
  //check if pipe read is needed
  if(last){
    close (fd[0]);
  }
  return fd[0]; // <- return int so it can be sent to next pipe

}
int main(int argc, char*arg[]){
  //buffer for fgets();
  char buffer[100];
  struct pipeline *line;
  int prompt_check = 0;
  bool first = true;
  bool last = false;
  char *nflag = "-n";
  if((argc>1)&&!(strcmp(arg[1],nflag))){
    prompt_check = 1;
  }
  signal (SIGCHLD, check_background);
  while(1){
    //print prompt
      if(!prompt_check){
        printf("myshell$");
      }
    //read input into buffer exit if ctrl+d
      if(fgets(buffer,sizeof(buffer),stdin)== NULL){
        if (feof(stdin)) {
          // End of file reached
          // Handle end-of-file condition or break out of the loop
          break;
        } else if (ferror(stdin)) {
          // Error reading from stdin
          perror("Error reading from stdin");
          // Handle the error or break out of the loop
          break;
        }
      }else{
          //take buffer and parse into pipeline
          line = pipeline_build(buffer);
      }
    //execute pipeline?
    struct pipeline_command *current = line->commands;
    int pipe_Rd = 0;
    //check if end of pipe or if there are no command args ie blank command
    while(current != NULL&&(current->command_args[0]!= NULL)){
      if(current->next == NULL){
        last = true;
      }
      pipe_Rd = execute_the_children(line->is_background,current,first,last,pipe_Rd);
      current = current->next;
      first = false;
    }
    //free pipe
      pipeline_free(line);
    //clear buffer for new input
      memset(buffer, 0, sizeof(buffer));
      first = true;
    
  }
  
  return 0;

}