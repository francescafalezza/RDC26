/*Si modifichi il programma che implementa il web server in modo che questo, 
non appena riceve dal client una request per la risorsa corrispondente al path “/reflect”, 
anziché cercare un file da aprire ed inviare, invii al client una response nella quale l’entity body sia

a. il testo esatto corrispondente all’intera request inviata dal client al server, 
comprensiva di tutti gli elementi che la compongono 
b. \r\n
c. L’indirizzo IP in notazione decimale puntata da cui il client ha inviato la propria request 
d. \r\n
e. il port da cui il client ha effettuato la propria request


IMPLEMENTAZIONE:
1. se path=/reflect inizia parte nuova con richieste d'esame 
2. inserisco nella response a. tutta la request (quindi salvo la request e la copio nella response con un ciclo for o sprintf)
3. salvo in una variabile char IP[10] l'indirizzo IP del client in notazione decimale
4. salvo la port del client in una variabile char port [10]
5. inserisco IP e port nella response con sprintf

STRUTTUA sockaddr_in
struct sockaddr_in {
    sa_family_t    sin_family;  // AF_INET
    in_port_t      sin_port;    // porta in network byte order
    struct in_addr sin_addr;    // indirizzo IP
};

struct in_addr {
    uint32_t s_addr;  // IP come intero a 32 bit
};


sprintf sovrascrive ogni volta!!

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

    // crea un socket "figlio" della socket s e scrive nella mia struttura
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

   /*--------------------------INIZIO PARTE NUOVA-----------------------------
   se path = /reflect allora il server devo mandare come repsonse tutta la request\r\nIP client\r\nport client*/

   if(strcmp(path, "/reflect")==0){

    printf("path: %s\nRitorno al client tutta la richiesta\n", path); 

    sprintf(response, "HTTP/1.1 200 OK\r\n\r\n"); 
    write(s2, response, strlen(response)); 

    //salvo IP client e port client, che sono nella struttura del client remote_addr.sockaddr_in
    char client_ip[100]; 
    int client_port =0; 

    strcpy(client_ip, inet_ntoa(remote_addr.sin_addr)); 

    client_port = ntohs(remote_addr.sin_port)  //FUNIONE PER CONVERTIRE LA PORTA IN INT 

    //non devo inviare il contenuto di un file ma devo inviare tutta la request 
    //salvo la request nel buffer dedicato alla response

    //---------IMPORTANTE I<J---------

    for (int i =0; i<j; i++){
        response[i] = request[i]; 
        printf("Lettura da request e salvataggio in response di %s", request[i]);
    }

    write(s2, response, strlen(response)); 

    sprintf(response, "\r\nIP client:%s\r\nPort client:%d", client_ip, client_port); 

    write(s2, response, strlen(response)); 
    close(s2); 
    continue; 
   }


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