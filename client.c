#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <pthread.h>

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5550
#define BUFF_SIZE 1024
#define FIND_FILE_REQUEST "Find file"
#define FILE_NOT_FOUND "File not found"
#define FILE_EXIST "File exist"
#define SEND_FILE_REQUEST "Send file"
#define SEARCH_MODE "Search mode"
#define SHARE_MODE "Share mode"

#define STORAGE "./share/"

int searchFile(char *filename);
void sendFile(int client_sock,char *fileName);
void receiveFile(int client_sock, char *fileName);

void doSearchFile(int client_sock);
void doSendFile(int client_sock);

void searchMode(int client_sock);
void shareMode(int client_sock);


typedef struct tagClient
{
    char addressIP[256];
    char id[20];
    struct tagClient *next;
} client;

int client_sock;
struct sockaddr_in server_addr;
int under_client_sock;
pthread_t tid;
client *head = NULL;
client *current = NULL;

client* findById(char *id){
   client* current = head;
   char idCheck[20];
   if(head == NULL){
      return NULL;
   }
   while(current != NULL ){
   	  strcpy(idCheck,current->id);
   	  if (strcmp(idCheck,id) == 0)
   	  {
   	  	return current;
   	  }
      current = current->next;
   }
   return current;
}

void InsertHeadList(char *id,char *addressIP)
{
	client *new = (client *)malloc(sizeof(client));

	strcpy(new->addressIP,addressIP);
	strcpy(new->id,id);
	new->next=head;
	head=new;
}

char *trymString(char * str)
{
	const char *s="-\n";
	char *token;
	char addressIP[256];
	char id[20];
   token = strtok(str, s);
   int i=0;
   while( token != NULL ) 
   {	
   		printf("token :%s\n", token);	
      if(i%2==0)
      {
      	strcpy(addressIP,token);
      	
      }else
      {
      	strcpy(id,token);
      	InsertHeadList(id,addressIP);
      }
    	i++;
      token = strtok(NULL, s);
   }
   return token;
}

void sendFile(int client_sock, char *fileName){
    
    FILE *fileptr;
    long filelen=0l;
    int bytes_sent;
    char name[100];
	strcpy(name,STORAGE);
	strcat(name,fileName);

    fileptr = fopen(name, "rb");
    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    filelen = ftell(fileptr);             // Get the current byte offset in the file       
    rewind(fileptr);

    printf("%s\n", "Ham gui file");

    bytes_sent = send(client_sock, &filelen, 20, 0); //send file size
    if(bytes_sent <= 0){
        printf("\nConnection closed!\n");
        return;
    }

    printf("filelen: %ld\n", filelen);

    int sumByte = 0; //count size byte send
    printf("\n Bat dau doc file !\n");
    while(1) {
        int numberByteSend = BUFF_SIZE;
        if((sumByte + BUFF_SIZE	) > filelen) {// if over file size
            numberByteSend = filelen - sumByte; 
        }
        char* buffer = (char *) malloc((numberByteSend) * sizeof(char));
        fread(buffer, numberByteSend, 1, fileptr); // read buffer with size 
        sumByte += numberByteSend; //increase byte send
        printf("%s\n", buffer);
        bytes_sent = send(client_sock, buffer, numberByteSend, 0);
        if(bytes_sent <= 0){
            printf("\nConnection closed!\n");
            break;
        }
        free(buffer);
        if(sumByte >= filelen) {
            break;
        }
    }
    printf("%s\n", "Send file success !"); 
    fclose(fileptr); //close file
}

void searchMode(int client_sock){
	char filename[BUFF_SIZE];
	int bytes_sent;
	int bytes_received;
	char listClientShare[BUFF_SIZE];
	char buff[BUFF_SIZE];
	char minus[] = "-";
    char enter[] = "\n";
    char *token;
    char clientStr[256];
    char clientID[20];
    client *ptr;
	bytes_sent = send(client_sock, SEARCH_MODE, BUFF_SIZE, 0);
	if(bytes_sent < 0)
		perror("\nError: ");
	printf("%s\n", "Enter File name: ");
	scanf("%s",filename);
	// printf("File name: %s\n", filename);
	bytes_sent = send(client_sock, filename, BUFF_SIZE, 0);
	bytes_received =  recv(client_sock, buff, BUFF_SIZE, 0);
	buff[bytes_received] = '\0';
	if(strcmp(buff,FILE_NOT_FOUND) ==0 ){
		printf("%s\n", FILE_NOT_FOUND);
		return;
	}
	strcpy(listClientShare, buff);
	trymString(listClientShare);
   	printf("%s\n", "============================");
   	printf("%s\n", "List client has file :");
   	// printf("%s", listClientShare);
   	client *tmp = head;
   	int i = 0;
   	printf("%-5s %-15s %-25s \n", "STT","IP Address","ID");
   	while(tmp != NULL){
   		i++;
   		printf("%-5d %-15s %-25s \n",i,tmp->addressIP,tmp->id);
   		tmp = tmp ->next;
   	}
   	printf("%s\n", "============================");
   	printf("%s\n", "Enter ID Client to get file :");
   	while(1){
   		scanf("%s", clientID);
   		tmp = findById(clientID);
   		if (tmp != NULL)
   		{
   			break;
   		}else{
   			printf("%s\n", "Client not exist, Enter ID Client again :");
   		}

   	}
   	
   	bytes_sent = send(client_sock, clientID, BUFF_SIZE, 0);

   	receiveFile(client_sock,filename);
   	head = NULL;

}

void receiveFile(int client_sock, char *fileName){
	FILE *filePtr;
	char name[100];
	char *fileContent;
	int bytes_received;
	long filelen=0l;

	strcpy(name, STORAGE); 
	strcat(name, fileName);

	printf("%s\n", "Ham nhan file");

	bytes_received = recv(client_sock, &filelen, 20, 0);
	if (bytes_received <= 0){
		printf("\nConnection closed");
		return;
	}
	printf("Uploaded file: %s, Size of file: %ld\n\n", fileName, filelen);

	int sumByte = 0;
	filePtr = fopen(name, "wb");
	fileContent = (char*) malloc(BUFF_SIZE * sizeof(char));
	while(1) {
		bytes_received = recv(client_sock, fileContent, BUFF_SIZE, 0);
		if(bytes_received == 0) {
			printf("Error: File tranfering is interupted\n\n");
		}
		sumByte += bytes_received;
		fwrite(fileContent, bytes_received, 1, filePtr);
		free(fileContent);
		fileContent = (char*) malloc(BUFF_SIZE * sizeof(char));
		if(sumByte >= filelen) {
			break;
		}
	}
	printf("%s\n", "Receive file success !"); 
	fclose(filePtr);
}

int initSock(){
	int newsock = socket(AF_INET, SOCK_STREAM, 0);
	if (newsock == -1 ){
		perror("\nError: ");
		exit(0);
	}
	return newsock;
}


void *backgroundHandleStart(){
	int bytes_sent, bytes_received;
	char buff[BUFF_SIZE + 1];
	char fileName[BUFF_SIZE + 1];


	printf("%s\n", "Waiting File name");
	bytes_received = recv(under_client_sock, fileName, BUFF_SIZE, 0);
	fileName[bytes_received] = '\0';
	printf("Finding file : %s \n", fileName);
	if (searchFile(fileName) == 1)
	{	
		printf("%s\n", FILE_EXIST);
		bytes_sent = send(under_client_sock, FILE_EXIST, BUFF_SIZE, 0);
		bytes_received =  recv(under_client_sock, buff, BUFF_SIZE, 0);
		buff[bytes_received] = '\0';
		printf("%s\n", buff);
		if (strcmp(buff,SEND_FILE_REQUEST) == 0){
			sendFile(under_client_sock,fileName);
		}

	}else{
		printf("%s\n", FILE_NOT_FOUND);
		bytes_sent = send(under_client_sock, FILE_NOT_FOUND, BUFF_SIZE, 0);
	}
	backgroundHandleStart();

}

void backgroundHandle(char *idClient){
	int bytes_sent, bytes_received;
	under_client_sock = initSock();
	if(connect(under_client_sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0){
		printf("\nError!Can not connect to sever! Client exit imediately! ");
		return ;
	}
	printf("idClient: %s\n", idClient);
	printf("under_client_sock %d\n", under_client_sock);

	bytes_sent = send(under_client_sock, SHARE_MODE, BUFF_SIZE, 0);
	bytes_sent = send(under_client_sock, idClient, BUFF_SIZE, 0);
	printf("%s\n", "Sended idClient");
	printf("%s\n", "Waiting ID");
	bytes_received = recv(under_client_sock, idClient, BUFF_SIZE, 0);
	idClient[bytes_received] = '\0';
	// scanf("%s",buff); //block
	printf("id :%s\n", idClient);
	pthread_create(&tid, NULL, &backgroundHandleStart, NULL);


}


void menu(int client_sock){
	int a = 0;
	char input[256];
	int bytes_sent, bytes_received;
	char buff[BUFF_SIZE + 1];
	char idClient[BUFF_SIZE + 1];
	char listClientShare[BUFF_SIZE+1];
	char fileName[BUFF_SIZE];

	
	bytes_received = recv(client_sock, buff, BUFF_SIZE, 0);
    buff[bytes_received] = '\0';

    strcpy(idClient,buff);
    printf("idClient :%s\n", idClient);

    bytes_sent = send(client_sock, SEARCH_MODE, BUFF_SIZE, 0); //send file size

    printf("client_sock %d\n", client_sock);
    backgroundHandle(idClient);

	while(1){
		printf("%s\n", "=====================");
		printf("%s\n", "  Enter file name to find :");
		printf("( or 'quit' to quit): \n");		
		scanf("%s", input);
		if (strcmp(input,"quit") == 0 )
		{
			printf("%s\n", "Good bye");
			break;
		}

		bytes_sent = send(client_sock, input, BUFF_SIZE, 0);
		printf("filename %s\n", input);
		strcpy(fileName, input);

		bytes_received =  recv(client_sock, buff, BUFF_SIZE, 0);
		buff[bytes_received] = '\0';
		if(strcmp(buff,FILE_NOT_FOUND) !=0 ){
			strcpy(listClientShare, buff);
			trymString(listClientShare);
		   	printf("%s\n", "============================");
		   	printf("%s\n", "List client has file :");
		   	// printf("%s", listClientShare);
		   	client *tmp = head;
		   	int i = 0;
		   	printf("%-5s %-15s %-25s \n", "STT","IP Address","ID");
		   	while(tmp != NULL){
		   		i++;
		   		printf("%-5d %-15s %-25s \n",i,tmp->addressIP,tmp->id);
		   		tmp = tmp ->next;
		   	}
		   	printf("%s\n", "============================");
		   	printf("%s\n", "Enter ID Client to get file :");
		   	while(1){
		   		scanf("%s", idClient);
		   		tmp = findById(idClient);
		   		if (tmp != NULL)
		   		{
		   			break;
		   		}else{
		   			printf("%s\n", "Client not exist, Enter ID Client again :");
		   		}

	   		}
	   	
	   	bytes_sent = send(client_sock, idClient, BUFF_SIZE, 0);
		receiveFile(client_sock,fileName);
		}else{
			printf("%s\n", FILE_NOT_FOUND);
		}
		
	   	
	   	head = NULL;

		}
}

int searchFile(char *filename)
{
	FILE *fileptr;
	char name[100];
	strcpy(name,STORAGE);
	strcat(name,filename);
	if ((fileptr = fopen(name, "rb")) == NULL){
            return 0;
        }
    fclose(fileptr);
    return 1;
}

int isValidIpAddress(char const *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    if (result != 0)
        return 1;
    else
        return 0;
}

int main(int argc, char const *argv[]){
	
	if (argc != 3)
    {
        printf("error, too many or too few arguments\n");
        return 1;
    }
    //check if input id is valid
    if (isValidIpAddress(argv[1]) == 0)
    {
        printf("Not a valid ip address\n");
        return 1;
    }


	char buff[BUFF_SIZE + 1];
	int msg_len, bytes_sent, bytes_received;
	
	//Step 1: Construct socket
	client_sock = socket(AF_INET,SOCK_STREAM,0);
	
	//Step 2: Specify server address
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	
	//Step 3: Request to connect server
	if(connect(client_sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0){
		printf("\nError!Can not connect to sever! Client exit imediately! ");
		return 0;
	}
	menu(client_sock);

	close(client_sock);
	return 0;
}