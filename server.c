#include <stdio.h>
#include <string.h>
#include <stdlib.h>       /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define PORT 5550   /* Port that will be opened */ 
#define BACKLOG 20   /* Number of allowed connections */
#define BUFF_SIZE 1024
#define BUFF_SEND 1024

#define FIND_FILE_REQUEST "Find file"	
#define SEND_FILE_REQUEST "Send file"
#define FILE_NOT_FOUND "File not found"
#define FILE_EXIST "File exist"

#define SEARCH_MODE "Search mode"
#define SHARE_MODE "Share mode"
#define STORAGE "./save/"

/* Receive and echo message to client */

typedef struct tagAddress
{
    char addressIP[256];
    int *connfd;
    int id;
    struct tagAddress *next;
} client;


client *head = NULL;
client *current = NULL;

client *headShareList = NULL;
client *headSearchList = NULL;

client *findByConn(int connfd);
void *mainFunction(void *);
void searchFileHandle(int connfd);
client *insertListAddr(client addr, client *ptr);

client *insertListAddr(client addr, client *ptr)
{	
	printf("%s : %d\n",addr.addressIP,*addr.connfd );
    client *link = (client *)malloc(sizeof(client));
    strcpy(link->addressIP, addr.addressIP);
    link->connfd = (int *)malloc(sizeof(int));
    *link->connfd = *(addr.connfd);
    link->id = addr.id;
    link->next = ptr;
    ptr = link;
    printf("%s : %d : %d\n",ptr->addressIP,*ptr->connfd, ptr->id );
    return ptr;
}

client *insertClient(client *addr, client *ptr)
{	
    client *link = (client *)malloc(sizeof(client));
    strcpy(link->addressIP, addr->addressIP);
    link->connfd = (int *)malloc(sizeof(int));
    *link->connfd = *(addr->connfd);
    link->id = addr->id;
    link->next = ptr;
    ptr = link;
    printf("%s : %d : %d\n",ptr->addressIP,*ptr->connfd, ptr->id );
    return ptr;
}

client *deleteNode(client *head_ref, int key) 
{ 
    // Store head node 
    client* temp = head_ref, *prev; 
  
    // If head node itself holds the key to be deleted 
    if (temp != NULL && *temp->connfd == key) 
    { 
        head_ref = temp->next;   // Changed head 
        free(temp);               // free old head 
        return head_ref; 
    } 
  
    // Search for the key to be deleted, keep track of the 
    // previous node as we need to change 'prev->next' 
    while (temp != NULL && *temp->connfd != key) 
    { 
        prev = temp; 
        temp = temp->next; 
    } 
  
    // If key was not present in linked list 
    if (temp == NULL) 
    	return head_ref; 
  
    prev->next = temp->next; 
  
    free(temp);  // Free memory 
    return head_ref;
}

void sendFile(int connfd, char *fileName){
    
    FILE *fileptr;
    long filelen =0l;
    int bytes_sent;
    char name[100];
	strcpy(name,STORAGE);
	strcat(name,fileName);

    fileptr = fopen(name, "rb");
    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    filelen = ftell(fileptr);             // Get the current byte offset in the file       
    rewind(fileptr);

    printf("%s\n", "Ham gui file");

    bytes_sent = send(connfd, &filelen, 20, 0); //send file size
    if(bytes_sent <= 0){
        printf("\nConnection closed!\n");
        return;
    }

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
        bytes_sent = send(connfd, buffer, numberByteSend, 0);
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


void receiveFile(int conn, char *fileName){
	FILE *filePtr;
	char name[100];
	char *fileContent;
	int bytes_received;
	long filelen =0l;
	long *size;

	strcpy(name, STORAGE); 
	strcat(name, fileName);

	printf("%s\n", "Ham nhan file");
	printf("filelen: %ld\n", filelen);

	size = (long*) malloc(BUFF_SIZE * sizeof(long));

	bytes_received = recv(conn, size, 20, 0);
	if (bytes_received <= 0){
		printf("\nConnection closed");
		return;
	}
	filelen = *size;
	printf("filelen: %ld\n", filelen);
	printf("Uploaded file: %s, Size of file: %ld\n\n", fileName, filelen);

	int sumByte = 0;
	filePtr = fopen(name, "wb");
	fileContent = (char*) malloc(BUFF_SIZE * sizeof(char));
	while(1) {
		bytes_received = recv(conn, fileContent, BUFF_SIZE, 0);
		if(bytes_received == 0) {
			printf("Error: File tranfering is interupted\n\n");
		}
		printf("bytes_received: %d fileContent: %s\n",bytes_received,fileContent );
		sumByte += bytes_received;
		fwrite(fileContent, bytes_received, 1, filePtr);
		printf("%s\n", fileContent);
		free(fileContent);
		fileContent = (char*) malloc(BUFF_SIZE * sizeof(char));
		if(sumByte >= filelen) {
			break;
		}
	}
	printf("%s\n", "Receive file success !"); 
	free(fileContent);
	free(size);		
	fclose(filePtr);
}

void InsertHead(client addr)
{
    client *link = (client *)malloc(sizeof(client));
    strcpy(link->addressIP, addr.addressIP);
    if (head == NULL)
    {
    	link->id = 0;
    }else{
    	int id = head->id;
    	link->id = id+1;
    }
    link->connfd = addr.connfd;
    *link->connfd = *(addr.connfd);
    link->next = head;
    head = link;
}

void insertShareList(int connfd)
{
    client *link = (client *)malloc(sizeof(client));
    *link->connfd = connfd;
    link->next = headShareList;
    headShareList = link;
}

void insertSearchList(int connfd)
{
    client *link = (client *)malloc(sizeof(client));
    *link->connfd = connfd;
    link->next = headSearchList;
    headSearchList = link;
}

void sendRequestFindFile(int currentCon, char *fileName,int idClientFrom){
	client *pointer;
    pointer = headShareList;
    client *headListConnHasFile = NULL;
    client *headListAddrHasFile = NULL;
    int connfd,connfd1;
    int bytes_sent = 0;
    int bytes_received = 0;
    char buff[BUFF_SIZE+1];
    char listClient[BUFF_SIZE+1];
    char minus[] = "-----------";
    char enter[] = "\n";
    pthread_t tid;
   	int *connPtr;

    client *temp = (client *)malloc(sizeof(client));

	while (pointer != NULL)
	{

		if (pointer->id != idClientFrom)
		{
			/* code */
		
		connfd = *pointer->connfd;
		bytes_sent = send(connfd, fileName, BUFF_SEND, 0);
		// bytes_sent = send(connfd, fileName, BUFF_SEND, 0);
		bytes_received = recv(connfd, buff, BUFF_SIZE, 0);
		// printf("bytes_received: %d\n", bytes_received);
		buff[bytes_received] = '\0';
		if(strcmp(buff,FILE_EXIST)==0){
			printf("%s\n", FILE_EXIST);
			temp = findByConn(connfd);
			headListAddrHasFile = insertListAddr(*temp,headListAddrHasFile);

		}
		}
		pointer = pointer->next;

    }
    if(headListAddrHasFile == NULL)
    {	
    	bytes_sent = send(currentCon,FILE_NOT_FOUND, BUFF_SEND, 0);
    }
    if (headListAddrHasFile != NULL)
    {
    	client *tmp = headListAddrHasFile;
    	memset(listClient,0,sizeof(listClient));
    	printf("listClient: %s\n", listClient);
    	while (tmp != NULL)
		{
			connfd = *tmp->connfd;
			char connStr[20];
			sprintf(connStr, "%d", connfd);
			strcat(listClient,tmp->addressIP);
			strcat(listClient,"-");
			strcat(listClient,connStr);
			strcat(listClient,"\n");
			printf("%s\n", listClient);
			tmp = tmp->next;
    	}
    	bytes_sent = send(currentCon,listClient, BUFF_SEND, 0);
    	memset(listClient,0,sizeof(listClient));
    	bytes_received = recv(currentCon, buff, BUFF_SIZE, 0);
    	buff[bytes_received] = '\0';
    	printf("Client ID has been choosed: %s\n", buff);
    	connfd = atoi(buff);

    	client *tmp2 = headListAddrHasFile;
    	while (tmp2 != NULL)
		{
			connfd1 = *tmp2->connfd;
			if(connfd1 != connfd && connfd1 != currentCon){
				send(connfd1,FILE_NOT_FOUND, BUFF_SEND, 0);
			}

			tmp2 = tmp2->next;
    	}	

    	temp = findByConn(connfd);
    	printf("Client will get file : %s\n", temp->addressIP);

    	bytes_sent = send(connfd,SEND_FILE_REQUEST, BUFF_SEND, 0);

    	receiveFile(connfd,fileName);
    	sendFile(currentCon,fileName);



    }

    searchFileHandle(currentCon);

 

}

client* findById(int idClient){
   client* current = head;
   if(head == NULL){
      return NULL;
   }
   while(current->id != idClient){

      if(current->next == NULL){
         return NULL;
      }else {
         current = current->next;
      }
   }
   return current;
} 

client* findByConn(int connfd){
	printf("connfd: %d\n", connfd);
   client* current = (client *)malloc(sizeof(client));
   current = head;
   if(head == NULL){
   	printf("%s\n", "NULL");
      return NULL;
   }
   while(current != NULL){
   	if (*current->connfd == connfd)
   	{
   		return current;
   	}
   	current = current->next;

   }
   
   return NULL;
} 

int main(int argc, char const *argv[])
{ 
	if (argc != 2)
    {
        printf("error, too many or too few arguments\n");
        printf("Correct format is /.server YourPort\n");
        return 1;
    }

	char fileName[BUFF_SIZE];
	int listenfd, *connfd;
	struct sockaddr_in server; /* server's address information */
	struct sockaddr_in *client; /* client's address information */
	int sin_size;
	pthread_t tid;
	int idClient = 0;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  /* calls socket() */
		perror("\nError: ");
		return 0;
	}
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));     
    server.sin_addr.s_addr = htonl(INADDR_ANY); 
	// printf("%s\n", argv[1]);
	if(bind(listenfd,(struct sockaddr*)&server, sizeof(server))==-1){ 
		perror("\nError: ");
		return 0;
	}     

	if(listen(listenfd, BACKLOG) == -1){  
		perror("\nError: ");
		return 0;
	}
	
	sin_size=sizeof(struct sockaddr_in);
	client = malloc(sin_size);
	while(1){		
		connfd = malloc(sizeof(int));
		sin_size = sizeof(struct sockaddr_in);
		if ((*connfd = accept(listenfd, (struct sockaddr *)client, &sin_size)) ==- 1)
			perror("\nError: ");
				
		printf("You got a connection from %s\n", inet_ntoa(client->sin_addr) ); /* prints client's IP */
		struct tagAddress tempAddr;
		strcpy(tempAddr.addressIP,inet_ntoa(client->sin_addr));
		// printf(" %d \n", *connfd);
		tempAddr.connfd = malloc(sizeof(int));
		tempAddr.connfd = connfd;
		if (head == NULL)
		{
			tempAddr.id = 0;
		}else{
			tempAddr.id = head->id;
		}
		
        InsertHead(tempAddr);
        printf("%s\n", "List Client :");
        
		/* For each client, spawns a thread, and the thread handles the new client */
		pthread_create(&tid, NULL, &mainFunction, connfd);	
	}
	
	close(listenfd);
	return 0;
}

void searchFileHandle(int connfd){
	char fileName[BUFF_SIZE+1];
	int bytes_sent, bytes_received;
	bytes_received = recv(connfd, fileName, BUFF_SIZE, 0);
	if (bytes_received <= 0){
		client *temp = findByConn(connfd);
		client *temp2 = headShareList;
		while(temp2 != NULL){
			if(temp2->id == temp->id){
				break;
			}
			temp2 = temp2->next;
		}
		printf("Share connfd close : %d\n", *temp2->connfd);
		deleteNode(head,*temp2->connfd);
		deleteNode(head,*temp->connfd);
		deleteNode(headShareList,*temp2->connfd);
		printf("Connection closed.");
		close(connfd);
		close(connfd);
		return;
	}
	fileName[bytes_received] = '\0';
	client* tmp10 = head;
	int id;
	printf("main connf: %d\n", connfd);
	while(tmp10 != NULL){
		id = tmp10->id;
		if(*tmp10->connfd == connfd){
			break;
		}
		tmp10 = tmp10-> next;
	}
	printf("File need to find: %s\n", fileName);
	sendRequestFindFile(connfd,fileName,id);
}

void *mainFunction(void *arg){
	int connfd;
	int bytes_sent, bytes_received;
	char buff[BUFF_SIZE + 1];
	char fileName[BUFF_SIZE+1];
	char chooseMode[BUFF_SIZE];
	int *connPtr;
	char idClient[10];
	connfd = *((int *) arg);
	// free(arg);
	pthread_detach(pthread_self());

	client *ptr;
    ptr = head;

    client *temp = findByConn(connfd);
    sprintf(idClient, "%d", temp->id);

    bytes_sent = send(connfd,idClient, BUFF_SEND, 0);




    printf("NEW %s\n", "===================");
   
	bytes_received = recv(connfd, chooseMode, BUFF_SIZE, 0); //blocking
	if (bytes_received < 0)
		perror("\nError: ");
	else if (bytes_received == 0){
		printf("%s\n", "AAAAAAAAAAAAAAAAAAAAAAA");
		printf("%s\n", "Quit");
		printf("Connection closed.");
	}

	chooseMode[bytes_received] = '\0';

	if(strcmp(chooseMode,SHARE_MODE) ==0 ){
		int idClient;
		bytes_received = recv(connfd, buff, BUFF_SIZE, 0);
		buff[bytes_received] = '\0';
		printf("background connfd: %d idClient : %s\n", connfd,buff);
		idClient = atoi(buff);
		client *ptr1 = headSearchList;

		ptr1 = findById(idClient);

		client *tempClient = (client *)malloc(sizeof(client));
    	strcpy(tempClient->addressIP, ptr1->addressIP);
    	tempClient->connfd = (int *)malloc(sizeof(int));
    	*tempClient->connfd = connfd;
    	tempClient->id = ptr1->id;

		// *ptr1->connfd = connfd;

		headShareList = insertClient(tempClient,headShareList);
		free(tempClient);   

		// insertShareList(connfd);


    	// ptr1 = headShareList;
    	// printf("%s\n", "background list");
    	// while (ptr1 != NULL)
    	// {
    	// 	printf("%s %d ID : %d \n", "Share connfd: ", *ptr1->connfd,ptr1->id);
     //   		ptr1 = ptr1->next;
    	// }


	}
	if(strcmp(chooseMode,SEARCH_MODE) ==0 ){
		printf("%s\n", "SEARCH_MODE");

		searchFileHandle(connfd);


    	

	}
	
	if (bytes_sent < 0){
		perror("\nError: ");
	
	close(connfd);	
	}
}