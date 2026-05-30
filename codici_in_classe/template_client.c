#include <stdio.h>
#include <sys/socket.h>             // socket
#include <errno.h>                  // errno
#include <arpa/inet.h>              // htons
#include <unistd.h>                 // write
#include <string.h>                 // strlen, strcmp
#include <stdlib.h>                 // atoi


/*Programma che gestica il meccaniscmo di controllo della caching

La prima volta la cache è vuota, deve aquisire la data di ultima modifica della risorsa e la salverà insime eall'entity body
La seconda volta riscaricherà la risorsa solo se è stata modificata sul server, altrimenti accede alla copia in cache

Pseudocodice:
-tenti di apire il file cache
se il file esiste allora
        il client invia richiesta HEAD al server per ottenere gli header della risposta
        e controlla se la risorsa è stata modificata controllando la data ultima modifica della response e 
        la data sul file cache
        se la risorsa non è stata modificata allora si apre il file cache 
        altrimenti si chiede al server la risorsa aggiornata


se la risorsa non è presente in cache 
    si fa una richiesta GET e  il download della risorsa e si salva in cache 
*/


char hbuf[10000];

struct headers{
    char * n; 
    char * v; 
} h[100];     



int main(){

    // local varibles
    struct sockaddr_in server_addr;     // server address
    int s;                              // socket
    int t;                              // temporary
    unsigned char * p;                  // ip address piointer
    int i, j;
    char * statusline;

    char request[1000];
    char response[1024*1024];
    // create socket
    s = socket( AF_INET, SOCK_STREAM, 0 );
    // printf("Socket: %d\n", s);


    //il file da salvare in cache ha nome cacheName
    char cacheName[1000];
    strcpy(cachename, "./cache/_");


    if( s == -1){
        printf("ERRNO = %d (%d)\n", errno, EAFNOSUPPORT);
        perror("Socket fallita\n");
        return 1;
    }


    /* Setup for request */

    // set server addr    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    
    // IPv4 server
    p = (unsigned char *) &server_addr.sin_addr.s_addr;
    p[0] = 142;     p[1] = 250;     p[2] = 187;      p[3] = 196;


    // connect server
    if(-1 == connect(s, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in))){ 
        perror("Connessione fallita\n");
        return 1;
    }


    //PROVA AD APRIRE FILE CACHE 
    FILE* cached;

    char* format= "%a, %d %b %Y %T GMT";

    //se il file esiste
    if ((cached = fopen(cacheName, "r")) != NULL){

        //leggo la prima riga del file cache che contiene la data di download
        char cacheDate[100];
        fread(cacheDate, sizeof(char), 10, cached); 

        //convertire la data di download--> per fare il confronto con la data dell'header
        //la DATA DI DOWNLOAD E' IN EPOCH (10 caratteri) quindi va convertita in struct tm  e poi in fromato HTTP

        time_t epochCache= atoi(cacheDate); //atoi converte una stringa che rappresenta un numero intero in un Integer

        struct tm timeCache = *gmtime(&epochCache); //converte epoch in truct tm

        char httpDate[100]; 
        strftime(httpDate, sizeof(httpDate), format, &timeCache); //struct tm --> http date


        //il client invia una request HEAD al server, per ottenere tutti gli header. Tra questi cercherà Last-Modified
        sprintf(request, "HEAD %s HTTP/1.1\r\nHost:%s\r\nIf-Modified-Since:%s\r\n\r\n", resourceName, hostName, httpDate);
        
        //system call write, scrive sul socket la request
        write(s, request, strlen(request)); 

        //PARSING DEGLI HEADER: leggo la risposta alla richiesta HEAD e cerco l'header Last-Modified, di cui prenderò la data 
        int headersCount=0; 
        headers[0].n =hbuff; 
        for (int i =0; read(s, hbuff+i, 1); i++){

            //fine riga header seguito da riga vuota 
            if (hbuff[i] =='\n' && hbuff[i-1]=='\r'){
                hbuff[i-1] =0; //terminatore di riga dell'header

                if (headers[headersCount].n[0] ==0) 
                    break; //se la riga è vuota, abbiamo finito di leggere gli header

                headers[++headersCount].n= hbuff+i+1; //il nome del prossimo header inizia dopo il terminatore di riga
            }

            //se troviamo : e non abbiamo assegnato un valore allora settiamo il puntatore del valore dell header 
            if (hbuff[i]== ':' && !headers[headersCount].v){
                hbuff[i] =0; 
                headers[headersCount].v = hbuff+i+2; //DOPO I : C'E' SEMPRE UNO SPAZIO
            }

        }

        //cerchiamo l'header Last-Modified e prediamo la data di ultima modifica
        char* lastModified=0; 
        int notModified=0; 

        if (strcmp("304 Not Modified", headers[0].n+9)==0){ //se lo status code è 304 
            printf("Not modified, serve from cache\n");
            notModified =1; //QUA POTREI GIA' scaricare la risorsa da cache, ma faccio il confronto delle date perchè se sto usando HTTP1.0 non ho la possibilità di usare l'header If-Modified- Since
        }

        else{
            printf("Resource modified, need to download\n");
        }

        //cerco l'header Last-Modified e prendo la data dopo i : 
        for (int i =0; i<headersCount; i++){
            if (strcmp("Last-Modified", headers[i].n) ==0){
                lastModified = headers[i].v; //lastModiefied e header value puntano allo stesos byte di memoria 

            }

        //convertire la data  che è in formato HTTP date in epoch
        struct tm* httpTime = malloc(sizeof(struct tm));
        strptime(lastModified, format, httpTime); //string--> tm
        time_t epochRemote =mktime(httpTime); //strct tm--> epoch

        //confronto i valori di epochRemote e epochCache

        //se epochRemote > epochCache la risorsa è stata modificata e il client fa richiesta GET per avere la risorsa aggiornata
        //se epochRemote <= epochCache la risorsa non è stata modificata e il client può accedere alla risorsa in cache 
        if (notModified || epochRemote<= epochCache){
            printf("\n Serve from cache\n");
            int c =0; 

            char* fileBuff[1024*5]; 

            //cilco for per leggere dal file cache a blocchi di 5kB
            for (int l =0; (c= fread(fileBuff, sizeof(char), 1024*5, cached)>0; l +=c))

            //mostra all'utente il contenuto del file
            printf("%s\n", fileBuff); 
            fclose(cached); 

            //TERMINA IL PROGRAMMA
            return 0; 

             

        }

        }


    }

    //se il file non esiste oppure è scaduto faccio richiesta GET al server
    // send request
    sprintf(request, "GET %s HTTP/1.1\r\nHost:%s\r\n\r\n", resouceName, hostName); 

    write(s, request, strlen(request));



    statusline = h[0].n = hbuf;
    j = 0;

    // reade header
    for( i = 0; read(s, hbuf + i, 1); i++ ){

        // end of line
        if( hbuf[i - 1] == '\r' && hbuf[i] == '\n'){
            
            hbuf[i - 1] = 0;
           
            if( !( h[j].n[0] ) )
                break;
            
            h[++j].n = &hbuf[i + 1];
        }

        // end of name
        if( (hbuf[i] == ':') && (h[j].v == NULL) ){

            h[j].v = &hbuf[i + 1];
            hbuf[i] = 0;
        }
    }

    // print headers
    for(i = 0; i < j; i++)
        printf("%s —————> %s\n", h[i].n, h[i].v);
    printf("\n\n");



    //  get content length
    int content_length;

    for(i = 0; i < j; i++)
        if( !strcmp( h[i].n , "Content-Length" ))
            content_length = atoi(h[i].v);
    

    // get entity body
    char response[2000000];           
    
    if ( !content_length ){

        // read the response
        for ( i = 0; t = read(s, response + i, content_length - i); i += t ) {}

        // null-terminate
        response[i] = 0;
        printf("%s\n\n", response);

        // exit
        return 0;
   
    }
        

    long chunk_size;
    char chunk_buffer[8];

    // will contain all the read bytes
    j = 0;

    do {    // when chunk_size == 0, exit

        // consume and convert in dec the chunk size
        for(chunk_size = 0, i = 0;
            read(s, chunk_buffer + i, 1)  &&  !(chunk_buffer[i - 1] == '\r' && chunk_buffer[i] == '\n');
            i++) {
            
            // lower case conversion
            if( chunk_buffer[i] >= 'A' && chunk_buffer[i] <= 'F')
                chunk_buffer[i] = chunk_buffer[i] - ('a' - 'A');

            
            // letter to number conversion
            if( chunk_buffer[i] >= 'a' && chunk_buffer[i] <= 'f')
                chunk_size = chunk_size * 16 + chunk_buffer[i] - 'a' + 10;
            
            // number conversion
            if( chunk_buffer[i] >= '0' && chunk_buffer[i] <= '9')
                chunk_size = chunk_size * 16 + chunk_buffer[i] - '0';
        
        }

        // read chunk 
        for( i = 0; t = read(s, response + j, chunk_size - i); i += t, j += t);

        // read CRLF
        read(s, chunk_buffer, 2);

    } while( chunk_size );


    // null-terminate response
    response[j] = 0;

    cached = fopen(cacheName, "w+");

    time_t epochNow=time(NULL); 

    fprintf(cached, "%u\n", (unsigned long)epochNow);

    //salviamo entity body nel file cache
    fwrite(response, sizeof(response[0], strlen(response), cached)); 
    printf("%s\n\n", response);

    fclose(cached); 
    return 0;

} // main