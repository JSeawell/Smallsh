#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>


/**************************************************
Name: Jake Seawell
Date: 07/25/19
Assignment: prog3 - smallsh
Description: This program simulates a small bash
shell. It prompts the user with a colon deliniated
command line, and has 3 built-in functions: cd,
exit, and status. All other commands given will
create a child process, and run the command.
**************************************************/


//Vars needed for expand$$ToPid function
char* expandedArgument;
char newStr[2054]; //2048 + 6 for pid expansion

//exit status var - GLOBAL
int myStatus = 0;

//This function takes for input a string, and a process ID;
//Inside the string, any instance of "$$" will be replaced
//by the process ID. The resulting string is returned
char* expand$$ToPid(char* str, int pid)
{
    	memset(newStr, '\0', 2054*sizeof(char));
    	int length = (int)strlen(str);
    	int i = 0;
    	for (i = 0; i < length; i++) 
    	{
    		if (str[i] == '$' && str[i+1] == '$'){
			sprintf(newStr + strlen(newStr), "%d", pid);
			i++;
		}
		else
			sprintf(newStr + strlen(newStr), "%c", str[i]);
    	}
    	expandedArgument = &newStr[0];
    	return expandedArgument;
}



//This function takes an array of strings, and the number of strings
//and prints them to the console.
//This function was used for testing purposes, and has thus been
//commented out for concision.
/*
void printStringArray (char strArr[516][10], int arrSize)
{
	int i = 0;
	for (i = 0; i < arrSize; i++){
		printf("Argument #%d: %s\n", i + 1, strArr[i]);
	}
}
*/


//This function takes a string, and parses that string by taking
//each space separated word, and putting it into an array
//This function returns the number of elements in the new array
int bigStringTOstringArray (char* bigStr, char strArr[516][10])
{
	int total = 0;
	char* token = strtok(bigStr, " ");

	int i = 0;
	while (token != NULL){
		strcpy(strArr[i], token);
		i++;
		total ++;
		token = strtok(NULL, " ");
	}
	return total;
}


//This function redirects stdout to a givenfilename
//This function was modified by professor/course
//provided code in the lecture videos
stdoutToFile (char* outputFilename)
{
	int targetFD;
	if (strcmp(outputFilename, "/dev/null") == 0)
		targetFD = open("/dev/null", O_WRONLY);
	else
		targetFD = open(outputFilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (targetFD == -1){
		printf("Could not open filename: '%s' for writing\n", outputFilename);
		fflush(stdout);
		exit(1);
	}
	else{
		//printf("sending stdout to: %s\n", outputFilename);
		int result = dup2(targetFD, 1);

		if (result == -1){
			perror("dup2()");
			exit(1);
		}
	}
}


//This function redirects a given filename to stdin
//This function was modified by professor/course
//provided code in the lecture videos
void fileToStdin (char* inputFilename)
{
	if (inputFilename == NULL){
		printf("No argument given, std in still pointed at keyboard\n");
		fflush(stdout);
	}
	else{
		int targetFD;
		if (strcmp(inputFilename, "dev/null") == 0)
			targetFD = open(inputFilename, O_RDONLY);	
		else
			targetFD = open(inputFilename, O_RDONLY, 0644);

		if (targetFD == -1){
			printf("Could not open filename: '%s' for reading\n", inputFilename);
			fflush(stdout);
			exit(1);
		}
	 	else{
			//printf("getting stdin from: %s\n", inputFilename);
			int result = dup2(targetFD, 0);

			if (result == -1){
				perror("dup2()");
				exit(1);
			}
		}
	}
}



//This function takes a vector of arguments, with the
//last arg being NULL, and runs the execp function on it
void execute(char** argv)
{
	if (execvp(*argv, argv) < 0){
		perror("Execute function failed");
		exit(1);
	}
}

//Vars needed for signal handlers
volatile sig_atomic_t fgoMode = 0;
volatile sig_atomic_t inputRecieved = 0;

//This custom signal handler catches CTRL+Z, 
//but instead of teminating the processes,
//it toggles foreground-only mode on and off.
//All child processes should ignore this signal instead.
void catchSIGTSTP(int signo)
{
	if (fgoMode == 0){
		char* message = "\nEntering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 50);
		fgoMode = 1;
	}
	else if (fgoMode == 1){
		char* message = "\nExiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 30);
		fgoMode = 0;
	}
	if (inputRecieved == 0){
		char* message2 = ": ";
		write(STDOUT_FILENO, message2, 2);
	}
}


//This is a custom signal handler for SIG_INT signal (CTRL+C)
void catchSIGINT(int signo)
{
  	//This code should not run
  	//Parent ignores this signal, 
  	//foreground child catches it 
  	//and temrinates itself
	char* message = "SIGINT caught. Terminating foreground child\n";
  	write(STDOUT_FILENO, message, 44);
}


//Global Vars defined
char outRedirection[256];
char inRedirection[256];

int numArgs = 0;


//Main function - Runs smallsh bash simulator
void main()
{

	sigset_t TSTP_BLOCK;
	sigaddset (&TSTP_BLOCK, SIGTSTP);

	struct sigaction ignore_action = {0};
	ignore_action.sa_handler = SIG_IGN;	
	sigfillset(&ignore_action.sa_mask);
	//ignore_action.sa_flags = SA_RESTART;

  	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = catchSIGINT;
  	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &ignore_action, NULL);
 	
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);	

	int numCharsEntered = -5; // How many chars we entered
	int currChar = -5; // Tracks where we are when we print out every char
	int exitStatus = -5;
	int childExitMethod = -5;
	int totalEntries = 0;
	int exitType = -5;
	int numChildPids = 0;
	int childPidsCleaned = 0;
	//Space for 100 background child processes
	int childPidArray[100];

	size_t bufferSize = 0; // Holds how large the allocated buffer is
	
	char* lineEntered = NULL; // Points to a buffer allocated by getline() that holds our entered string + \n + \0
	char* inputFilename;
	char* outputFilename;
	char* argVector[516];

	char devNull[9] = "/dev/null";
	char command[256];
	char argument[256];
	char FBground[1];
	char inputArray[516][10];
	char argArray[516][10];

	pid_t parentPid = getpid();
	pid_t childPid = -5;


//Until user exits by typing command: "exit"
  while(1)
  {
    	//Clear var contents each time loop is run
    	memset(newStr, '\0', 2054*sizeof(char));
    	memset(command, '\0', 256*sizeof(char));
    	memset(argument, '\0', 256*sizeof(char));
    	memset(inRedirection, '\0', 256*sizeof(char));
    	memset(outRedirection, '\0', 256*sizeof(char));
    	memset(FBground, '\0', sizeof(char));
    	numArgs = 0;

    	// Get input from the user
    	while(1)
    	{
      		//check for child zombie processes, 
      		//and clean them up if they have returned
		int i = 0;
		//For each childPid in array
		for (i=0; i<numChildPids; i++){
			//If childPid has not been cleaned-up/waited for
			if (childPidArray[i] != -5){
				//printf("Checking if child(%d) at index: %d has finished\n", childPidArray[i], i);
				//clean-up childPid if it has finished	
				if (waitpid(childPidArray[i], &childExitMethod, WNOHANG) != 0){
					//printf("Child(%d) finished. Cleaning it up\n", childPidArray[i]);
					//If process exited normally
					if (WIFEXITED(childExitMethod) != 0){
						exitStatus = WEXITSTATUS(childExitMethod);
						printf("background pid %d is done: exit value %d\n", childPidArray[i], exitStatus);
					}
					//If process was killed by signal
					else if (WIFSIGNALED(childExitMethod) != 0){
						exitStatus = WTERMSIG(childExitMethod);
						printf("background pid %d is done: terminated by signal %d\n", childPidArray[i], childExitMethod);
					}
					//Set exit value
					myStatus = exitStatus;
					//printf("background pid %d is done\n", childPidArray[i]);
					fflush(stdout);
					childPidArray[i] = -5;
					childPidsCleaned ++;
				}
				else
					//printf("Child(%d) still processing. Will check on it later. Going to next\n", childPidArray[i]);
				if (childPidsCleaned == numChildPids){
					//printf("All child pids cleaned. No zombies!! Resetting childPidArray\n");
					numChildPids = 0;
					childPidsCleaned = 0;
				}
				//Go to next pid in array
			}
		}


/********************************************************/
// This portion of code was provided and has been modified
// for my program needs.
		
		//Display prompt
		printf(": ");
      		fflush(stdout);
		inputRecieved = 0;
     	
	
		// Get a line from the user
      		numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
      		inputRecieved = 1;

		if (numCharsEntered == -1)
        		clearerr(stdin);
      		else
			break; // Exit the loop - we've got input
   	}
	
	//Block SIG_TSTP (CTRL+Z)
	sigprocmask(SIG_BLOCK, &TSTP_BLOCK, NULL);

   	//Remove newline char from input string
	lineEntered[strcspn(lineEntered, "\n")] = '\0';

// End of provided/modified code
/********************************************************/

	//Expand $$ into pid in input string, store back in same string
	expandedArgument = expand$$ToPid(lineEntered, parentPid);			
	lineEntered = expandedArgument;

	//simple assignment for built-in functions
	sscanf(lineEntered, "%s %s", command, argument);

	//User enters nothing, or a comment - do nothing
	if (*command == '#' || numCharsEntered == 1){
		//printf("Blank input or comment.\n");
		//fflush(stdout);
	}
	//Change directory
	else if (strcmp(command, "cd") == 0){
		//No argument given
		if (strcmp(argument, "") == 0){
			chdir(getenv("HOME"));
		}
		//argument given - go to given dir
		else
			chdir(argument);
	}
	//Exit: kills the parent process
	else if (strcmp(command, "exit") == 0){
		
		//Kill all child processes first
		int i = 0;
		//For each childPid in array
		for (i=0; i<numChildPids; i++){
			kill(childPidArray[i], SIGKILL);
		}

		//Then EXIT PROGRAM
		exit(0);
	}
	//Display status var
	else if (strcmp(command, "status") == 0){
		//printf("exit type: %d, exit status: %d\n", exitType, myStatus);
		//fflush(stdout);

		if (exitType == 1)
			printf("exit value %d\n", myStatus);
		else if (exitType == 2){
			myStatus = WTERMSIG(childExitMethod);
			printf("terminated by signal %d\n", myStatus);
		}
		fflush(stdout);
	}
	//Otherwise, we have a non-built-in-function input
	else {

		//Put user input string into an array of strings,
		//separated by spaces in input
		//function returns number of different words
		totalEntries = bigStringTOstringArray(lineEntered, inputArray);

		//first input word is command
		strcpy(command, inputArray[0]);

		//Parse remaining entries
		int i = 1;
		int j = 0;
		for (i=1; i<totalEntries; i++){
			//if input
			if (strcmp(inputArray[i - 1], "<") == 0)
				strcpy(inRedirection, inputArray[i]);
			//if output
			else if (strcmp(inputArray[i - 1], ">") == 0)
				strcpy(outRedirection, inputArray[i]);
			//if last input is &
			else if (strcmp(inputArray[i], "&") == 0 && i == (totalEntries - 1))
				strcpy(FBground, inputArray[i]);
			//Otherwise, it must be an argument
			//disregard < and > signs
			else if (strcmp(inputArray[i], "<") != 0 && strcmp(inputArray[i], ">") != 0){
				strcpy(argArray[j], inputArray[i]);
				j++;
				numArgs ++;
			}
		}

		//Make argument vector for execute function
		//First argument must be the command
		argVector[0] = command;
		for (i=0; i < numArgs+1; i++){
			argVector[i] = inputArray[i];
			//printf("%s\n", argVector[i]);
		}
		//Last argument must be NULL
		argVector[numArgs+1] = NULL;

		/*	
		//Print out variables - Comment out for simplicity
		printf("Command: %s\n", command);
		printStringArray(argArray, numArgs);
		printf("in: %s\n", inRedirection);
		printf("out: %s\n", outRedirection);
		printf("&: %s\n", FBground);
		fflush(stdout);
		*/	

		//Create new process using fork
		childPid = fork();

		//if fork fails
		if (childPid == -1){
			perror("Error in fork.\n");
			exit(1);
		}
		//Child process runs this code:
		else if (childPid == 0){
		
			//ignore CTRL+Z signal
			sigaction(SIGTSTP, &ignore_action, NULL);
	
			//If foreground process
			//catch and terminate self via INT(2)
			if (strcmp(FBground, "&") != 0 || fgoMode == 1)
				sigaction(SIGINT, &SIGINT_action, NULL);

			//Comment out for simplicity
			//printf("Child(%d) created.\n", getpid());
			//printf("Child(%d) running command: %s\n", getpid(), command);
			//fflush(stdout);

			//redirect input
			inputFilename = NULL;
			
			//If background process and no input provided, redirect to dev/null
			if (strcmp(FBground, "&") == 0 && strcmp(inRedirection, "") == 0 && fgoMode == 0){
				inputFilename = "/dev/null";
				//printf("inputFile: %s\n", inputFilename);
				fileToStdin(inputFilename);
			}
			//Otherwise, redirect to specified input
			else if (strcmp(inRedirection, "") != 0){
				inputFilename = inRedirection;
				//printf("inputFile: %s\n", inputFilename);
				fileToStdin(inputFilename);
			}

			//redirect output
			outputFilename = NULL;
			
			//If background process and no output provided, redirect to dev/null
			if (strcmp(FBground, "&") == 0 && strcmp(outRedirection, "") == 0 && fgoMode == 0){
				outputFilename = "/dev/null";
				//printf("outputFile: %s\n", outputFilename);
				stdoutToFile(outputFilename);
			}
			//Otherwise, redirect to specified output
			else if (strcmp(outRedirection, "") != 0){
				outputFilename = outRedirection;
				//printf("outputFile: %s\n", outputFilename);
				stdoutToFile(outputFilename);
			}
			
			//execute function
			//this will terminate on successful exec, or exit
				execute(argVector);
				exit(0); //This line should not run
		}
		//Parent process runs this code
		else {
			//If & was not used, OR fgoMode is on,
			//wait for child to exit before returning command line access
			if (strcmp(FBground, "&") != 0 || fgoMode == 1){

				//printf("Parent(%d) waiting for child(%d).\n", parentPid, childPid);
				//Wait for child process to terminate
				waitpid(childPid, &childExitMethod, 0);
				//sleep(1); //sleep 1 sec

				//If process exited normally
				if (WIFEXITED(childExitMethod) != 0){
					exitStatus = WEXITSTATUS(childExitMethod);
					exitType = 1;
				}
				//If process was killed by signal
				else if (WIFSIGNALED(childExitMethod) != 0){
					exitStatus = WTERMSIG(childExitMethod);
					exitType = 2;
					printf("terminated by signal %d\n", exitStatus);
				}
				//Set exit value
				myStatus = exitStatus;
				//printf("Child(%d) terminated. Parent(%d) process resuming\n", childPid, parentPid);
				fflush(stdout);
				
				//resume looping
			}
			//Otherwise, & was called AND fgoMode is off,
			//Return command line access immediately
			else{
				printf("background pid is %d\n", childPid);
				//store childPid in childPidArray
				childPidArray[numChildPids] = childPid;
				//printf("Child(%d) stored in childPidArray at index: %d\n", childPid, numChildPids);
				//increment numChilds/array index
				numChildPids ++;

				//resume looping
			}
		}
		//Unblock SIGTSTP (CTRL+Z)
		sigprocmask(SIG_UNBLOCK, &TSTP_BLOCK, NULL);
        }


   	//reset lineEntered PTR to point to NULL
    	lineEntered = NULL;
  }
}
