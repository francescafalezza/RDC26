

/*Si modifichi il programma Web Server sviluppato durante il corso in modo tale che vieti, 
tramite opportuno codice di errore (cfr. RFC2616, cap. 10.4 ), ad un user agent (browser) 
convenzionale di accedere alla risorsa /file2.html se prima il medesimo user agent non abbia 
precedentemente avuto accesso alla risorsa /file1.html.

Per implementare questa funzione si utilizzino i meccanismi di gestione dello stato HTTP, detti Cookies, 
facendo riferimento agli esempi della sezione 3.1 della RFC6265 e alle grammatiche nella Sezione 4 
del medesimo documento.

Il meccanismo dei Cookie può essere visto come una variante del meccanismo di autenticazione visto a lezione, 
in quanto, al posto di richiedere al client di includere in ogni request l’header WWW-authenticate riportante 
credenziali (username e password) precedentemente registrate sul server, similmente richiede al client di includere, 
in ogni request, l’header Cookie riportante una il nome e il valore di una variabile di stato comunicata dal server 
al client al primo accesso. Questo permette al server di capire che tante richieste provengono da uno stesso 
user agent e condividono uno stesso stato, senza necessità di gestire la registrazione di utenti.

N.B. lo user agent non sarà obbligato ad accedere al /file1.html subito prima di accedere al /file2.html. Al contrario il server deve poter permettere anche la sequenza di accesso
/file1.html, ...<altre risorse>..., /file2.html.
  
## Agli studenti più preparati si richiede un requisito ulteriore:
  
Non sarà sufficiente allo user agent accedere una volta per tutte a /file1.html per poi aver il permesso di accedere tante volte al /file2.html. Al contrario, dopo aver dato il permesso di accedere al /file2.html, il server vieterà un secondo accesso se prima il medesimo user agent non avrà avuto accesso di nuovo al /file1.html.

PSEUDOCODICE:
- se viene richiesto di accedere a /file1.html il server risponde con l'header Set-Cookies e manda la risorsa. Viene impostata una 
variabile accesso =1.
- se viene richiesto di accedere a /file2.html e accesso=1 allora si controlla l'header Cookie della richiesta e se corrisponde a quello
del server allora il server manda la risorsa e viene settata accesso = 0. 
*/


#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

#define COOKIE_GRANTED "SID=GRANTED"
#define COOKIE_USED "SID=USED"

// Strutura dati dell'header
struct header{
char * n;
char * v;
}h[100];

int tmp;

int main() {

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
   //----------------------PARTE NUOVA-------- salvo header cookie se c'è
   char *cookieRequest; 
   for(i = 1; i < k; i++) {

     printf("%s ----> %s\n", h[i].n, h[i].v);

     if(strcmp(h[i].n, "Cookie")==0){
          cookieRequest=h[i].v +1; 
     }

   }

   //---------------------FINE PARTE NUOVA----------------


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



   /*----------------------PARTE NUOVA-----------------------
   se viene richiesto l'accesso a file1 viene impostato un Header Set Cookie nella risposta con un valore diverso ogni volta
   se viene richiesto accesso a file2 controllo l'header Cookie della richiesta, deve avere valore uguale a quello settato in precedenza
      viene fatto di nuovo Set-Cookie nella risposta 

    USO VARIABILI DEL SERVER
   
   */
  
   char cookie [] = 'SID=31d4d96e407aad42'; 
   

   //CASO 1: risorsa richiesta è file1
   if(strncmp(path, "/file1.html", 11)==0){
          FILE *f =fopen(path+1, "r"); 
          if (f==NULL){
            sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n"); 
            write(s2, response, strlen(response)); 
            close(s2); 
          }

          //lettura del file e salvataggio
          unsigned char *body=malloc(10000); 
          int bodysize=0; 
          int c; 

          while((c=fgetc(f)) != EOF){
            body[bodysize++]=c; 
          }
          fclose(f); 

          //mando la risposta con status code e header, poi invio contenuto del file
          sprintf(response, "HTTP/1.1 200 OK\r\nSet-Cookie: %s\r\n\r\n", COOKIE_GRANTED); 
          write(s2, response, strlen(response)); 
          write(s2, body, bodysize); 
          free(body); 
          accesso = 1; 

   }

   //CASO 2: richiesta accesso a file2. Controllo dell'header Cookie nella REQUEST
  else if(strcmp(path, "/file2.html", 11)==0 ){

        if (strcmp(cookieRequest, COOKIE_GRANTED)==0 && cookieRequest != NULL){
            printf("Accesso alla risorsa file2 consentito"); 
            FILE *f =fopen(path+1, "r"); 
            if (f==NULL){
              sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n"); 
              write(s2, response, strlen(response)); 
              close(s2); 
          }

          //lettura del file e salvataggio
          unsigned char *body=malloc(10000); 
          int bodysize=0; 
          int c; 

          while((c=fgetc(f)) != EOF){
            body[bodysize++]=c; 
          }
          fclose(f); 
          sprintf(response, "HTTP/1.1 200 OK\r\nSet-Cookie: %s\r\n\r\n", COOKIE_USED); 
          write(s2, response, strlen(response)); 
          write(s2, body, bodysize); 
          free(body); 

           
        }

        else {
           
          sprintf(response, "HTTP/1.1 403 Forbidden\r\n\r\n"); 
          write(s2, response, strlen(response)); 
          close(s2); 
          continue;
        }
  }


  //--------------------FINE PARTE NUOVA----------------------


   // Controlla se /cgi-bin/ è nel path
   else if(strncmp(path, "/cgi-bin/", 9) == 0) {

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
   else{
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
 }}
}

