/*
**Consegna**

Si scriva un programma in linguaggio C che implementi un web server HTTP/1.1, basato sui sorgenti sviluppati 
a lezione, con le seguenti specifiche aggiuntive.

### 1) Gestione dell'header `Range` (RFC 2616, sezioni 14.35 e 14.16)

Il server deve supportare richieste di **range retrieval** su risorse statiche. 
Se la richiesta GET include un header:

```
Range: bytes=<first-byte-pos>-<last-byte-pos>
```

il server deve:

- Estrarre `first-byte-pos` e `last-byte-pos` dal valore dell'header (si gestisca **solo** il caso con un singolo range esplicito, es. `bytes=100-499`; non è richiesto il supporto a range multipli o al formato suffisso `bytes=-500`).
- Verificare che il range sia soddisfacibile rispetto alla dimensione reale del file (usare `fseek`/`ftell` per ottenerla). 
Se `first-byte-pos` è maggiore o uguale alla lunghezza del file, il server deve rispondere con:
  ```
  HTTP/1.1 416 Requested Range Not Satisfiable
  Content-Range: bytes <dimensione-file>
  ```
  e chiudere la connessione (senza entity body).

- Se il range è valido, il server deve rispondere con status `206 Partial Content`, includendo gli header:
  ```
  Content-Range: bytes <first>-<last>/<dimensione-totale>
  Content-Length: <last - first + 1>
  ```
  seguiti dal solo segmento di file richiesto (usare `fseek` per posizionarsi su `first-byte-pos` prima di leggere).
- Se l'header `Range` **non** è presente, il comportamento deve restare quello standard 
(200 OK con l'intero file, scegliendo tra `Content-Length` o `chunked` come da implementazione vista a lezione).

### 2) Gestione dell'header `Accept-Language` (RFC 2616, sezione 14.4)

Per ogni risorsa richiesta con metodo GET, se esiste un file con lo stesso nome ma suffisso di lingua 
(es. per `/index.html` con `Accept-Language: it` deve cercare `/index.it.html`), 
il server deve servire la variante localizzata, se presente, altrimenti ripiegare sulla risorsa originale. 
**Si gestisca solo la prima lingua indicata nell'header** 
(si ignorino eventuali `q-value` e lingue successive nella lista separata da virgole).

### 3) Logging delle richieste su file (funzioni globali richieste)

Il server deve mantenere un **log persistente su disco** di tutte le richieste servite, 
in un file `access.log` nella working directory del processo, 
con una riga per ogni richiesta nel formato:

```
<IP-client>	<metodo>	<URI>	<status-code>	<bytes-inviati>
```


### 4) Concorrenza

Il server deve gestire più client contemporaneamente tramite `fork()` (analogamente a `SERVER_gataway.c` e `PROXY.c` visti a lezione): per ogni connessione accettata, il processo padre continua il ciclo di `accept()`, mentre il figlio gestisce l'intera transazione e termina con `exit()` dopo aver chiuso il socket.

### 5) Vincoli implementativi

- Si riutilizzi la tecnica di parsing degli header byte-a-byte vista a lezione (struct `header` con campi `n` e `v`, terminazione tramite sostituzione di `\r\n` con `\0`).
- Non è consentito l'uso di librerie HTTP esterne: tutto il parsing deve essere fatto manualmente sui buffer.
- Si gestiscano almeno i metodi `GET` e `HEAD`; per ogni altro metodo si risponda `405 Method Not Allowed` con header `Allow: GET, HEAD`.
- Si discuta nella relazione perché `Content-Length` e `Transfer-Encoding: chunked` non possono coesistere nella stessa risposta (RFC 2616 §4.4), e quale dei due si è scelto di usare per le risposte `206 Partial Content`.

---

**Domande teoriche da includere nella relazione:**

1. Perché una risposta `304 Not Modified` o `204 No Content` non deve mai includere un message-body, e come fa il client a saperlo *senza* dover leggere un eventuale `Content-Length`?
2. Spiegare la differenza tra **entity-length** e **transfer-length** (sezione 7.2.2 e 4.4 della RFC) e come questa distinzione si applica al caso della risposta `206 Partial Content` che avete implementato.
3. Perché l'header `Range` rende la GET "condizionale" solo se combinato con `If-Range`? Cosa succederebbe, in termini di correttezza della cache, se un client richiedesse un range su una risorsa modificata dal server tra una richiesta e l'altra, senza usare `If-Range`?

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
} h[100];



//funzione per logging su file
void logging(char *ip_client, char *metodo, char *uri, char *status_code, int byte_inviati){
    FILE *log=fopen("access.log", "a"); 
    fprintf(log, "%s %s %s %s %d", ip_client, metodo, uri, status_code, byte_inviati);

}


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

   /*-------------------------------PARTE NUOVA-----------------------------------------
   Cerco header Range e accept language
   Range: butes=posIniziale-posFinale, salvo le posizioni 
   Accept-language: it, ... salvo la prima lingua. 
   
   */

   char range[100]; 
   int pos_primo_byte=0; 
   int pos_byte_finale=0; 
   char *temp;
   char valore_lingue[100];
   char lingua[10];

   int flag =0;

   int accept=0; 
   for (int i = 0; i<k; i++){
    if(strcmp(h[i].n, "Range")==0){
        strcpy(range, h[i].v); 
        sscanf(range, "bytes=%d-%d", &pos_primo_byte, &pos_byte_finale); 
        flag =1; 
    }

    if(strcmp(h[i].n, "Accept-Language")==0){
        strcpy(valore_lingue, h[i].v);
        for (int j =0; valore_lingue[j] != ','; j++){} valore_lingue[j]=0;  
        strcpy(lingua, valore_lingue); 
        accept=1;
    }
   }

   //Controllo se esiste il file in lingua ne leggo il contenuto per salvare la dimesione
   //Controllo la correttezza del valore del range

   //path da cercare = nome.lingua.tipo--> devo modificare il path che ho e cercare quello in lingua
   char nuova_risorsa[100];
   char *nome_risorsa;
   char *tipo_risorsa;

   int l=0;
   for (l=0; path[l] != '.'; l++){} 
   path[l]=0; 
   nome_risorsa=path+1; 
   tipo_risorsa=path+l+1;

   FILE *file_lingua; 
   //sscanf(path, "/%s.%s", nome_risorsa, tipo_risorsa); 
   if(accept){
   //creo il nome della nuova risorsa da cercare
   sprintf(nuova_risorsa, "%s.%s.%s", nome_risorsa, lingua, tipo_risorsa);

   file_lingua=fopen(nuova_risorsa, "r");

   if(file_lingua == NULL){
        //sovrascrivo nuova_risorsa con il valore della risorsa originale
        printf("La risorsa %s non esiste\n", nuova_risorsa);

        strcpy(nuova_risorsa, path+1);
        printf("Viene servita la risorsa originale: %s\n", nuova_risorsa); 
        file_lingua = fopen(nuova_risorsa, "r");
   }
  }

  else{
    file_lingua = fopen(path+1, "r"); 
  }

  if(file_lingua ==NULL){
    sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n"); 
    write(s2, response, strlen(repsonse));
    close(s2);
    exit(0); 
  }
   //utilizzo file_lingua che sarà in ogni caso il file corretto a seconda che il file in lingua esista o meno.
   //salvo la dimensione del file

   int byte_letti=0; 
   char body[1024*1024];
   int n=0;
   while((n=fread(body+byte_letti, 1, 20, file_lingua))>0){
    byte_letti +=n; 
   }

   //salvo l'ip del client

   char ip_client[100];
   char *ip=inet_ntoa(remote_addr.sin_addr);
   strcpy(ip_client, ip);

   //se il client ha inviato l'header range, gestisco come richiesto:
   if(flag){
   //controllo il range. Se il primo byte è maggiore della dimesione del file mando messaggio di errore
    if(pos_primo_byte>byte_letti){
        sprintf(response, "HTTP/1.1 416 Requested Range Not Satisfiable\r\n Content-Range: bytes=%d\r\n\r\n", byte_letti);
        write(s2, response, strlen(response));
        close(s2); 
        continue; 
        }

    //altrimenti mando il file. 
    int content_length=pos_byte_finale-pos_primo_byte+1; 
    sprintf(response, "HTTP/1.1 206 Partial Content\r\nContent-Range: bytes %d-%d/%d\r\nContent-Length: %d\r\n\r\n",pos_primo_byte, pos_byte_finale, byte_letti, content_length);
    write(s2, response, strlen(response)); 

    //ho già salvato il contenuto del file quindi uso body e mando solo il range richiesto
    int byte_inviati=0; 
    int m=0; 
    while(byte_inviati<content_length){
        m=write(s2, body+pos_primo_byte+byte_inviati, content_length-byte_inviati);
        byte_inviati +=m; 
    }

    
    logging(ip_client, method, path,"206", byte_inviati);
    }

    else {
    //se non c'è range mando il body completo 
    sprintf(response, "HTTP/1.1 200 Ok\r\n Content-Length: %d\r\n\r\n", byte_letti);
    write(s2, response, strlen(response));
    write(s2, body, byte_letti);
    logging(ip_client, method, path,"200", byte_letti);
    
    }
    close(s2);
    fclose(file_lingua);

}
}
    /*

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