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

    printf("ho scritto %d bytes\n", byteScritti);

    char response[1000000];

    int n = 0;
    int byteLetti = 0;
    read(sockfd, response + byteLetti, sizeof(response)- byteLetti);
    //while( (n = read(sockfd, response + byteLetti, sizeof(response)- byteLetti)) > 0){
    //    printf("%s", response);
    //    printf("numero caratteri letti:%d\n", n);
    //    byteLetti += n;
        //printf("numero caratteri letti finora:%d\n", byteLetti);
    //}

    statusLine = response;

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

    int statusCode;
    char httpVersion[10];
    char statusPhrase[20];
    sscanf(statusLine, "%s %d %s", httpVersion, &statusCode, statusPhrase);
    printf("lo status code = %d\n", statusCode);

    printf("numero di header letti = %d\n", headerIndex);

    for(int i = 0; i < headerIndex -1 ; i++){
        printf("header %s = %s\n", h[i].n, h[i].v);
    }

    printf("il numero di caratteri del body%d\n",byteBody);
    //printf("valore del body:\n%s", body);

    printf("la responseLine=%s", statusLine);

    int f = open("response.html", O_CREAT | O_WRONLY);

    write(f, body, byteBody);
    //printf("il server dice:%s\n", response);
}