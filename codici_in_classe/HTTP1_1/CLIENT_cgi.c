#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<string.h>


int main(){


	int sockId = socket(AF_INET, SOCK_STREAM, 0);
	
	int opt = 1;
	setsockopt(sockId, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//opzione per riutilizzare la porta

	struct sockaddr_in serverAddr;//struttura contenente IP server

	serverAddr.sin_family = AF_INET; //IPv4
	serverAddr.sin_port = htons(8085);
	char *p = (char *)&serverAddr.sin_addr.s_addr;
	p[0] = 0; p[1] = 0; p[2] = 0; p[3] = 0; // accettiamo connessioni da tutti

	int b = bind(sockId, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	
	if(b == -1){
		perror("Bind Error");
		exit(EXIT_FAILURE);
	}

	int l = listen(sockId, 5);

	if( l == -1){
		perror("listen error");
		exit(EXIT_FAILURE);
	}

	while(1){
	struct sockaddr_in client_addr;
	int sizeSockAddrClient = sizeof(client_addr);
	int clientSockId = accept(sockId, (struct sockaddr *)&client_addr, &sizeSockAddrClient);

	if(clientSockId == -1){
		perror("Accept error");
		exit(EXIT_FAILURE);
	}

	printf("accept effettuata\n");
	unsigned char *p2 = (unsigned char *)&client_addr.sin_addr.s_addr;
	unsigned short int clientPort = ntohs(client_addr.sin_port);
	printf("%d.%d.%d.%d:%d\n", p2[0],p2[1],p2[2],p2[3], clientPort);
	fflush(stdout);

	int pid = fork();
	if(pid == 0){
	close(sockId);
	char buffer[10000];

	struct header {
		char *n;
		char *v;
	};

	struct header h[100];

	int header_index = 0;
	int leggoHeaderName = 0;
	h[header_index].n = buffer;

	int byteLetti = 0, n = 0;
	while((n = read(clientSockId, buffer + byteLetti, 1)) > 0){
		printf("%c\n",buffer[byteLetti]);
		if(byteLetti >= 2 && buffer[byteLetti] == '\n' && buffer[byteLetti - 1] == '\r'){
			if(buffer[byteLetti - 2] == '\n' && buffer[byteLetti -3] == 0){
				break;
			}
			h[++header_index].n = buffer + byteLetti + 1;
			buffer[byteLetti - 1] = 0;
			leggoHeaderName = 1;
		}

		if(leggoHeaderName && buffer[byteLetti] == ':'){
			buffer[byteLetti] = 0;
			h[header_index].v = buffer + byteLetti + 1;
			leggoHeaderName = 0;
		}

		
		byteLetti += n;
		
	}

	char *request_line = buffer;
	printf("request line:%s\n", request_line);
	for(int i = 1; i < header_index; i++){
		printf("%s: %s\n", h[i].n, h[i].v);
	}

	char method[10], uri[100], http_version[10];
	sscanf(request_line, "%s %s %s", method, uri, http_version);

	printf("metodo:%s\n uri:%s\n version:%s\n", method, uri, http_version);

	if(strcmp("GET", method) == 0){
		if(strncmp("/cgi-bin/", uri, 9) == 0){
			int p = fork();
			if(p == 0){
				dup2(clientSockId, 1); //stdout
				dup2(clientSockId, 0); //stdin
				printf("HTTP/1.1 200 OK\r\n");
				setenv("REQUEST_METHOD", "GET", 1);
				char *arg[] = {uri+1, NULL};
				execv(uri + 1, arg);
			} else {
				waitpid(p);
			}

		} else {

		FILE *f = fopen(uri + 1, "r");
		if(f == NULL){
			char response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
			write(clientSockId, response, strlen(response));
		} else {

			int bodySize = 0;
			char c;
			int maxBodySize = 1000000;
			unsigned char *body = malloc(maxBodySize);
			while(EOF != (c = fgetc(f))){
				body[bodySize++] = c;
			}


		char response[1000];
	        sprintf(response,"HTTP/1.1 200 OK\r\nConnection:close\r\nContent-Length:%d\r\n\r\n", bodySize);
		write(clientSockId, response, strlen(response));
		write(clientSockId, body, bodySize);
		}
		}
	} else {
		char response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
		write(clientSockId, response, strlen(response));
	}
	close(clientSockId);
	exit(EXIT_SUCCESS);
	} else {
		close(clientSockId);
	}
	}
	
	close(sockId);

}
