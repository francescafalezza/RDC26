#include<stdio.h>
#include<string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int inviaByte(int fd, int numeroByte, char *buffer){
    
     int byteScritti = 0;
     int m =0;
     while(byteScritti < numeroByte){
         m = write(fd, buffer + byteScritti, numeroByte- byteScritti);
         byteScritti += m;
      }
      

}

int main(){
    struct header {
          char *n;
          char *v;
      };

      struct header h[100];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(8081);
    address.sin_addr.s_addr = INADDR_ANY;

    int s = bind(sockfd,(struct sockaddr *) &address, sizeof(address));

    if(s != 0){
        perror("bind fallita");
        exit(1);
     } else {
        printf("bind avvento con successo");
        fflush(stdout);
    }

    int l = listen(sockfd, 5);


    printf("il server è pronto per l'accept");
    fflush(stdout);
    int clientSockId = accept(sockfd, NULL, NULL);


    char buffer[10000];

    printf("mi preparo a leggere\n");
    fflush(stdout);
    int n = 0;
    int lettoNomeHeader = 0;
    int byteLetti = 0;
    int headerIndex = 0;
    while((n += read(clientSockId, buffer + n, 1))> 0 ){
        printf("%c", buffer[n - 1]);
        if(buffer[n -1] == '\n' && buffer[n - 2] == '\r'){
              if(buffer[n - 4] == 0){
                  //body = buffer + byteLetti + 1;
                  break;
              }
              lettoNomeHeader = 0;
              buffer[byteLetti - 1] = 0;
              h[headerIndex].n = buffer + byteLetti + 1 ;
          } else if(!lettoNomeHeader && buffer[byteLetti] == ':'){
              lettoNomeHeader = 1;
              buffer[byteLetti] = 0;
              h[headerIndex++].v = buffer + byteLetti + 1;
          }

    }

    for(i=0; i<headerIndex;i++){
        printf("%s:%s\n", h[i].n, h[i].v);
    }

    char method[10], uri[100], version[10];
    char *requestLine= buffer;
    printf("Request line:%s", requestLine);

    sscanf(requestLine, "%s %s %s", method, uri, version)

    printf("method:%s\n", method);
    printf("uri:%s\n", uri);
    printf("version:%s\n", version);



    char response[1000] = "HTTP/1.1 200 OK\r\n\r\n<html>";
    if(strcmp(method, "GET"==0)){
        //se arriva la get non fa nulla, se non arriva la repsonce non arriva e non sappiamo quanto grande sia per questo abbiamo messo 1000
        spintf(uri, "/index.html");
        int fd= open(uri+1, O_RDONLY);
        if (fd<0){
            spintf(response, "HTTP/1.1 404 Not Found\r\n\r\n") //possiamo dirgli come visualizzare il codice di errore: "HTTP/1.1 404 Not Found\r\n\r\n<html> PAgina non trovata <html>"
        }
        else{
            //dobbiamo inviare la response: il file è il body, cioè dopo statusLine e header.
            //prima mandiamo write con status line e header. Dopo facciamo la seconda e leggiamo il file
            inviaByte(clientSockId, strln(response), response);
            int m =0;
            char bufferFile[1024];
            while((m=read(fd, bufferFile, sizeof(bufferFile)))>0){
                inviaByte(clientSockId, buffer, m);
            }
        }
    
         
    }else {spintf(response, "HTTP/1.1 405 Method not allowed OK\r\n\r\n");
    inviaByte(clientSockId, strln(response), response);
    }

    close(clientSockId);
    close(sockfd);
}
