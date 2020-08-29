#include <stdio.h>      
#include <sys/types.h>
#include <sys/socket.h>   
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define CORRECT 1
#define ERROR 0
#define MAX_BUFFER_SIZE 512
typedef enum {
	OK,
    ERR
} result;

typedef enum {
	START,
    SYNTAX,
	DATA,
	STATS
} responseType;

typedef struct {
	result state;
	responseType type;
	char message[MAX_BUFFER_SIZE];
	int elemCalculated;
	float avg;
	float var;
} serverResponse;

void replaceEndOfString(char* str);
int convertInteger(char* str);
int checkIfNumber(char* str);
void resetBuffer(char* buffer);
int parse_response(serverResponse* response, char buffer[]);
char* substr(char *src, int m, int n);

int main(int argc, char *argv[]) {

    int simpleSocket = 0;
    int serverPort = 0;
    int returnStatus = 0;
    char buffer[MAX_BUFFER_SIZE] = "";
	char bufferMsg[MAX_BUFFER_SIZE] = "";
	char strTmp[MAX_BUFFER_SIZE] = "";
    struct sockaddr_in server;
	
    serverResponse response;
	
    if (argc != 3) {
        fprintf(stderr, "Number of arguments incorrect.\n Usage: %s <server> <port>\n", argv[0]);
        exit(1);
    }

    /* create socket      */
    simpleSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (simpleSocket == -1){
        fprintf(stderr, "Could not create a socket!\n");
        exit(1);
    }

    serverPort = atoi(argv[2]);
    memset(&server, '\0', sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr= inet_addr(argv[1]);	
    server.sin_port = htons(serverPort);

    /*  connect to the address and port with our socket  */
    returnStatus = connect(simpleSocket, (struct sockaddr *)&server, sizeof(server));
    if (returnStatus < 0) {
        fprintf(stderr, "Could not connect to address!\n");
		close(simpleSocket);
		exit(1);
    }

    returnStatus = read(simpleSocket, buffer, sizeof(buffer));
    if (parse_response(&response, buffer)==ERROR) {
        printf("Invalid Format response. Close socket.\n");
        close(simpleSocket);
        exit(1);
    }
	
	printf("** Welcome: %s **\n", response.message);
	printf("** This program is writed to give you Average and Variance **\n");
	printf("** Insert the number of the data you want to calculate. **\n");
	printf("** After that you have requested to insert the values, one at a time. **\n");
	printf("** Insert 0 To terminate and start the calculation. **\n\n");	

	int val;
	int tmpValue;
	
	do{
		resetBuffer(buffer);
		resetBuffer(bufferMsg);
		printf("Insert how many numbers do you want to process (0 for finish): ");		
		fgets(buffer, sizeof(buffer) - 1, stdin);
		replaceEndOfString(buffer);
		if(strcmp(buffer,"0")==0){
			val = 0;			
		}else{
			val = checkIfNumber(buffer) == 1 ? convertInteger(buffer): -1;			
			if(val < 0){
				printf("Value should be positive integer\n");
				continue;
			}
		}
		
		sprintf (strTmp, "%d", val);
		strcat(bufferMsg,strTmp);
		if (val != 0) {
			strcat(bufferMsg," ");
		}

		for (int i = 0; i <= val; i++ )
		{
			if( i < val) {

				tmpValue = 0;
				resetBuffer(buffer);


				printf("Insert %d° number : ",i+1);
				fgets(buffer, sizeof(buffer) - 1, stdin);
				tmpValue = checkIfNumber(buffer)== 1 ? convertInteger(buffer): -1;
				if(tmpValue<=0){
					printf("number not correct\n");
					i=i-1;
					continue;
				}
				sprintf (strTmp, "%d", tmpValue);
				strcat(bufferMsg,strTmp);
				if (i+1 != val) {
					strcat(bufferMsg," ");
				}
			} else {
				strcat(bufferMsg,"\n");
				printf("sto mettendo LF %s \n", bufferMsg);
				if (bufferMsg[strlen(bufferMsg)-1] != '\n') {
     			   printf("ERROR End Line Character not found\n");
        			
    			}

			}
		}

				
		write(simpleSocket, bufferMsg, strlen(bufferMsg));

		if(val!=0){
			//WAITING RESULT CONFIRM RECIVE DATA
			resetBuffer(buffer);
			
					
			returnStatus = read(simpleSocket, buffer, sizeof(buffer));
			if (parse_response(&response, buffer)==ERROR) {
				printf("Format response not valid\n");
				close(simpleSocket);
				exit(1);
			}
			
			if(response.state==ERR){
				printf("ERROR %s\n",response.message);
				close(simpleSocket);
				exit(1);			
			}else{
				tmpValue = checkIfNumber(buffer)== 1 ? convertInteger(response.message): -1;
				if(tmpValue == val){
					printf("Server has successfully received %s elements\n",response.message);			
				}else{
					printf("Server has reviced incoerent number of elements\n");													
				}
			}
		}

	}while(val != 0);
	
	resetBuffer(buffer);
	returnStatus = read(simpleSocket, buffer, sizeof(buffer));
	if (parse_response(&response, buffer)==ERROR) {
		printf("Format response not valid\n");
		close(simpleSocket);
		exit(1);
	}
	if(response.state==ERR){
		printf("ERROR %s\n",response.message);
		close(simpleSocket);
		exit(1);			
	}else{
		printf("N° elements: %d\n", response.elemCalculated);
		printf("AVG: %.2f\n", response.avg);		
		printf("Variance: %.2f\n", response.var);				
	}

    close(simpleSocket);
    return 0;
}

int convertInteger(char str[]) {
    replaceEndOfString(str);
	int number = atoi(str);
	if(number == 0)
		return -1;
    return number;
}

int checkIfNumber(char str[]){
	//replaceEndOfString(str);
    int length,i; 
    length = strlen (str);
	printf ("string %s %d\n", str, length);
	int charAscii;
    for (i=0;i<length; i++)
		charAscii = (int)str[i];
		// convert char in ASCII code and check if is not a number
        if (charAscii < 48 || charAscii > 57)
        {
            printf ("Entered input is not a positive number %c\n", str[i]);
            return -1;
        }
    //printf ("Given input is a number\n");
	return 1;
}


char* substr(char *src, int m, int n){
	int len = n - m;
	char *dest = (char*)malloc(sizeof(char) * (len + 1));
	strncpy(dest, (src + m), len);
	return dest;
}
int parse_response(serverResponse* response, char buffer[]) {
	char bufferCopy[MAX_BUFFER_SIZE];
	stpcpy(bufferCopy, buffer);
	
    if (buffer[strlen(buffer)-1] != '\n') {
        printf("ERROR End Line Character not found\n");
        return ERROR;
    }
	if (strlen(buffer) > MAX_BUFFER_SIZE) {
        printf("ERROR Message exceeds: %lu char\n", strlen(buffer));
        return ERROR;
    }
	replaceEndOfString(buffer);
	resetBuffer(response->message);
	
	int nToken = 0;
	char *token;	
	int intTmp = 0;
	int startMsg = 0;
    /* get the first token */
    token = strtok(buffer, " ");

	while( token != NULL ) {
		switch (nToken)
		{
			case 0:
				if (strcmp(token, "ERR") == 0){
					response->state = ERR;
					startMsg = 4;					
				}else if (strcmp(token, "OK") == 0){
					response->state = OK;
					startMsg = 3;

				}else{
					printf("Invalid state");
					return ERROR;					
				}
				break;
			case 1: 
				if (strcmp(token, "SYNTAX") == 0){
					response->type = SYNTAX;
					startMsg += 7;					
				}else if (strcmp(token, "START") == 0){
					response->type = START;
					startMsg += 6;
				}else if (strcmp(token, "STATS") == 0){
					response->type = STATS;						
					startMsg += 6;					
				}else if (strcmp(token, "DATA") == 0){
					response->type = DATA;		
					startMsg += 5;
				}else{
					printf("Invalid type");
					return ERROR;					
				}
				break;
			case 2:
				if(response->type==STATS && response->state==OK){
					response->elemCalculated = checkIfNumber(buffer)== 1 ? convertInteger(token): -1;
				}else{
					strcpy(response->message, substr(bufferCopy, startMsg, strlen(bufferCopy)-1));	
					return CORRECT;					
				}
				break;
			case 3:
				if(response->type==STATS){
					response->avg = atof(token);
				}else{				
					printf("Error!! Too much tokens, '%s', number=%d, type=%d\n",token,nToken,response->type);
					return ERROR;
					break;				
				}
				break;				
			case 4:
				if(response->type==STATS){
					response->var = atof(token);
				}else{				
					printf("Error!! Too much tokens, '%s', number=%d, type=%d\n",token,nToken,response->type);
					return ERROR;
					break;				
				}				
				break;
			default: 
				printf("Error!! Too much tokens, '%s', number=%d\n",token,nToken);
				return ERROR;
				break;
		}
		nToken +=1;
		token = strtok(NULL, " ");
	}
    return CORRECT;
}

void resetBuffer(char* buffer){
	memset(buffer, 0, MAX_BUFFER_SIZE);
}

// replace \n with \0 termination
void replaceEndOfString(char* str) {
    if (str[strlen(str) - 1] == '\n')
        str[strlen(str) - 1] = '\0';
}
