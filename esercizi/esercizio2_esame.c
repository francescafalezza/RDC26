/* Universita' degli Studi di Padova

              Dipartimento di Ingengeria dell'Informazione

              Prof. Nicola Zingirian Prof. Federico Botti

                     Esame di Reti di Calcolatori

                            4 Luglio 2025

                         ORA CONSEGNA :  12.55

Modificare il programma di web server TCP realizzato a lezione, il cui
nome sorgente  corrisponde alla  propria matricola (es.  123456.c), in
modo che:

1. Il server ascolti sulla  porta indicata nel file porta.txt (copiata
e cablata nel codice, non letta a runtime dal file).

2. Il server gestisca un cookie di identificazione utente nel formato:

   user=utente<indice>

   dove <indice> e' un progressivo che viene incrementato: 1,2,3 etc

3. Alla prima richiesta senza cookie da parte di un client:

   - Il server assegna un nuovo identificatore user=utente<indice> con
Max-Age=60 secondi, utilizzando l'header Set-Cookie (RFC 6265, v. link
piu' sotto)

   - Registra l'utente in memoria con un contatore di accessi iniziale
pari a 0.

4. Alle richieste successive, provenienti dal browser, con header:

   Cookie: user=utente<indice>

   - Il contatore associato a quell'<indice> viene incrementato.

   - Il valore  aggiornato  viene  incluso  nel  corpo della  risposta
   all'inizio dell'Entity Body,  nella forma

             "<html><br> Numero accessi:  X <br>"

   dove X e' il valore del contatore.

5. Il contatore deve essere indipendente per ciascun utente.

Riferimento standard:

 RFC 6265 - HTTP State Management Mechanism
 Sezione 4.1 – Syntax and Semantics of the Set-Cookie Header Field
 https://datatracker.ietf.org/doc/html/rfc6265#section-4.1

Test richiesto:

- Aprire  una finestra normale  del browser  e visitare il  server: il
contatore deve aumentare a ogni refresh.

-  Aprire una  finestra in  modalita' incognita  e visitare  lo stesso
server: il contatore deve partire da 1 e aumentare indipendentemente.

- I due contatori devono progredire separatamente.

IMPLEMENTAZIONE:

Uso struttura globale visite per salvare trio (ip_client, n_accessi, risorsa)
Uso funzione globale per aggiungere alla struttura ogni nuova coppia con n_accessi=0, e per aggioornare il numero di accessi:
        -se il trio è gia nella struttura allora aggiorno n_accessi
        -altrimenti aggiungo il trio alla struttura

Dopo il parsing degli header controllo header Cookie:
        - se presente NON è la prima richiesta 
        faccio funzione aggiorna_visite, che mi ritorna il n_accessi aggiornato. 
        server manda risposta con Set-cookie: user=ip_client<n_accessi>
        sprintf(response, "<html><br> Numero accessi:  %d <br>", n_accessi)-->write
        manda il body

        - se è la PRIMA RICHIESTA
        funzione aggiorna_visite
        manda risposta con Set-cookie: user=ip_client<n_accessi>
        manda body normale

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

//----------------------------PARTE NUOVA---------------------
//struttura per salvare ip_client, n_accessi, risorsa
struct visite{
    char ip_client[100];
    int n_accessi;
    char risorsa[100]; 
};
 
struct visite visite_salvate[1000]; 
int contatore_visite=0; 

//funzione che aggiorna le visite da parte di un client per una data risorsa
int aggiorna_visite(char *ip_client, char *risorsa){

    for(int i =0; i<contatore_visite; i++){
        //se il client ha già visitato questa risorsa, aggiorno il contantore
        if((strcmp(visite_salvate[i].ip_client, ip_client)==0) && (strcmp(visite_salvate[i].risorsa, risorsa)==0)){
            visite_salvate[i].n_accessi+=1; 
            return visite_salvate[i].n_accessi; 
        }
    }
    //se non c'è mai stata una richiesta per quella risorsa allora la aggiungo alla struct e metto il primo accesso

    strcpy(visite_salvate[contatore_visite].ip_client,ip_client); 
    strcpy(visite_salvate[contatore_visite].risorsa,risorsa); 
    visite_salvate[contatore_visite].n_accessi=1; 
    contantore_visite++; 

    return visite_salvate[contatore_visite-1].n_accessi; 
    
}

//-----------------------------FINE PARTE NUOVA-----------------
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

   //------------------PARTE NUOVA---------------
   //cerco header Cookie: valore_cookie = "user=utente<n_accessi>"
   char *cookie=NULL; 
   char valore_cookie[100];
    char utente [100]; 
   int n_accessi_corrente=0; 

   char *host; 

   for(int i=0; i<k; i++){
        if(strcmp(h[i].n, "Cookie")==0){
            cookie=h[i].v;
            strcpy(valore_cookie, cookie); 
            //estraggo info utili
            sscanf(valore_cookie, "user=%s<%d>", utente, &n_accessi_corrente); 
        }

        if(strcmp(h[i].n, "Host")==0){
            host=h[i].v; 
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

   //----------------------------------------------------------------------
   //controllo se è la prima richiesta:
   //se cookie=NULL è la prima richiesta 
   //altrimenti ci sono già stati accessi.

   FILE *f; 
   if(cookie==NULL){ //PRIMA RICHIESTA: set cookie e mando la risorsa.
        //salvo indirizzo ip del client e la risorsa--> aggiorna_visite
        struct hostent *he=gethostbyname(host);
        char *ip_client=inet_ntoa(remote_addr.sin_addr);  //IMPORTANTE 
        
        int contatore = aggiorna_visite(ip_client, path); //dovrebbe essere 1
        printf("Prima visita del client: %s\n per la risorsa: %s", ip_client, path+1); 

        //mando il contenuto del file come entity body della response
        f=fopen(path+1, "rt");

        if(f==NULL){
            printf("Errore nell'apertura del file\n"); 
            sprintf(response, "HTTP/1.1 404 Not found\r\n\r\n"); 
            write(s2, response, strlen(response)); 
            close(s2); 
            continue; 
        }

        sprintf(response, "HTTP/1.1 200 Ok\r\nMax-Age: 60\r\nSet-Cookie: user=%s<%d>\r\n\r\n", ip_client, contatore);
        write(s2, response, strlen(response)); 

       }

   else if(cookie!=NULL){//RICHIESTE SUCCESSIVE ALLA PRIMA

        struct hostent *he=gethostbyname(host);
        char *ip_client=inet_ntoa(remote_addr.sin_addr);  //IMPORTANTE
        int n_visite_aggiornato=aggiorna_visite(ip_client, path+1);
        printf("Numero accessi prima di questa visita: %d\n N accessi dopo la visita corrente: %d\n", n_accessi_corrente, n_visite_aggiornato); 

        sprintf(response, "HTTP/1.1 200 Ok\r\nSet-Cookie: user=%s<%d>\r\n\r\n", ip_client, n_visite_aggiornato); 
        write(s2, response, strlen(response)); 

        //scrivo all'inizio del body <html><br> Numero accessi:  %d <br>", n_accessi
        sprintf(response, "<html><br> Numero accessi:  %d <br>", n_visite_aggiornato);
        write(s2, response, strlen(response));

        f=fopen(path+1, "rt");

        if(f==NULL){
            printf("Errore nell'apertura del file\n"); 
            sprintf(response, "HTTP/1.1 404 Not found\r\n\r\n"); 
            write(s2, response, strlen(response)); 
            close(s2); 
            continue; 
        }
   }

   //leggo dal file e salvo in response
    int n=0; 
    int byte_letti=0;
    char filebuff[1024];
    while((n=fread(filebuff+byte_letti, 1, 10, f))>0){
        byte_letti+=n; 
        
    }

    write(s2, filebuff, byte_letti);
    fclose(f);
    close(s2);

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