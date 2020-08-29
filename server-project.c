#include <stdio.h>      
#include <sys/types.h>
#include <sys/socket.h>   
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <math.h>

#define WELCOME "SERVER, GET YOUR IP : "
#define CORRECT 1
#define ERROR 0
#define MAX_BUFFER_SIZE 512

void replaceEndOfString(char* str);
int convertInteger(char str[]);
void resetBuffer(char* buffer);
int checkMessage(char buffer[]);

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

int main(int argc, char *argv[]) {

    int mySocket = 0;
    int serverPort = 0;
    int returnStatus = 0;
    struct sockaddr_in server;
    char buffer[MAX_BUFFER_SIZE] = "";
	char MESSAGE[MAX_BUFFER_SIZE] = ""; /*<Esito> <Tipo> <Contenuto>*/
	char *token;
	int arrData[255];
	
	socklen_t len;
	struct sockaddr_storage addr;
		
	int arrDataIndex = 0;
	int tmp = 0;
	int totalElement = 0;
	int nTmpElements = 0;
	int elemCalculated = 0;
	int indexToken = 0;
	int valid = 1;
	
    if (2 != argc) 
	{
        fprintf(stderr, "Number of paramter Incorrect. Usage: %s <port>\n", argv[0]);
        exit(1);
    }
	
    mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (mySocket == -1) 
	{
        fprintf(stderr, "Could not create a socket!\n");
        exit(1);
    }
    else 
	{
	    fprintf(stderr, "Socket created!!\n");
    }

    /* retrieve the port number for listening */
    serverPort = atoi(argv[1]);

    /* setup the address structure */
    /* use INADDR_ANY to bind to all local addresses  */
    memset(&server, '\0', sizeof(server)); 
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(serverPort);

    /*  bind to the address and port with our socket  */
    returnStatus = bind(mySocket,(struct sockaddr *)&server,sizeof(server));

    if (returnStatus == 0) 
	{
	    fprintf(stderr, "Bind completed!!\n");
    }
    else 
	{
        fprintf(stderr, "Could not bind to address!\n");
		close(mySocket);
		exit(1);
    }

    /* lets listen on the socket for connections      */
    returnStatus = listen(mySocket, 5);

    if (returnStatus == -1) 
	{
        fprintf(stderr, "Problem! Cannot listen on socket!\n");
		close(mySocket);
        exit(1);
    }

    while (1)
    {
        struct sockaddr_in clientName = { 0 };
		int simpleChildSocket = 0;
		int clientNameLength = sizeof(clientName);
		
		arrDataIndex = 0;
		tmp = 0;
		totalElement = 0;
		nTmpElements = 0;
		elemCalculated = 0;
		indexToken = 0;
		valid = 1;		

		/* wait here the accept*/
        simpleChildSocket = accept(mySocket,(struct sockaddr *)&clientName, &clientNameLength);

		if (simpleChildSocket == -1) {
			fprintf(stderr, "Cannot accept connections!\n");
			close(mySocket);
			exit(1);
		}


		char *clientIP = inet_ntoa(clientName.sin_addr);

		/* handle the new connection request  */
		/* write out our message to the client */

		resetBuffer(MESSAGE);
		strcpy(MESSAGE,"OK START ");
		strcat(MESSAGE, WELCOME);
		strcat(MESSAGE, clientIP);
		strcat(MESSAGE, "\n");
		//fprintf(stderr,"%s", MESSAGE);
		write(simpleChildSocket, MESSAGE, strlen(MESSAGE));

		resetBuffer(MESSAGE);
		do
		{
			tmp = 0;			
			nTmpElements = 0;
			elemCalculated = 0;
			indexToken=0;
			valid = 1;
			/* WAITING <number of data> <data1> <data2> <dataN>*/
			returnStatus = read(simpleChildSocket, buffer, sizeof(buffer) - 1);
			if (returnStatus <= 0) {
				fprintf(stderr, "Error read client message\n");
				close(simpleChildSocket);
				exit(1);
			}
			if(checkMessage(buffer)==ERROR){
				close(simpleChildSocket);
				exit(1);
			}
			
			//printf("MESSAGE RECIVE: %s", buffer);
			token = strtok(buffer, " ");
			
			while( token != NULL ) {
				//printf("token %s\n",token);
				if(indexToken==0){
					if(strcmp(token,"0\n")==0){
						tmp=0;
						//printf("FIRST TOKEN is zero? %s", token);						
					}else{				
						tmp = convertInteger(token);
						
						//First Element
						if(tmp<0){
							strcpy(MESSAGE,"ERR DATA The First element is not positive integer\n");
							valid = 0;
							break;
						}
						nTmpElements = tmp;
					}
				}else{
					tmp = convertInteger(token);
					if(tmp<0){
						strcpy(MESSAGE,"ERR DATA The input must be a positive number\n");
						valid = 0;
						break;						
					}
					arrData[arrDataIndex] = tmp;
					arrDataIndex +=1;
					elemCalculated +=1;
				}
				indexToken +=1;				
				token = strtok(NULL, " ");				
		    }
			totalElement += nTmpElements;
			
			//printf("ELAB: %d, FIRST ELE: %d\n",elemCalculated,nTmpElements);
			
			//Check NElementElab
			if(valid==1){
				if(nTmpElements != elemCalculated){
					strcpy(MESSAGE,"ERR DATA First element is not consistent with the elements sent\n");
					write(simpleChildSocket, MESSAGE, strlen(MESSAGE));			
					close(simpleChildSocket);
					break;					
				}else{
					if(tmp!=0){
						strcpy(MESSAGE,"OK DATA ");
						resetBuffer(buffer);
						sprintf (buffer, "%d", nTmpElements);
						strcat(MESSAGE,buffer);
						strcat(MESSAGE,"\n");
						write(simpleChildSocket, MESSAGE, strlen(MESSAGE));												
					}
				}
			}else{
				write(simpleChildSocket, MESSAGE, strlen(MESSAGE));		
				close(simpleChildSocket);
				break;					
			}

			resetBuffer(MESSAGE);
			resetBuffer(buffer);
		}while(tmp != 0);

		if (tmp == 0 && totalElement == 0) {
			strcpy(MESSAGE,"ERR STATS 0 number received and NO other numbers Sended before.\n");
			write(simpleChildSocket, MESSAGE, strlen(MESSAGE));	
		}
		else if(totalElement==1){
			strcpy(MESSAGE,"ERR STATS The popolation should be greater than 1\n");
			write(simpleChildSocket, MESSAGE, strlen(MESSAGE));	
		}else{
			//CALC AVG 
			float avg;
			int tot=0;
			int i;
			float var;
			for(i = 0; i < totalElement; i++){
				tot=tot + arrData[i];
			}
			avg = (float)tot/(float)totalElement;
			
			for(i = 0; i < totalElement; i++){
				var += pow((float)arrData[i] - avg,2);
			}
			
			var = var / totalElement;
			
			strcpy(MESSAGE,"OK STATS ");
			sprintf(buffer, "%d ", totalElement);
			strcat(MESSAGE,buffer);
			resetBuffer(buffer);
			
			sprintf(buffer, "%.2f ", avg);
			strcat(MESSAGE,buffer);
			resetBuffer(buffer);
			
			sprintf(buffer, "%.2f\n", var);
			strcat(MESSAGE,buffer);
			resetBuffer(buffer);

			//printf("MESSAGE STATS: %s\n", MESSAGE);
			write(simpleChildSocket, MESSAGE, strlen(MESSAGE));		
		}
    }

    close(mySocket);
    return 0;
}

void resetBuffer(char* buffer){
	memset(buffer, 0, MAX_BUFFER_SIZE);
}

int convertInteger(char str[]) {
    replaceEndOfString(str);
	printf("atoi %s \n", str);
	int number = atoi(str);
	if(number == 0)
		return -1;
    return number;
}

void replaceEndOfString(char* str) {
    if (str[strlen(str) - 1] == '\n')
        str[strlen(str) - 1] = '\0';
}

int checkMessage(char buffer[]){
	if (strlen(buffer) > MAX_BUFFER_SIZE) {
        printf("ERROR Message too long: %lu char\n", strlen(buffer));
        return ERROR;
    }
	
    if (buffer[strlen(buffer)-1] != '\n') {
        printf("ERROR End Line Character not found\n");
        return ERROR;
    }
}
