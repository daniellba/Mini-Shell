/********************
*	Daniel Ben-Ami  *
********************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

void sig_handler(int signo)
//this function disable ^c
{
	signal(SIGINT, sig_handler);
	signal(SIGCHLD, sig_handler);
	while(waitpid(-1,NULL,WNOHANG)>0);
}

void prompt ()				//prints the prompt line
{
		struct passwd *pws;
		pws = getpwuid(0);
		char cwd[256];

		getcwd(cwd,sizeof(cwd));

		printf("%s@%s>", pws->pw_name,cwd);
}

int locateIndex(char ** words,int count,char* x)
//searching for diffrent chars in the array if found, return it's position if not, return -1
{
	for(int i=0; i < count-1; i++) 
	{
		if (strcmp(words[i], x) == 0)
		{
			return i;
		}
	}
	return -1; 
}

void freeMem(char **arr, int count)
//free up the array memory 
{
	for(int j=0; j < count; j++) 
	{
		if (arr[j] == NULL)
		{ continue; }
		free(arr[j]);
	}
	free(arr);	
}

void pipeFunction(char **words, int location, int count)
//in this function it opparte a pipe command by dividing the command in to two arrays.
{
	int status=0;
	char ** right, ** left;
	int fds[2];
	int r = 0;
	pid_t leftSon, rightSon;
	
	left = (char**)malloc((location+2)*sizeof(char*));	//allocating left array memory
	if (left == NULL)
	{
		printf("unableto alocate memory (left array)");
	}
	
	right = (char**)malloc((count-location)*sizeof(char*)); //allocating right array memory
	if (left == NULL)
	{
		printf("unableto alocate memory (left array)");
	}
	
	for (int i=0; i < count-1; i++)							//after allocting memory, the loop insert each word to it's cell
	{
		if (i < location)
		{
			left[i] = words[i];
		}
		
		else if (i > location)
		{
			right[r++] = words[i];
		}
	}
	left[location] = NULL;									//the last cell is null
	right[count-location-1] = NULL;
	
	
	if ( pipe(fds) < 0)
	{
		printf("Faild to make pipe");
		exit(1);
	}
	
	if ((leftSon=fork()) == 0)
	{
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO);						//close and open the pipe according to our needs
		if(execvp(*left, left) < 0)
		{
			printf("Faild to do left-son fork");
		}
		close(fds[1]);
	}
		
	if ((rightSon=fork()) == 0)
	{
		close(fds[1]);
		dup2(fds[0], STDIN_FILENO);							//close and open the pipe according to our needs
		if(execvp(*right, right) < 0)
		{
			printf("Faild to do right-son fork");
		}
		close(fds[1]);
	}
	else {
		close(fds[0]);
		close(fds[1]);
		wait(&status);
		if(!WIFEXITED(status))
        {
			return;
        }
		wait(&status);
		if(!WIFEXITED(status))
        {
          return;
        }
	}
		
	freeMem(left,location+1);						
	freeMem(right,count-location-1);
}

void execute(char ** words, int *cmdCounter, int *sumOfLetter, int count)
/*in that function i check the commands the user inserts if its a vaild command 
the shall operate it, it also checks if the user insert "done" in order to finsh 
the program*/
{
	signal(SIGINT, sig_handler);
	signal(SIGCHLD, sig_handler);
	bool whileSonWorking = false;
	
    int status=0;
	
	if(strcmp(words[0], "cd") == 0) 				//change the path
	{
		chdir(words[1]);
	}
	
    else 											//if the cammand is vaild it will oparte it
    {
        *cmdCounter = *cmdCounter + 1;
        *sumOfLetter = strlen(words[0]) + *sumOfLetter;

		if (locateIndex(words, count, "&") == count - 2)  //if the user wants to opporate a command in the background
		{
			whileSonWorking = true;
			words[count-2] = NULL;
		}
		
	    pid_t process = fork();						 //makes a father-son process
	    
		if (process == -1) 							 //faild to do fork
	    {
	    	perror("Erorr in fork");
            exit(1);
	    }
		
	    else if (process == 0) 						 //son
	    {
	    	if (execvp(words[0], words) == -1) 		 //checks if the the command is vaild, if not 
													 //prints an error
	    	{
				exit(0);
	    	}
	    }

	    else											 //father function
	    {
			if (whileSonWorking)
			{
				return;
			}
			
	    	wait(&status); 								 //the father waits untill the son finish.
            if(!WIFEXITED(status))
            {
                exit(0);
            }
        }
    }
}

int redirectDefnition(char ** words,int count)
//this function defines in witch case we handle
{
	for(int i=0;i<count;i++)
	{
		if(strcmp(words[i],">") == 0)
			return 1;
		if(strcmp(words[i],">>") == 0)
			return 2;
		if(strcmp(words[i],"2>") == 0)
			return 3;
		if(strcmp(words[i],"<") == 0)
			return 4;
	}
	return -1;
}

void redirectFunction(char ** words,int count,int redirectPlace,int definedRedirect)
//in this function i "take action" according to the case im handling
{
	char ** temp = (char**) malloc ( sizeof(char*)*redirectPlace + 1 );
	if( temp == NULL )
	{
		printf("unable to allocate memory");
		exit(0);
	}
	
	char fileName[510];
	int fd;
	pid_t p;
	
	for(int i=0;i<count-1;i++)
	{
		if(i < redirectPlace)
			temp[i] = words[i];
		else
		{
			if(i == redirectPlace)
				continue;
			strcpy(fileName,words[i]);
		}
	}
	
	int pipe_index = locateIndex(temp, redirectPlace, "|");
	if(pipe_index != -1)
	{
		bool is_piped = true;
		pipeFunction(words, redirectPlace, count);
		free(temp);
		return;
	}
	
	else
	{
		temp[redirectPlace] = NULL;
		if( (p=fork()) == 0 )
		{		
			if( definedRedirect == 1 || definedRedirect == 3 )
			{
				fd = open(fileName,O_WRONLY|O_CREAT|O_TRUNC,0600);
				if(fd<0)
				{
					perror("File open failed\n");
					free(temp);
					freeMem(words,count);
					exit(1);
				}
			}
			
			else if( definedRedirect == 2 )
			{
				fd = open(fileName,O_WRONLY|O_CREAT|O_APPEND,0600);
				if(fd<0)
				{
					perror("File open failed\n");
					free(temp);
					freeMem(words,count);
					exit(1);
				}
			}
			
			else if( definedRedirect == 4)
			{
				fd = open(fileName,O_RDONLY,0600);
				if(fd < 0)
				{
					fprintf(stderr,"File open failed\n");
					free(temp);
					freeMem(words,count);
					exit(1);
				}
			}

			if(definedRedirect == 1 || definedRedirect == 2)
				dup2(fd,STDOUT_FILENO);
			else if (definedRedirect == 3)
				dup2(fd,STDERR_FILENO);
			else if (definedRedirect == 4)
				dup2(fd,STDIN_FILENO);

			if(execvp(*temp,temp)<0)					//opporates the command
			{
				perror("execvp failed\n");
				free(temp);
				freeMem(words,count);
				close(fd);
				exit(1);
			}
			
		free(temp);
		close(fd);
		}

		else if(p<0)
		{
			printf("ERR \n");
			free(temp);
			freeMem(temp,count);
			exit(1);
		}
		else
			wait(NULL);

	}
}

int main()
{
	int sumOfLetter = 0, cmdCounter = 0, flag=0;
    int count;
    char **words;

	while(true) 
    {
		signal(SIGINT, sig_handler);
		signal(SIGCHLD, sig_handler);
		
		int i=0;
		char userS[510]; 									//user insert
       
        while(flag == 0) 										//if the user press enter without typing a command
        { 
            prompt();
            fgets(userS,510,stdin);
            if(strcmp(userS,"\n")!=0)
                flag=1;
        }
        flag=0;

		char temp[510];										//copy array
		strcpy(temp,userS);

		char *ptr = strtok(temp," \n\""); 					//dividing sentance to words according to the symbols

		count = 1;

		while(ptr != NULL)									//as long as the ptr is not null it keeps dividing
		{
			count++;										//counts the num of words
			ptr = strtok(NULL," \n\"");
		}

		words = (char**)malloc(sizeof(char*)*count); 		//allocating memory
		if(words == NULL)
		{
			printf("Couldn't allocate memory ");
			exit(1);
		}
		
		ptr = strtok (userS, " \n\""); 

		while(ptr!=NULL) 									//here i insert the word in the cell
		{
			words[i] = (char*)malloc(strlen(ptr));
			if (words == NULL)
			{
				printf("unableto alocate memory (words array)");
			}
			strcpy(words[i++],ptr);
			ptr = strtok(NULL," \n\"");
		}

		words[i] = NULL;												//the last cell needs to be null
		
		int definedRedirect = redirectDefnition(words,count-1);			//here im useing definedRedirect function 
																		//in order to define in witch case im handling
		if(definedRedirect!=-1)
		{
			int redirectPlace;
			if( definedRedirect == 1 ) 	
				 redirectPlace = locateIndex(words,count-1, ">");
			else if( definedRedirect == 2 ) 
				 redirectPlace = locateIndex(words,count-1,">>");
			else if( definedRedirect == 3 ) 
				 redirectPlace = locateIndex(words,count-1,"2>");
			else if( definedRedirect == 4 )
				 redirectPlace = locateIndex(words,count-1,"<");

			redirectFunction(words,count,redirectPlace,definedRedirect);
			freeMem(words,count-1);
			continue;
		}
		
		int location=locateIndex(words, count, "|");
		if (location != -1)					//checks if there is a pipe
		{									//defines location
			cmdCounter = cmdCounter + 1;
			sumOfLetter = strlen(words[0]) + sumOfLetter;			
			pipeFunction(words, location, count);  			//opporate pipe 
		}
		
		if (strcmp(words[0], "done") == 0) 		//if the user wants to close the program
		{
			printf ( "%d \n", cmdCounter);
			printf ( "%d \n", sumOfLetter);
			printf ( "Bye!\n");
			freeMem(words,count);
			exit(0);
		}
		
		else
		{	
			execute(words, &cmdCounter, &sumOfLetter, count); //calls the function
		}
	}
}
