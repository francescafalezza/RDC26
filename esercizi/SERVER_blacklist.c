/*Si modifichi il programma Web Server in modo che si comporti come segue.
Se l'utente di un client richiede una risorsa provenendo da una pagina configurata come
“cattiva” (blacklisted), il Web Server non permette l’accesso alla risorsa richiesta ma ridireziona
il client alla pagina di provenienza.
Creare prima il meccanismo e, solo successivamente, gestire un file degli URL in black list.
E' il client che proviene da una pagina blacklisted. 

Si faccia riferimento alla ​RFC 1945
blacklist.txt
http://127.0.0.1:17999/blacklist.html
http://88.80.187.84:17999/blacklist.html

PSEUDOCODICE:
il server riceve la richiesta del client. 
controlla che la risorsa non sia dentro la blacklist
- se è dentro la blacklist il server risponde con status line 307 Temporany Redirect, ma dove reindirizza il client???
    il client include un Header Location: <url>  
- non è dentro la blacklist il server ripsonde con 200 OK e serve la risorsa al client 


Come avviene il controllo ? 
prendo il file con dentro le pagine configurate come blacklist e salvo gli indirizzi
confronto poi la risorsa del client con quelli salvati, se c'è una corrispondenza allora deve avvenire il redirect.

apro il file blacklist.txt 
leggo e salvo un buffer
uso sscanf per estrarre gli indirizzi 

salvo la pagina richiesta dal client (quindi l'hostname)
*/




#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>


// Strutura dati dell'header
struct header{
char * n;
char * v;
}h[100];


struct blackPage{
    char *page; 
    int n; 
}; //PUNTO VIRGOLA FINALE!!!

int tmp;

int main() {

/*--------------PARTE NUOVA: estraggo gli indirizzi della blacklist-----------*/
FILE *file = fopen ("blacklist.txt", "r"); 

if (file==NULL){
    printf("Errore nell'apertura del file blacklist.txt"); 
}

char pageBuffer[1000]; //buffer per salvare gli indirizzi della blacklist
struct blackPage blackpages[2]; 

 fread(pageBuffer, sizeof(char), 1000, file); //salvo gli indirizzi della blacklist nel buffer
int temp =0; 

//parsing del buffer per estrarre gli indirizzi e salvarli 
for (int i=0; pageBuffer[i] !=0; i++){
    if(pageBuffer[i] == '\\' && pageBuffer[i-1]=='\\'){
        pageBuffer[i]=0; 
        blackpages[temp].page = pageBuffer +1+i;
        blackpages[temp].n = temp; 
        temp++;  //salvo indirizzo della blacklist; 
        }

    else if (pageBuffer[i]== '\\' && pageBuffer[i-1] != '\\'){
        pageBuffer[i] =0; 
    }
    }
//-------------FINE PARTE NUOVA-------------------

  struct sockaddr_in addr,remote_addr;

  int i,j,k,s,t,s2,c;
  socklen_t len;
  char command[100];
  FILE * fin;
  int yes = 1;

  char * commandline;
  char * method, *path, *ver;
  // sono i 2 buffer per la richiesta e la risposta
  char request[5000],response[10000];
  char location[100];

  // chiamata a sistema che apre una comunicazione e restituisce un INT,
  // che è un File Descriptor ovvero l’indice della tabella con tutto ciò
  // che serve per gestire la comunicazione
  // int socket(int domain, int type, int protocol);
  s =  socket(AF_INET, SOCK_STREAM, 0);

  if ( s == -1 ){

    // Printa testo di errore
    perror("Socket fallita");
    return 1;

  }
  // Indirizzo ipv4 del server che attende la richiesta
  addr.sin_family = AF_INET;
  // Port del server
  // host to network short (htons)
  addr.sin_port = htons(12101);
  // Puntatore all'array contenente l'indirizzo ip del server
  // Essendo io il serve è zero
  addr.sin_addr.s_addr = 0;

  // Indico al sistema operativo che in caso di riavvi ecc
  // Voglio poter riutilizzare la stessa porta usata in precedenza
  t = setsockopt(s, SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int));

  if (t == -1) {

    // Printa testo di errore
    perror("setsockopt fallita");
    return 1;

  }

  // Connette il socket indicando il tipo (AF_INET), la porta da usare e in questo caso
  // Indico che sono un serve con addr.sin_addr.s_addr = 0
  if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {

    // Printa testo di errore
    perror("bind fallita");
    return 1;
  }
  // Dico al socket che avrà un backlog max di 225 cliente
  // Ovvero max 225 tentativi diversi di accedere alla risorsa mentre sto soddisfando la prima richiesta
  if (listen(s, 225) == -1) {

    // Printa testo di errore
    perror("Listen Fallita");
    return 1;
  }

  // lunghezza della struttura che ricevo dal client
  len = sizeof(struct sockaddr_in);

  while(1) {

    // crea un socket "figlio" della socket s e scrive nella mia strutturra
    // remote_addr le informazioni del client e collega questo
    // nuovo socket a quello del cliente
    s2 =  accept(s, (struct sockaddr *)&remote_addr, &len);

   if (s2 == -1) {

     // Printa testo di errore
     perror("Accept Fallita");
     return 1;

   }

   // Metto a zero tutti bite nell'header
   bzero(h, 100 * sizeof(struct header *));

   // inizializza al puntatore commandline e alla struttura h[0].n al puntatore request
   commandline = h[0].n = request;

   // Legge gli header della richiesta
   for( j = 0 ,k = 0; read(s2, request+j, 1); j++) {

       if(request[j] == ':' && (h[k].v == 0)) {

         request[j] = 0;
         h[k].v = request + j + 2;

       }
       else if((request[j] == '\n') && (request[j - 1] == '\r')) {

         request[j - 1] = 0;

         if(h[k].n[0] == 0) break;

         h[++k].n = request + j + 1;

       }
   }

   // è la richiesta fatta dal client al server (GET: ...)
   printf("Command line = %s\n", h[0].n);

   // Stampa gli header della richiesta
   for(i = 1; i < k; i++) {

     printf("%s ----> %s\n", h[i].n, h[i].v);

   }

   /*----------PARTE NUOVA: estraggo il valore dell'header Host e lo salvo in hostname ----------------- 
   e controllo che non corrisponda a quelli nella blacklist che sono indirizzi IP*/
   char hostname[100]; 
   for(i =1; i<k; i++){
    if(strcmp(h[i].n, "Host")==0){
        strcpy(hostname, h[i].v+1); //salvo il valore dell'header host, saltando 1 perchè c'è uno spazio prima del valore
    }
    if (strcmp(h[i].n, "Location")==0){
        strcpy(location, h[i].v+1); //salvo il valore dell'header location, saltando 1 perchè c'è uno spazio prima del valore
    }
   }

   struct hostnet* IP; 
   printf("resolving hostname to IP\n"); 
   IP= gethostbyname(hostname); 

   if (IP==NULL){
    printf("errore nella risuluzione dell'hostname in IP\n");
   }

   int intIP = inet_ntoa(IP); //FUNZIONE IMPORTANTE: converte hostnet in stringa 

   else {
    printf("Hostanme risolto in indirizzo IP: %d\n", intIP); 
   }

   //confronto indirizzo IP con quelli della blacklist
   for (i=0; i<temp; i++){
    if (strcmp(intIP, blackpages[i].page)==0){
        printf("Indirizzo IP del client è nella blacklist\n Indirizzo IP: %s\n Indirizzo blacklist: %s\n", intIP, blacklistpages[i].page); 
        sprintf(response, "HTTP/1.1 307 Temporary redirect\r\nLocation: %s\r\n\r\n", location); //location è la pagina di reindirizzamento
        write(s2, response, strlen(response)); 

        //IMPORTNATE: dopo aver inviato la risposta di redirect al client, devo chiudere la connessione con il client e continuare ad accettare nuove richieste
        close(s2); 
        continue; 
    }
   }

//----------------FINE PARTE NUOVA-------------------


   // punta all'inizio della request come commandline
   method = commandline;

   // Con questo ciclo appena trova uno spazio tra i caratteri della prima richiesta
   // esce dal ciclo e sostituisce alla spazio il valore zero, così da avere il tipo di richiesta separato
   // dal resto che viene messo in path
   for(i = 0; commandline[i] != ' '; i++){} commandline[i] = 0; path = commandline + i + 1;

   // Con questo ciclo appena trova uno spazio tra i caratteri della prima richiesta
   // esce dal ciclo e sostituisce alla spazio il valore zero, così da avere il tipo di richiesta separato
   // dal resto che viene messo in vet
   for(i++; commandline[i] != ' '; i++){} commandline[i] = 0; ver = commandline + i + 1;
   /* il terminatore NULL dopo il token versione è già stato messo dal parser delle righe/headers*/

   printf("method=%s path=%s ver=%s\n", method, path, ver);




   // Controlla se /cgi-bin/ è nel path
   if(strncmp(path, "/cgi-bin/", 9) == 0) {

     //Stampo il contenuto della richiesta nel file tmpfile.txt
     // path+9 percè l'url è path/cgi-bin/ e /cgi-bin/ sono 9 caratteri
     // -> path/cgi-bin/...

     // pendo il comando arrivato a path + 9 (dopo /cgi-bin/) e lo scrivo insieme
     // a  > tmpfile.txt nel buffer command
     sprintf(command, "%s > tmpfile.txt", path + 9);
     printf("Eseguo il comando %s\n", command);

     // esgue il comando command a sistema e scrive l'output nel file tmpfile.txt
     // essendo che il comando eseguito è ls > tmpfile.txt
     t = system(command);

     // Se t è andato a buon fine
     if (t != -1)
       // Sovrascrive il nome del file sulla posizione + 1 del path => /tmpfile.txt
       // al posto di /cgi-bin/...
       strcpy(path + 1, "tmpfile.txt");
     }

   // Indico che voglio aprire il file path+1 in lettura con "rt"
   if ((fin = fopen(path + 1, "rt")) == NULL){
     // Se null -> errore
     sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
     // Scrivo sul socket s2 la risposta del HTTP/1.1 404 Not Found\r\n\r\n
     // che va al cliente
     write(s2, response, strlen(response));

     } else {

       // Se NOT null -> errore
       sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
       // // Scrivo sul socket s2 la risposta del HTTP/1.1 200 OK\r\n\r\n
       // che va al cliente
       write(s2,response,strlen(response));
       // Fino a che dal file fin non troviamo EOF fai...
       // Apre il file
       while ( (c = fgetc(fin)) != EOF ) {

         // Scrivo nel Socket il contenuto del file
         write(s2, &c, 1);

       }

       // Chiudo la connessione
       fclose(fin);
     }

     // Chiudo il Socket
     close(s2);
 }
}


/***`inet_ntoa`**

La funzione converte un indirizzo IP binario (32 bit) in stringa leggibile. Il problema nel tuo codice era che passavi un `struct hostent*` intero invece del campo corretto:

```c
// SBAGLIATO
int intIP = inet_ntoa(IP);

// CORRETTO
char *intIP = inet_ntoa(*(struct in_addr*)(IP->h_addr_list[0]));
// h_addr_list[0] è il primo indirizzo IP associato all'hostname
// va castato a struct in_addr* perché inet_ntoa si aspetta quel tipo
// restituisce char*, non int
```

Però in realtà per la blacklist non ti serve affatto fare questa conversione — il file blacklist contiene già URL come stringhe (`http://127.0.0.1:17999/blacklist.html`), quindi puoi confrontare direttamente stringhe senza risolvere nulla in IP.

---

**Header `Referer`**

È un header che il browser include automaticamente in ogni richiesta, indicando da quale pagina l'utente stava navigando quando ha cliccato il link. Per esempio, se sei su `blacklist.html` e clicchi un link verso `index.html`, la richiesta per `index.html` conterrà:

```
GET /index.html HTTP/1.1
Host: 127.0.0.1:17999
Referer: http://127.0.0.1:17999/blacklist.html
```

È esattamente quello che ti serve: il server riceve la richiesta per `index.html`, legge il Referer, lo confronta con la blacklist, e se c'è corrispondenza blocca l'accesso e rimanda indietro il client alla pagina di provenienza (cioè proprio il Referer).

```c
// estrai il Referer dagli header
char referer[200] = "";
for (i = 1; i < k; i++) {
    if (strcmp(h[i].n, "Referer") == 0) {
        strcpy(referer, h[i].v + 1); // +1 per saltare lo spazio dopo ":"
    }
}

// confronta con la blacklist
for (i = 0; i < temp; i++) {
    if (strcmp(referer, blackpages[i].page) == 0) {
        sprintf(response, "HTTP/1.1 307 Temporary Redirect\r\nLocation: %s\r\n\r\n", referer);
        write(s2, response, strlen(response));
        close(s2);
        continue; // torna al while(1) ad aspettare il prossimo client
    }
}
```*/