//
//  shell.c
//  shell
//
//  Created by Alex on 2016-01-26.
//  Copyright Â© 2016 Alex. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<signal.h>

//Signal handler function dec.
void sighandler(int);

//Simple fork to execute commands without pipe or redirection
int executeCmd(char** params);

//Command length and num of parameters 
#define MAX_COMMAND_LENGTH 100
#define MAX_NUMBER_OF_PARAMS 10

// Split cmd into array of parameters
int parseCmd(char* cmd, char *params[]){

    char *line;
    int i;
    i = 0;
    
    line = cmd;
    params[i] = strtok(line, " ");
    do  {
        i++;
        line = NULL;
        params[i] = strtok(line, " ");
    } while (params[i] != NULL);
    
    return i;
}

//Pipe
int do_pipe(char **args, int pipes) {
    
    // The number of commands to run
    const int commands = pipes + 1;
    int i = 0;
    
    int pipefds[2*pipes];
    
    for(i = 0; i < pipes; i++){
        if(pipe(pipefds + i*2) < 0) {
            perror("Error! Couldn't Pipe");
            exit(EXIT_FAILURE);
        }
    }
    
    int pid;
    int status;
    
    int j = 0;
    int k = 0;
    int s = 1;
    int place;
    int commandStarts[10];
    commandStarts[0] = 0;
    

    // Set pipes to null and create an array from the next element
    while (args[k] != NULL){
        if(!strcmp(args[k], "|")){
            args[k] = NULL;
            // printf("args[%d] is now NULL", k);
            commandStarts[s] = k+1;
            s++;
        }
        k++;
    }
    
    
    
    for (i = 0; i < commands; ++i) {
        
        // place is where in args the program should start running when it gets to the execution
        place = commandStarts[i];
        
        pid = fork();
        
        //Child
        if(pid == 0) {
            
            if(i < pipes){
                if(dup2(pipefds[j + 1], 1) < 0){
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            
            if(j != 0 ){
                if(dup2(pipefds[j-2], 0) < 0){
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            
            int q;
            for(q = 0; q < 2*pipes; q++){
                close(pipefds[q]);
            }
            
            // Execute commands
            if( execvp(args[place], args+place) < 0  ){
                perror(*args);
                exit(EXIT_FAILURE);
            }
        }
        
        //Pipe Error
        else if(pid < 0){
            perror("Pipe Error in do_pipe function");
            exit(EXIT_FAILURE);
        }
        
        j+=2;
    }
    
    for(i = 0; i < 2 * pipes; i++){
        close(pipefds[i]);
    }
    
    for(i = 0; i < pipes + 1; i++){
        wait(&status);
    }
    
    return 0;
}

//MAIN
int main() {
    
   signal(SIGINT, sighandler);
    
    char cmd[MAX_COMMAND_LENGTH + 1];
    char histar[10][MAX_COMMAND_LENGTH + 1];
    char hischar[MAX_COMMAND_LENGTH + 1];
    
    int a = 0;
    int piped =0;
    int redirin = 0;
    int redirout = 0;
    int hiscount = 0;
    

    while(1) {
        
        char* params[MAX_NUMBER_OF_PARAMS + 1];
       
        printf("azarifog> ");
        

        // Read command from standard input
        if(fgets(cmd, sizeof(cmd), stdin) == NULL) break;
        
        // Remove trailing newline character, if any
        if(cmd[strlen(cmd)-1] == '\n') {
            cmd[strlen(cmd)-1] = '\0';
        }
        
        //Copy the input for history to save later..
        strncpy(hischar, cmd, MAX_COMMAND_LENGTH + 1);

        // Split cmd into array of parameters
        int n = parseCmd(cmd, params);
        
        // If exit is entered
        if(strcmp(params[0], "exit") == 0) break;
        
        //History
        if(strcmp(params[0], "history") == 0){
            int count_num = 1;
            if(hiscount < 10) count_num = hiscount;
            else count_num = 10;
            
            for(int h = 0 ; h < count_num; h++){
                printf("%s\n" , histar[h]);
            }
            
            continue;
        }
        else {
            int hc = hiscount % 10;
            strcpy(histar[hc], hischar);
            //printf("Copying to index at %d: %s\n" ,hc, hischar);
            hiscount++;
        }
        
        
        //Check variables to find out if commands is pipe or redirect
        for(a = 0; a < n; a++){
            if(!strcmp(params[a], "|")){
                piped++;  //pipe command entered
                //printf("piped: %i", piped);
            }
            if(!strcmp(params[a], "<")){
                redirin++;  //redireted in command
                 //printf("redirin: %i", redirin);
            }
            if(!strcmp(params[a], ">")){
                redirout++; //redirected out command
                 //printf("redirout: %i", redirout);
            }
        }
        
        //If there is no need for pipe or redirection
        if(piped == 0 && redirin == 0 && redirout == 0){
            //printf("PARAM: %s\n", params[0]);
            if(executeCmd(params) == 0) break;
        }
        else{
            
            //If there is a need for pipe
            if(piped != 0 ){
                //Pipe function
                do_pipe(params, piped);
            }
            
            //If there is redirection
            else if(redirin == 1 || redirout == 1){
                
                int ic1 = 2;
                int ic2 = 4;
                
                for(int i=0;params[i]!='\0';i++)
                {
                    if(strcmp(params[i],"<")==0)
                    {
                        params[i]=NULL;
                        ic1 = i+1;
                    }
                    
                    else if(strcmp(params[i],">")==0)
                    {
                        params[i]=NULL;
                        ic2 = i+1;
                    }
                }
                
                
                // Fork process
                pid_t pid = fork();
                
                // Error
                if (pid == -1) {
                    char* error = strerror(errno);
                    printf("fork: %s\n", error);
                    return 1;
                }
                
                // Child process
                else if (pid == 0) {
                    
                    int in, out;
                    
                    int p = 0;
                    
                    char *args2[MAX_COMMAND_LENGTH + 1] = {};
                    
                    for(p =0; p < ic1; p++){
                        args2[p] = params [p];
                        //printf("%s" , args2[p]);
                    }
                    
                    args2[p] = NULL;
                    
                    
                    // Open input and output files
                    if(redirin == 1){
                       in = open(params[ic1], O_RDONLY);
                        
                        //Error handling if the file does not exist.
                        if(in == -1){
                            printf("The file does not exist: %s\n", params[ic1]);
                            piped = 0;
                            redirin = 0;
                            redirout = 0;
                            close(in);
                            break;
                        }
                        
                        // Replace standard input with input file
                        dup2(in, 0);
                        close(in);
                        
                        
                    }
                    if(redirout == 1){
                        out = open(params[ic2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                        
                        // replace standard output with output file
                        dup2(out, 1);
                        close(out);
                    }
                    
                    
                    // execute arguments
                    execvp(args2[0], args2);
                    
                    // Error occurred
                    char* error = strerror(errno);
                    printf("Error! azarifog: %s: %s\n", params[0], error);
                    
                }
                
                // Parent process
                else {
                    // Wait for child process to finish
                    int childStatus;
                    waitpid(pid, &childStatus, 0);
                    
                }

                
            } // redirect
            
            
            
        }
        //reset variables
        piped = 0;
        redirin = 0;
        redirout = 0;
        
        //For the signal handler testing
        //sleep(1);
        
    }
    
    return 0;
}

//To execute commands without pipe and redirection
int executeCmd(char** params){
    // Fork process
    pid_t pid = fork();
    
    // Error
    if (pid == -1) {
        char* error = strerror(errno);
        printf("Error! fork: %s\n", error);
        return 1;
    }
    
    // Child process
    else if (pid == 0) {
        // Execute command
        execvp(params[0], params);
        
        // Error occurred
        char* error = strerror(errno);
        printf("Error! azarifog: %s: %s\n", params[0], error);
        return 0;
    }
    
    // Parent process
    else {
        // Wait for child process to finish
        int childStatus;
        waitpid(pid, &childStatus, 0);
        return 1;
    }
}

//Signal handler
void sighandler(int signum){
    printf("Signal caught, exiting...\n");
    exit(1);
}