#include<stdio.h>
#include<string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(){
    struct header {
        char *n;
        char *v;
    };

    struct header h[100];
    char *statusLine;
    char *body;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;

    address.sin_family = AF_INET;
    address.sin_port = htons(80); //big endian di 8080 (network byte order)

    char* ip = (char *)&address.sin_addr;
    //ip[0] = 217, ip[1] = 61, ip[2] = 15, ip[3] = 222;
    //ip[0] = 172, ip[1] = 66, ip[2] = 147, ip[3] = 243;
    ip[0] = 142, ip[1] = 251, ip[2] = 29, ip[3] = 101;
    int c = connect(sockfd,(struct sockaddr*) &address, sizeof(address));

    if(c == 0){
        printf("connessione stabilita\n");
    } else {
        perror("connessione fallita");
        exit(1);
    }

    char buffer[] = "GET / HTTP/1.1\r\nHost:www.google.com\r\nConnection:keep-alive\r\n\r\n";

    int byteScritti = 0;
    int m =0;
    while(byteScritti < strlen(buffer)){
       m = write(sockfd, buffer + byteScritti, strlen(buffer)- byteScritti);
       byteScritti += m;
    }

    printf("ho scritto %d bytes\n", byteScritti);

    char response[1000000];

    int n = 0;
    int byteBody = 0;
    int headerIndex = 0;
    int lettoNomeHeader = 0;
    int byteLetti = 0;
    while( (n = read(sockfd, response + byteLetti, 1)) > 0){
          if(response[byteLetti] == '\n' && response[byteLetti -1] == '\r'){
              if(response[byteLetti - 3] == 0){
                  body = response + byteLetti + 1;
                  break;
              }
              lettoNomeHeader = 0;
              response[byteLetti - 1] = 0;
              h[headerIndex].n = response + byteLetti + 1 ;
          } else if(!lettoNomeHeader && response[byteLetti] == ':'){
              lettoNomeHeader = 1;
              response[byteLetti] = 0;
              h[headerIndex++].v = response + byteLetti + 1;
          }

        byteLetti += n;
    }

       printf("ho letto tuti gli header");
        statusLine = response;



    int statusCode;
    char httpVersion[10];
    char statusPhrase[20];
    sscanf(statusLine, "%s %d %s", httpVersion, &statusCode, statusPhrase);
    printf("lo status code = %d\n", statusCode);

    printf("numero di header letti = %d\n", headerIndex - 1);

    int transfer_enconding = 0;
    for(int i = 0; i < headerIndex ; i++){
        printf("header %s = %s\n", h[i].n, h[i].v);
        if(strcmp("Content-Length", h[i].n) == 0){
            sscanf(h[i].v, "%d", &byteBody);
        }

        if(strcmp("Transfer-Encoding", h[i].n) == 0 && strcmp(h[i].v, " chunked") == 0){
            transfer_enconding = 1;
            printf("transfer encoding: chunked\n\n\n");
        }
    }
    char *chunkValue = body;
    int chunkSize = 0;
    int lettoChunked = 0;
    printf("il numero di caratteri del body = %d\n",byteBody);
    //printf("valore del body:\n%s", body);
    if(transfer_enconding){
        n =0;
         while( (n += read(sockfd, body + n, 1)) > 0){
            if(body[n - 1] == '\n' && body[n -2] == '\r'){
                body[n - 2] = 0;
                if(lettoChunked){
                    lettoChunked = 0;

                    n -= 2;
                    chunkValue =body + n;


                } else {

                    sscanf(chunkValue, "%x", &chunkSize);
                    printf("il valore del chunked = %d = 0x%x\n", chunkSize, chunkSize);
                    if(chunkSize == 0){

                        break;
                    }
                    fflush(stdout);
                    n -= (strlen(chunkValue) + 2);
                    lettoChunked = 1;
                    byteBody += chunkSize;
                    int m = 0;
                    while( (m += read(sockfd, body + n + m, 1)) > 0 && m < chunkSize){}

                     n += m;
                }
            }
         }
    } else {

        n = 0;
        while( (n += read(sockfd, body + n, 1)) > 0 && n < byteBody){}
    }

    char trailer[1000];
    n = 0;
    while( (n += read(sockfd, trailer + n, 1)) > 0){
             if(trailer[n - 1] == '\n' && trailer[n -2] == '\r'){
                 break;
        }
    }
    printf("trailer size: %d\n", n);

    close(sockfd);
    printf("numero di byte letti = %d\n", n);
    printf("la responseLine=%s", statusLine);

    int f = open("response.html", O_CREAT | O_WRONLY);

    write(f, body, byteBody);
    //printf("il server dice:%s\n", response);
}