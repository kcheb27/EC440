#include "myshell_parser.h"
#include "stddef.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
//fnction prototype
char ** tokenize(const char * command_line, int *element_Count);


struct pipeline *pipeline_build(const char *command_line)
{
        // TODO: Implement this function
        
	// initialize the pipeline and first pipeline command 
	  struct pipeline *ret_Val = (struct pipeline *)malloc(sizeof(struct pipeline));
	  struct pipeline_command *cmd1 = (struct pipeline_command *) malloc(sizeof(struct pipeline_command));
	  ret_Val->commands = cmd1;
	  ret_Val->is_background = false;
	  ret_Val->commands->next = NULL;
	  ret_Val->commands->redirect_in_path = NULL;
	  ret_Val->commands->redirect_out_path =NULL;
	  
	  // new idea use tokenize function, and strtok plus the origional command line to keep the delimiters
	  //keeps track of number of tokens in the returned array
	  int element_Count;
	  

	  char ** tokens = tokenize(command_line, &element_Count);
	   bool redirect_in = false;
	   bool redirect_out = false;
	  //set a current pipeline ptr to keep track of pipes
	  struct pipeline_command *current = ret_Val->commands;
	  //set an index for current arg/command
	  int index = 0;
	  //parse
	  for (int i = 0; i< element_Count; i++){
	    if(!strcmp(tokens[i],"|")){
	      current->next = (struct pipeline_command *)malloc(sizeof(struct pipeline_command));
	      current = current->next;
	      current->next = NULL;
	      current->redirect_in_path = NULL;
	      current->redirect_out_path = NULL;
	      index = 0;
	      free(tokens[i]);
		//do stuff make new pipeline command
	    }else if (!strcmp(tokens[i],">")){
	      free(tokens[i]);
	      redirect_out = true;
	    }else if (!strcmp(tokens[i],"<")){
	      
	      free(tokens[i]);
	      redirect_in = true;
	    }else if (!strcmp(tokens[i],"&")){

	      ret_Val->is_background = true;
	      free(tokens[i]);
	    }else if (!strcmp(tokens[i]," ")){
	      free(tokens[i]);
	      //do  nothing
	    }else if (!strcmp(tokens[i],"\n")){
	      free(tokens[i]);
	      //do nothing
	    }else if (!strcmp(tokens[i],"\t")){
	      free(tokens[i]);
	      //do nothing
	    }else{
	      
	      if(redirect_in){
		current->redirect_in_path = tokens[i];
		redirect_in = false;
	      }else if (redirect_out){
		current->redirect_out_path = tokens[i];
		redirect_out = false;
	      }else{
		current->command_args[index] = tokens[i];
		index ++;
	      }
	    }
	  }
	  free(tokens);
	  return ret_Val;
	
}

void pipeline_free(struct pipeline *pipeline)
{
  struct pipeline_command *current = pipeline->commands;
  struct pipeline_command *past;
  while (current !=NULL){
    int i = 0;
    while(current->command_args[i] != NULL){
      free (current->command_args[i]);
      i++;
    }
    free(current->redirect_in_path);
    free(current->redirect_out_path);
    past = current;
    free(past);
    current = current->next;
  }
 
  free(pipeline);
  // TODO: Implement this function
};


///change the tokenize fuction to return a char** ( an array of strings)
//feeds in command_line and Element_Count to keep track for # of tokens in the array;
char** tokenize(const char* command_line, int* element_Count) {
    //initialize the array and allocate memory for the ptrs
    char** tokens = (char**)malloc(MAX_LINE_LENGTH * sizeof(char*));
    //intitialize num of elements
    int size = 0;      
     //temp the input string
    char* temp = strdup(command_line);
    
   
    //strtok 1rst time
    char* tkn = strtok(temp, "|<>&\n \t");

    while (tkn != NULL) {
        //allocate memory for token and copy
        tokens[size] = strdup(tkn);
        size++;

        // Find the position of the tkn in the original string
        size_t position = tkn - temp;
	int len = strlen(tkn);

	//Move to next tkn
	tkn = strtok(NULL, "|<>&\n \t");

	size_t position2 = tkn-temp;
	if(tkn != NULL){
	  position2 = tkn -temp;
	}else{
	  position2 = strlen(command_line);
	}
	
	for (int i = position + len; i<position2; i++){
		     //allocate for delim and check for null terminator
		     tokens[size] = (char*)malloc(2); // One for the delimiter and one for the null terminator
		     tokens[size][0] = (command_line[i] != '\0')
                                ? command_line[i]
                                : '\0';
		     tokens[size][1] = '\0';
		     size++;
	}
        
        
    }

    // Free the the copied string
    free(temp);

    // Update the element count
    //return elements
    *element_Count = size;

    return tokens;
}
