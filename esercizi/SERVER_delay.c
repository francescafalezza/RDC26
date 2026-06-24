/*
ARGOMENTI: REINDIRIZZAMENTO E SALVATAGGIOO RICHIESTE

Esame di Reti di Calcolatori -  2 settembre 2016

Implementare un server HTTP che:

- reindirizza a una pagina predefinita se la risorsa di destinazione non è disponibile 
- Invii una risposta temporaneamente non disponibile se la risorsa è disponibile, 
e dopo una seconda richiesta dia l'output.

**Suggerimenti**:
L'header `Retry-After` è ignorata dalla maggior parte dei browser web, 
quindi il reindirizzamento non avverrà dopo 10 secondi, ma immediatamente

IMPLEMENTAZIONE: 

- salvo il path della REQUEST e salvo la risorsa in una variabile filename

- se fopen(filename, 'r') == NULL la risorsa non è disponibile, il client viene reindirizzato a una pagina predefinita (notFound.html)
    response: 307 Temporany Redirect 

- se fopen !=NULL la risorsa è disponibile. 
    al primo accesso response: 202 Accepted 
    al secondo accesso ritorna l'output

    come memorizzare la prima e seconda richiesta? Devo usare i valori degli Header ma quale???
    potrei salvare in un buffer le risorse richieste così poi confronto la nuova risorsa con quelle del buffer e se corrispondono allora posso dare l'output
    ma devo anche memorizzare che client fa le richieste. 

USO VARIABILI E FUNZIONI GLOBALI:
funzione int isVisited(char *path), mi ritorna 1 se la risorsa è già presente in un buffer della struct visisted

uso una struct visited{
    char * path; 
    char * ipClient;   
}visisted[100];

quindi se la risorsa è disponibile controllo se la coppia ipClient e path sono già presenti
        -se SI-->ritorno l'output del risorsa
        se NO--> salvo la coppia nel buffer e mando temporany redirect. Uso Header Location

PROBLEMA: se il server si riavvia perde la memoria. Potrei gestire in modo diverso gli accessi così d non avere questo problema
Potrei scrivere l'IP e il path nel file la prima volta che viene richiesto 
la seconda volta controllo se all'inzio del file c'è scritto l'IP e il path (simile a gestione date della cache)



*/


#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

//------------------------------------PARTE NUOVA---------------------------
//importante che siano funzioni globali perchè ad ogni while il server apre una nuova connessione e perde memoria. 
//struttura dati per ipClient e percorso risorse
//NELLA STRUTTURA GLOBALE LE VARIABILI DEVONO ESSERE BUFFER
/*SE OGNI VOLTA ASSEGNO AL PUNTANTORE GLOBALE IL VALORE DEL PUNTANTORE LOCALE, POI QUEST'ULTIMO VIENE SOVRASCRITTO E PERDO LE INFO
DEVO USARE DEI BUFFER GLOBALI E FARE STRCPY*/
struct visited{
    char * path; 
    char * ipClient; 
}current_visit[1000]: 

int visit_count = 0; 

//Funzione per verificare se il client ha già richiesto la risorsa
int isVisited(char *path, char * clientIp){

    for (int i =0; i<visit_count; i++){
        if(strcmp(path, visited[i].path)==0 && strcmp(clientIp, visited[i].ipClient)==0){
             printf("Risorsa visitata: %s\n Dal client: %s ", visited[i].path, visited[i].ipClient);
            return 1; 
        }
    }

    
    addVisit(path, clientIP); 
    return 0; 
}

void addVisit(char *path, char * clientIP){
    strcpy(current_visit[visit_count].path, path); 
    strcpy(current_visit[visit_count].ipClient, clientIP); 
    visit_count++; 
}

//---------------------------------FINE PARTE NUOVA--------------------------

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

  char *filename; 
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

   /*-------------------------------PARTE NUOVA------------------------------
   provo ad aprire la risorsa e se non è disponibile reindirizzo il client a un'altra pagina*/

   FILE *file = fopen(path+1, "r"); 

   if(file==NULL){
        printf("Risorsa non diponibile\n"); 
        
        //legge il file in memoria per sapere il Content Lenght
        strcpy(path+1, "notFound.html"); 

        FILE *f_redirect= fopen(path, "r"); 

        int bodySize=0; 
        char *body=malloc(100000); 
        int c =0; 

        while((c=fget(f_redirect)) !=EOF){
            body[bodySize++]=c; 
        
        }
        fclose(f_redirect); 

        sprintf(response, "HTTP/1.1 307 Temporany Redirect\r\nContent-Length: %d\r\n\r\n", bodySize); 
        write(s2, response, strlen(response)); 
        write(s2, body, bodysize); 
        close(s2); 

   }

   strcpy(path+1, filename); 

   /*se la risorsa è diponibile controllo se è già presente la coppia IP e path nel buffer current_visit.
   Se ritona 0 e mando messaggio 503 Service Unavailable 
   se tirona 1 allora mando l'output*/
   //risolvo l'ip del client 

   char *current_clientIP = inet_ntoa(remote_addr.sin_addr); 

   int is_visited = isVisited(filename, current_clientIP); 

   if(!isVisited){
        printf("La risorsa %s non è mai stata richiesta dal client %s\n", filename, current_clientIP); 
        sprintf(response, "HTTP/1.1 503 Service Unavailable\r\nRetry-After: 10\r\n"); 
        write(s2, response, strlen(response)); 
        close(s2); 
        exit(0); 
   }

   else{ //invio l'output della risorsa
    printf("La risorsa %s è già stata richiesta almeno una volta dal Client %s\n", filename, current_clientIP);

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
  close(s); 
}
//----------------------------------FINE PARTE NUOVA ------------------------------
 /*  // Controlla se /cgi-bin/ è nel path
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
     close(s2);*/
 