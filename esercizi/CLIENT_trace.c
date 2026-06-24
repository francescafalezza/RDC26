
/*CONSEGNA: si modifichi il client per supportare il metodo Trace

Si colleghi il Client Web modificato al servizio web esposto dai seguenti indirizzi IP: 
46.37.17.205 (www.radioamatori.it)

definito nell’HTTP/1.1 definito alla sezione 9.8 della RFC 2616 e identifichi se la Request viene 
modificata da proxy trasparenti prima di giungere al server.
Cosa è il metodo Trace: 

Il metodo TRACE viene utilizzato per richiamare un loopback remoto a livello di applicazione del messaggio di richiesta. Il destinatario finale della
richiesta DOVREBBE riflettere il messaggio ricevuto al client come corpo-entità di una risposta 200 (OK). Il destinatario finale è il server di origine o il
primo proxy o gateway a ricevere un valore Max-Forwards pari a zero (0) nella richiesta (vedere sezione 14.31). Una richiesta TRACE NON DEVE
includere un'entità.

TRACE allows the client to see what is being received at the other
end of the request chain and use that data for testing or diagnostic
information. The value of the Via header field (section 14.45) is of
particular interest, since it acts as a trace of the request chain.
Use of the Max-Forwards header field allows the client to limit the
length of the request chain, which is useful for testing a chain of
proxies forwarding messages in an infinite loop.

If the request is valid, the response SHOULD contain the entire
request message in the entity-body, with a Content-Type of
"message/http". Responses to this method MUST NOT be cached.

   AGGIUNTA: controllare anche header Max-Forwards. 
   Max-Forwards è un campo header della request che limita il numero di proxy che possono inoltrare la richiesta
   Max-Forwards   = "Max-Forwards" ":" 1*DIGIT


IDEA: 
Il CLIENT invia una request con TRACE al server e header Max-Forwards con valore 10. 
il SERVER risponde con 200 OK e include nel body tutta la request ricevuta e il campo Max-forwards aggiornato a 9
Il client controlla che l'entity body della response contenga tutta la request inviata e che il campo Max-Forwards sia stato decrementato.

DUBBIO: controllo anche la status line della risposta?
sprintf(request, "TRACE HTTP/1.1\r\nHost:%s\r\nMax-Forwards: 10\r\n\r\n", hostname)
*/


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>  // atoi
#include <sys/socket.h> // socket, struct sockaddr
#include <sys/types.h>  // ""
#include <arpa/inet.h>  // socket, struct sockaddr_in
#include <stdint.h>
#include <unistd.h>  // write
#include <string.h>  //strlen

struct sockaddr_in remote;
char hostname[100];
char response[10000001]; // messa in mem statica => init a 0, char terminatore di default

struct header {
   char * n;
   char * v;
} h[100];

struct header {
   char * n;
   char * v;
} hr[100];


int main() {
   int k,n;
   char request[1000]; 

    int contentType=0; //per controllare che la risposta abbia contentee type http

   char * statusline;
   char hbuffer[10000];
   //unsigned char ipserver[4] = { 142, 250, 180, 3 };
   int s;

   if (-1 == (s = socket(AF_INET, SOCK_STREAM, 0))) {
      printf("errno = %d\n", errno);
      perror("An error occured on creation of socket");
      return -1;
   }

   //risoluzione dell'hostname in indirizzo ip
   sprintf(hostname, "www.radioamatori.it");

   struct hostent* remoteIP; 
   printf("Risoluzione di hostname: %s\n", hostname); 
   remoteIP=gethostname(hostname); 

   remote.sin_family = AF_INET;  // sa_family_t address family
   remote.sin_port = htons(80);  // in_port_t port in network byte order (16 bit)
   remote.sin_addr.s_addr = *(unsigned int*)(remoteIP -> h_addr_list[0]); // struct_in_addr internet address { uint32_t s_addr // address in network byte order}

   if (-1 == connect(s, (struct sockaddr *)&remote, sizeof(struct sockaddr_in))) {
      perror("Connessione socket fallita");
      return -1;
   }

   //PARTE NUOVA: TRACE request 
   sprintf(request, "TRACE / HTTP/1.1\r\nHost:%s\r\nMax-Forwards:10\r\n\r\n",hostname);

   //invio della request e parsing degli header della risposta. 
   int j = 0;  // scorre elementi in header
   statusline = h[0].n = hbuffer;
   int i = 0;  // scorre caratteri
   int chunked = 0;
   size_t len = 0;

   for(k=0; k < 1; k++){
      write(s, request, strlen(request));
      bzero(hbuffer,10000);   // azzera bytes
      statusline = h[0].n = hbuffer;
      int bodylen = 1000000;

      //Parsing degli header della risposta
      for (i=0, j=0; read(s, hbuffer + i, 1); i++) {
         if (hbuffer[i] == '\n' && hbuffer[i - 1] == '\r') {
            hbuffer[i-1] = 0;      // Termino il token attuale
            if (! h[j].n[0]) break;
            h[++j].n = hbuffer + i + 1;
         }
         if (hbuffer[i] == ':' && !h[j].v) {
            hbuffer[i] = 0;
            h[j].v = hbuffer + i + 1;
         }
      }
      

      for(i=1; i<j; i++){
         printf("name %s value %s\n", h[i].n, h[i].v);
         // if chunked transfer encoding
         if(!strcmp("Transfer-Encoding", h[i].n) && !strcmp(" chunked", h[i].v)) {
            chunked = 1; // chunked = true
         // if Content-Length
         } else if(!strcmp("Content-Length", h[i].n)) {
            bodylen = atoi(h[i].v);
            chunked = 0;
         }
         if((!strcmp("Content-Type", h[i].n)) && !strcmp(" message/http", h[i].v)){
            contentType=1; 
        }

         
        }
      }
      
      char chunk_size_hex[100000];
      size_t chunk_size = 0;
      size_t cc = 0;


      // read (int fd, void* buf, size_t cont)
      if (chunked > 0) {
         // infinite loop to read chunks
         for (len = 0;;len += cc) {
            // read chunk_size until it meet CR LF
            for (i = 0; 0 < (n = read(s, chunk_size_hex + i, 1)); ++i) {
               if (chunk_size_hex[i] == '\n' && chunk_size_hex[i-1] == '\r') {
                  chunk_size_hex[i-1] = 0;
                  break;
               }
            }
            // convert hex string to size_t
            chunk_size = (size_t)strtol(chunk_size_hex, NULL, 16);
            printf("chunk_size %zu\n", chunk_size);
            // last chunk has chunk_size == 0
            if (chunk_size == 0)
               break;
            // read chunk data
            for (cc = 0; cc < chunk_size && 0 < (n = read(s, response + len + cc, chunk_size - cc)); cc += n);
            if (n < 0) {
               perror("Read fallita");
               return -1;
            }
            // read CR
            read(s, chunk_size_hex, 1);
            // read LF
            read(s, chunk_size_hex + 1, 1);
         }


      } else {
         for (len = 0; len<bodylen && (n = read(s, response + len, 10000000 - len)) > 0; len += n);
         if (n < 0) {
            perror("Read fallita");
            return -1;
         }
      }

      //PARTE NUOVA: controllo che tutti gli header nel body corrispondano a quelli della richiiesta
      //L'header Max-Forwards è un request header, quindi lo controllo nell'entity body e deve essere decrementato. 
      char *response_line; 
      hr[0].n = response=response_line; 

      for (i=0, j=0; response[i] != '\0'; i++) {
         if (response[i] == '\n' && response[i - 1] == '\r') {
            response[i-1] = 0;      // Termino il token attuale
            if (! hr[j].n[0]) break;
            hr[++j].n = response + i + 1;
         }

         if (response[i] == ':' && !hr[j].v) {
            response[i] = 0;
            hr[j].v = response + i + 1;
         }
      }

      //controlo gli header del body della response e verifico che corrispondano a quelli della request
      //La prima riga del body è la request line, quindi parto da i=1 per controllare gli header
      for (i=1; i<j; i++){
        printf("name %s value %s \n", hr[i].n, hr[i].v);
        if (strcmp(hr[i].n, "Max-Forwards")==0){
            if (strcmp(" 9", hr[i].v)==0){
                printf("Max-Forwards decrementato correttamente a 9\n");
            }
            else{
                printf("Max-Forwards non decrementato corretamente, valore attuale: %s\n", hr[i].v); 
            }
        }

        if (strcmp(hr[i].n, "Host")==0){
            if (strcmp(hostname, hr[i].v)==0){
                printf("Host header correttamente presente nella request riflessa\n");
            }
            else{
                printf("Host header non presente nella request riflessa\n");
            }
        }
            

      response[len] = 0;
      FILE *file= fopen("index.html", "w");
      int results = fputs(response, file);
      if (results == EOF)
         perror("Failed to write to index.html");
      fclose(file);
      printf("Response written in file index.html");
   }
   return 0;
}

/*
ESERCIZIO: quanti proxy trasparenti ci sono stati tra client e server?
`Via` è un header aggiunto dai **proxy** (non dal client, non dal server) per "firmarsi" 
lungo il percorso.

**Formato:**
```
Via: 1.1 nomeproxy.example.com (commento opzionale)
       ↑           ↑
   versione HTTP   identificativo del proxy
```

**Come si accumula nella catena:**
```
Client manda:
TRACE / HTTP/1.1
Host: www.example.com
                        ← nessun Via

Proxy1 riceve e aggiunge il suo Via, poi manda avanti:
TRACE / HTTP/1.1
Host: www.example.com
Via: 1.1 proxy1.tim.it

Proxy2 riceve e APPENDE il suo Via:
TRACE / HTTP/1.1
Host: www.example.com
Via: 1.1 proxy1.tim.it, 1.1 proxy2.fastweb.it

Server riceve e riflette tutto nel body
```

Quindi dal body della response TRACE puoi ricostruire **quanti proxy** 
ci sono stati e in che ordine, leggendo il campo `Via` da sinistra a destra.

Nella pratica però molti proxy oggi sono **trasparenti 
e non aggiungono Via** per ragioni di privacy/sicurezza, 
quindi potresti non trovarlo anche se un proxy c'è stato — 
in quel caso l'unica evidenza è il `Max-Forwards` decrementato.*/

