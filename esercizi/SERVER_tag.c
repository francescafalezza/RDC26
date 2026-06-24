/*Si modifichi il programma ​esame.c che implementa il Web Server sviluppato durante il corso, 
in modo tale che sia in grado di gestire il meccanismo di controllo del caching basato sull’ETag 
descritto nella ​RFC2616. Capitoli 3.11, 14.19, 14.24, 14.26.

Al posto di utilizzare la data per stabilire se una risorsa è modificata il server 
associa un identificativo comunicato con l’header “Etag” che riassume il contenuto del file 
e che rimane invariato se e solo se il contenuto della risorsa non varia.

Si provi con un browser l’accesso ripetuto ad una pagina. 
Il server si dovrà comportare correttamente a seconda che il file sia stato modificato o meno 
tra una richiesta e l’altra del client. 
Si implementi il minimo indispensabile per soddisfare le richieste del client. 
Per connettersi al server cloud usare il comando ssh username@88.80.187.84


Il server usa l'Header Etag: xyzw 
il client usa gli header If-Match o If-None-Match --> usati per rendere un metodo condizionale.

        - se l'entity tag di If-Match è uguale all' entity tag della risorsa che sarebbe stata ritonata 
        o se viene madandato come valore *, il server esegue il metodo della richiesta

        -se non c'è nessun match e viene mandaato * ma non c'è nessuna entità allora il sever non esegue il metodo della richiesta
        e manda messaggio 412 Precondition Failed.

1. Il server riceve la richiesta
2. Controlla se c'è header  If-None-Match
3. calcolo etag del file richiesto con compute_etag
4. Controlla equivalenza valore_if_match = valore_etag 
5. Se sono uguali --> viene eseguito il metodo della richiesta
6. se non sono uguali --> server non esegue il metodo e invia messaggio 402 Precondition Failed. 
7. se if-none-match =null 0 altro allora vuol dire che è la prima richiesta o è stata modificata la risorsa. 


`stat` è una system call Unix che legge i **metadati** di un file (non il contenuto). 
Restituisce una struttura `struct stat` con vari campi:

```c
struct stat {
    off_t   st_size;    // dimensione in byte
    time_t  st_mtime;   // data ultima MODIFICA del contenuto
    time_t  st_ctime;   // data ultima modifica dei metadati
    time_t  st_atime;   // data ultimo accesso in lettura
    mode_t  st_mode;    // permessi
    // ... altri campi
};
```

Si usa così:

```c
struct stat st;
stat("index.html", &st);   // legge i metadati di index.html
```

`st_mtime` (**m**odification **time**) è il timestamp dell'ultima volta che 
il **contenuto** del file è stato scritto. Viene aggiornato automaticamente 
dal sistema operativo ogni volta che qualcuno scrive nel file, e rimane invariato se nessuno lo tocca.

Quindi nell'ETag:

```c
sprintf(etag_out, "\"%ld-%ld\"", (long)st.st_size, (long)st.st_mtime);
```

combini dimensione + data ultima modifica. Se il file non cambia tra una richiesta e l'altra, 
entrambi i valori restano identici → ETag identico → `304 Not Modified`. Se qualcuno modifica il file, 
`st_mtime` cambia → ETag diverso → `200 OK` con il nuovo contenuto.
 
*/

#include <sys/stat.h> //per stat. 
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

int tmp;

//-----------------------------------------PARTE NUOVA-----------------------------------

void compute_etag(char *filename, char *etag){
    struct stat st;
    if(strcmp(filename, &st)==0){
        sprintf(etag, "%d-%d", st.st_size, st.st_mtime); 
    } 
    else 
        strcpy(etag, "/unknown/"); 

}


//----------------------------------FINE PARTE NUOVA--------------------------------------

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


   //-----------------------------PARTE NUOVA-------------------
   //cerco header If-Match e If-None-Match

   char *client_etag; 
   for (int i =0; i<k; i++){
    if (strcmp(h[i].n, "If-Match")==0){
        client_etag=h[i].v; 
    }

    if(strcmp(h[i].n, "If-None-Match")==0){
        client_etag=h[i].v; 
    }
   }

   //--------------------------FINE PARTE NUOVA--------------------

   // è la richiesta fatta dal client al server (GET: ...)
   printf("Command line = %s\n", h[0].n);

   // Stampa gli header della richiesta
   for(i = 1; i < k; i++) {

     printf("%s ----> %s\n", h[i].n, h[i].v);

   }

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



   //------------------------------PARTE NUOVA-----------------------------------------------
   /*
   DUBBIO: il path è sempre un file??

   Prendo la risorsa 
   */
   FILE *f =fopen(path+1, "r"); 
   char filename[100]; 
   srtcpy(filename, path+1); 
   if (f ==NULL){
    printf("Errore apertura file\n"); 
    sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n"); 
    write(s2, repsonse, strlen(response)); 
    close(s2); 
   }

   char etag_file[1000]; 
   compute_etag(filename, etag_file); 

   //verifico che client abbia invaito l'header If-None-MAtch
   //se l'ha inviato procedo con la verifica dei valori degli etag

   if(client_etag != NULL){

    if(strcmp(client_etag, etag_file )!=0){
        printf("Valori Etag sono diversi tra loro\nEtag inviato dal client: %s\nEtag del file: %s\n", client_etag, etag_file);
        sprintf(response, "HTTP/1.1 412 Precondition failed\r\n\r\n"); 
         write(s2, response, strlen(response)); 
        close(s2);
        continue; 
        }
    
    else{ //se la risorsa non è stata modificata
        sprintf(response, "HTTP/1.1 304 Not Modified\r\n");
        write(s2, repsonse, strlen(response)); 
        close(s2);
        continue; //IMPORTANTE!!!!!

        }               
    }


    //se la risorsa non è mai stata richiesta il server manda risposta con header etag e tutto il body
    sprintf(response, "HTTP/1.1 200 OK\r\nEtag: %s\r\n\r\n", etag_file); 
    write(s2,response,strlen(response));
       // Fino a che dal file fin non troviamo EOF fai...
       // Apre il file
       while ( (c = fgetc(fin)) != EOF ) {

         // Scrivo nel Socket il contenuto del file
         write(s2, &c, 1);

       }

       // Chiudo la connessione
       fclose(fin);
     
     // Chiudo il Socket
     close(s2);
   
    }
}
/*
   



   //-------------------------------FINE PARTE NUOVA ---------------------------------------


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