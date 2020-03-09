#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>

void takeInput(char input[]){

	fgets(input,512,stdin);
	
	//exclude '\n' character (enter) from string-input
	int i = 0;
	while (input[i]!='\0'){
		i++;
	}
	input[i-1]='\0';
}

//--------------------------------------------------------

void parse(char input[], char* tokens[],const char del[]){

	char *token;
	int i=0;
	token = strtok(input, del);

	while( token != NULL ) {

		tokens[i]=token;
     	i ++;
      	token = strtok(NULL, del);

  	}

  	tokens[i] = NULL;

}

//-------------------------------------------------------

int sepCommands(char input[],char *cmds[], bool* success){

	char *token; 	
	int i=0,pos=0;
	
	//find all delimiters
	for(int j=0;j<strlen(input)-1;j++){
		if(input[j]=='&' && input[j+1]=='&'){
			success[pos]=true;
			pos++;
		}else if(input[j]==';'){
			success[pos]=false;
			pos++;
		}
	}

	//split the commands
	parse(input, cmds, ";&&"); //parse iput in ';' or '&&'

  	return pos+1; 	//pos+1 = number of commands
}

//--------------------------------------------------------

int executeCmd(char* tokens[]){

	int status;
	pid_t child_pid;

	child_pid = fork();

  	if(child_pid == 0) {
    	/* This is done by the child process. */
    	if (execvp(tokens[0], tokens)<0){
    		perror(tokens[0]); 
    		return -1;
    	}
 	}else if(child_pid<0){
 		perror("Fork failed");
 		return 1;
	} else {
        waitpid(child_pid, &status, WUNTRACED);
    }
	//return 0;
	if (status==0){
		return 0;
	}else{
		return -1;
	}
}	

//--------------------------------------------------------

int findPipe(char cmds[]){

	if(strchr(cmds, '|')!=NULL){
		return 1;
	}else{
		return 0;
	}
}

//--------------------------------------------------------

int findRedirection(char cmds[]){

	if(strchr(cmds, '>')!=NULL){
		return 1;
	}else if(strchr(cmds, '<')!=NULL){
		return 2;
	}else{
		return 0;
	}
}

//--------------------------------------------------------

int executeRedirection(char cmds[], int f){

	char *tokens1[512];
	char *tokens2[512];
	int status;
	pid_t child_pid;

	parse(cmds, tokens1, "<>"); //parse in spaces or '<' or '>'

	parse(tokens1[0], tokens2, " \t");

	char *token= strtok(tokens1[1], " \t");
	
	while( tokens1[1] == " \t") {

      	token = strtok(NULL, " \t");

  	}

	char *fileName= token;

	FILE* file = fopen(fileName, "a+");
	 
	child_pid = fork();

  	if(child_pid == 0) {
    	/* This is done by the child process. */
    	if(f==1){
    		close(1); //close stdout
			dup2(fileno(file),fileno(stdout)); //stout goes to file        	

		}else if(f==2){
			close(0); //close stdin
			dup2(fileno(file),fileno(stdin)); //stdin is in file
		}

    	if (execvp(tokens2[0], tokens2)<0){
    		perror(tokens2[0]); 
    		exit(1);
    	}
 	}else if(child_pid<0){
 		perror("Fork failed");
 		dup2(fileno(file),fileno(stderr)); //stderr goes to file
 		exit(1);
	} else {
		fclose(file);
        waitpid(child_pid, &status, WUNTRACED);
    }

	if (status==0){
		return 0;
	}else{
		return -1;
	}

}

//------------------------------------------------------

int executePipe(char cmds[]){
	int status1, status2;
	pid_t pid1, pid2;
	int fds[2];
	char *tokens1[512];
	char *tokens2[512];
	char *tokens3[512];

	parse(cmds, tokens1, "|"); //tokens1 contains the cmds 

	// Create a pipe.
	if(pipe(fds) == -1){
		perror("Pipe failed");
        return -1;
	}

	int r1,r2;
	r1=findRedirection(tokens1[0]);
	r2=findRedirection(tokens1[1]);

	// Create our first process.
	pid1 = fork();
	if (pid1 < 0) {
      perror("Fork failed");
      exit(1);
    }else if (pid1 == 0) { 
        dup2(fds[1], 1);
        close(fds[0]);
        if(r1==0){
        	parse(tokens1[0], tokens2, " \t"); //tokens2 the 1st cmd(pefore pipe)
        	execvp(tokens2[0], tokens2); // run command BEFORE pipe character in userinput
        	perror("Execvp failed");
        	exit(1);
        }else{
        	executeRedirection(tokens1[0],r1);
        	exit(0);
        }
        
    } 
    close(fds[1]);
	// Wait for everything to finish and exit.
	waitpid(pid1, &status1, WUNTRACED);

	// Create our second process.
    pid2 = fork();
    if (pid2 < 0) {
      perror("Fork failed");
      exit(1);
    }else if (pid2 == 0) { 
        dup2(fds[0], 0);
        close(fds[1]);
        if(r2==0){
        	parse(tokens1[1], tokens3, " \t"); //tokens3 the 2nd cmd(after pipe)
        	execvp(tokens3[0], tokens3); // run command AFTER pipe character in userinput
        	perror("Execvp failed");
        	exit(1);
        }else{
        	executeRedirection(tokens1[1],r2);
        	exit(0);
        }
    }
    close(fds[0]);
    waitpid(pid2, &status2, WUNTRACED);


    if (status1==0 && status2==0){
    	return 0;
    }else{
    	return -1;
    }  
}

//--------------------------------------------------------

void parseExec(char *input){

	//initiallization
	char *cmds[512]; 	//commands stored seperately
	char *tokens[512]; 	//parts of each command stored seperately
	int numOfCmds=0;
	int redirFound=-1;
	int pipeFound=-1;
	bool *success=(bool *)malloc(512*sizeof(bool));
	
	//parse multiple commands
	numOfCmds = sepCommands(input, cmds, success);

	
	//parse and execute each command
	for (int i =0 ; i<numOfCmds; i++){

		//check exit
		if(strcmp(cmds[i],"quit") == 0){
			exit(0);
		}

		pipeFound=findPipe(cmds[i]);
		redirFound=findRedirection(cmds[i]);

		if(pipeFound==0 && redirFound==0){
			//Neither pipe nor redirection
			parse(cmds[i], tokens, " \t"); //parse space
	
			if (executeCmd(tokens)==-1 && success[i]==1){
				break;
			}
		
		}else if(pipeFound==0 && (redirFound==1 || redirFound==2)){
			//Redirection only
			if (executeRedirection(cmds[i],redirFound)==-1 && success[i]==1){
				break;
			}

		}else{
			if (executePipe(cmds[i])==-1 && success[i]==1){
				break;
			}
		}
	}

}

//-------------------------------------------------------------------

void shell(){
	//my name is
	printf("karatza_8828>");
	
	//take input from user
	char input[512]; //string-input from user
	takeInput(input); 

	//parse and execute the command 
	parseExec(input);

	//recursion
	shell();
}

//--------------------------------------------------------------

int batch(char * argv[]){

	//initiallization
	char str[512];
	char *batch_cmds[512];
	int i=0, j=0; 
	int num;

	//open the batch file
	FILE *fp;
	fp = fopen(argv[1], "r");

	//error handling
	if(fp == NULL) {
  		perror("Error opening file");
  		return(-1);
	}
	
	//seperate the commands from the batchfile 	
	while(fgets(str,512, fp)!=NULL){
		batch_cmds[i] = strdup(str);
		i++;
		num = i;
	}

	//parse and execute each command 
	for (i=0;i<num;i++){
		//exclude /n from the end of the cmd
		for (j=0;j<512;j++){
			if (batch_cmds[i][j]=='\n'){
				batch_cmds[i][j]='\0';
			}
		}
		parseExec(batch_cmds[i]);
	}
	
	fclose(fp);

	return 0;
}

//-----------------------------------------------------------------------

int main(int argc, char *argv[]){
	if (argc >=2){
		//batchfile mode
		batch(argv);

	}else{
		//interactive mode
		shell();
	}
	
	return 0;
}