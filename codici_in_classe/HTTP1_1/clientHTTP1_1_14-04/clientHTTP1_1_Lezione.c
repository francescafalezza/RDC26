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
    ip[0] = 104, ip[1] = 18, ip[2] = 27, ip[3] = 120;

    int c = connect(sockfd,(struct sockaddr*) &address, sizeof(address));

    if(c == 0){
        printf("connessione stabilita\n");
    } else {
        perror("connessione fallita");
        exit(1);
    }

    char buffer[] = "GET / HTTP/1.1\r\nHost:www.example.com\r\nConnection:keep-alive\r\n\r\n";

    int byteScritti = 0;
    int m =0;
    while(byteScritti < strlen(buffer)){
       m = write(sockfd, buffer + byteScritti, strlen(buffer)- byteScritti);
       byteScritti += m;
    }

    char response[1000000];

    int n =0;
    int byteLetti=0;
    while( (n = read(sockfd, response + byteLetti, 1)) > 0){
        printf("%s", response);
       printf("numero caratteri letti:%d\n", n);
       
        int byteBody = 0;
         int headerIndex = 0;
        int lettoNomeHeader = 0;
        for(int i = 0; i < byteLetti; i++){
            if(response[i] == '\n' && response[i -1] == '\r'){
             if(response[i - 3] == 0){
                   body = response + i + 1;
                  byteBody = byteLetti - i;
                 break;
             }
                lettoNomeHeader = 0;
                response[i - 1] = 0;
                h[headerIndex].n = response + i + 1 ;
            } else if(!lettoNomeHeader && response[i] == ':'){
                lettoNomeHeader = 1;
                response[i] = 0;
                h[headerIndex++].v = response + i + 1;
            }
        }
        byteLetti += n;
        //printf("numero caratteri letti finora:%d\n", byteLetti);
    }


    //mentre leggiamo dobbiamo porci il problema dei cuncked
    
    
}