/*Esame di Reti di Calcolatori - 16 Luglio 2020

Si modifichi il web client sviluppato durante il corso per renderlo in grado di scaricare file di grandi dimensioni in
presenza di connettività di rete non affidabile caratterizzata da una frequente interruzione delle connessioni.
In queste condizioni, lo scaricamento di una risorsa di grandi dimensioni rischia di essere interrotto dalla perdita
della connessione. Risulta evidente che anche ripetendo più volte lo scaricamento del file intero si rischia ogni
volta l’interruzione e di conseguenza la probabilità di terminare con successo l’operazione diviene molto bassa.
Per ovviare a questo inconveniente il client web dev’essere modificato in modo tale che scarichi a piccoli pezzi il
file tramite una sequenza di più richieste che scaricano ciascuna un segmento del file (ad esempio di 10
Kbytes) che verranno alla fine giustapposti in un buffer per salvare il contenuto completo su un file locale.
  
Il protocollo HTTP/1.1 supporta questa funzione per mezzo dell’header “Range”. Documentato nella RFC 2616
in particolare nelle sezioni 14.35 e 14.16 e 3.12.
  

Il client fa una richiesta con Header Range: bytes 0-10000 (specifica i byte che vuole scaricare)
il server risponde con Header Content-Range: 0-10000/678900
il numero di byte effettivamente inviati è mostrato dall'header Content-Lenght

IMPLEMENTAZIONE: 
devo gestire la perdita di connessione quindi uso un buffer globale o fuori dal while(1) per salvare i vari pezzi di body
Buffer per salvare tutto l'entity body. 

Il client invia una richiesta: GET /image.png HTTP/1.1\r\nHost: www-google.com\r\nRange: 1-10000\r\n\r\n

il server risponde con response contente header Content-Range: bytes 0-10000/6790098
        leggo dopo tipo e range con sscanf(range_header_value, "%s %s", type, range_value); 
        salvo il primo valore, secondo valore e la lunghezza completa del body
        leggo il body fino a 10000 (valore scelto da me come range)
        salvo il body nel entity_buffer, nella posizione definita dal primo valore del header Content-Range 
        se il seocndo valore=lunghezza completa body --> ho finito, salvo tutto il body in un file locale e chiudo la connessione. 

AD OGNI RICHIESTA DEVE CAMBIARE IL RANGE

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

int i, tmp, j, k, t;
int s_socket;

//Inizializzo la var che indica la lunghezza della Content-Length
int entity_length;

struct sockaddr_in addr;

// Puntatore alla statusline
char * statusline;

char * entity;

// <indirizzo ip del server 216.58.213.100 >
// Array contenete l'indirzzo ip del server
//unsigned char targetip[4] = {143,204,182,107};
//unsigned char targetip[4] = { 172,217,169,3};
//unsigned char targetip[4] = { 216, 58 ,213,100 };
//unsigned char targetip[4] = { 213,92,16,101 };
unsigned char targetip[4] = { 172,217,21,68 };


// sono i 2 buffer per la richiesta e la risposta
char request[5000],response[10000];

/*----------------------------PARTE NUOVA--------------------------
funzione globale per scrivere nel buffer dell'entità il body arrivato nella posizione giusta*/

char entity_buffer[10000000]; 
 

void write_entity_buffer(char *buffer, char *current_entity, int posizione, int range){
    int byte_scritti =0; 

    while(byte_scritti<range){
        buffer[posizione+byte_scritti]=current_entity[byte_scritti]; 
        byte_scritti++; 
    }
}

//------------------------------FINE PARTE NUOVA--------------------------------


int main() {

  // chiamata a sistema che apre una comunicazione e restituisce un INT,
  // che è un File Descriptor ovvero l’indice della tabella con tutto ciò
  // che serve per gestire la comunicazione
  // int socket(int domain, int type, int protocol);

int offset=0; 
int total=-1; 
int lunghezza_tot=0; 


//IMPORTNATE 
bzero(response, sizeof(repsonse)); 


while(offset<total || total ==-1){
  s_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (s_socket == -1){

    // Liberia degli errori
    tmp = errno;

    // Printa testo di errore
  	perror("Socket fallita");

    // Printa  i dati relativi all'errore
  	printf("i=%d errno=%d\n",i,tmp);
  	return 1;

  }

// Indirizzo ipv4 del client che fa la richiesta
addr.sin_family = AF_INET;
// Port del client
// host to network short (htons)
addr.sin_port = htons(80);
// Puntatore all'array contenente l'indirizzo ip del server
addr.sin_addr.s_addr = *(unsigned int*)targetip;

// connect == -1 da errore

// connect(): è una chiamata a sistema per connettersi al socket riferito dal
// file descriptor sockfd all’indirizzo specificato da addr. addrlen specifica
// la lunghezza dell’indirizzo.

// sockaddr è generica

// sockaddr_in è dedicata all' IPv4 cioè per essere usata con
// l'Internet Protocol (TCP o UDP per il trasporto).
if (connect(s_socket,(struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {

  // Printa testo di errore
	perror("Connect fallita");

}
printf("%d\n", s_socket);
int range = 100000; 
sprintf(request, "GET /image.png HTTP/1.1\r\nHost:www.google.com\r\nRange: bytes %d-%d\r\n\r\n", offset,range+offset);

// la write riceve un file descriptor fd (in questo caso corrisponde a s)
// Un puntatore ad un array di caratteri (request)
// la dimensione del buffer (o array di carattery) (strlen(request)))
// se -1 da errore
if (write(s_socket, request, strlen(request)) == -1) {

  perror("write fallita");
  return 1;
}

// Metto a zero tutti bite nell'header
bzero(h, sizeof(struct header)*100);

statusline = h[0].n = response;

for (j = 0, k = 0; read(s_socket, response+j, 1); j++){

  if (response[j] == ':' && (h[k].v==0)){

    // é il carattere che indica la terminazione della stringa
    response[j]=0;

    h[k].v=response+j+2;
  }

  else if((response[j]=='\n') && (response[j-1]=='\r') ){

    // é il carattere che indica la terminazione della stringa
    response[j-1]=0;

    if(h[k].n[0]==0) break;

    h[++k].n=response+j+1;
  }
}

// inizializzo la lunghezza dell'entity body
entity_length = -1;
char *value_content_range; 
printf("Status line = %s\n",statusline);

int pos_iniziale; 
int pos_finale; 


for(i = 1; i < k; i++) {

  // verifico se la Content-Length è presente o meno nello header
  if(strcmp(h[i].n, "Content-Length") == 0) {

    // se la Content-Length è presente assegno il valore della lunghezza della Content-Length
    // atoi() converte ina stringa (o un puntatore ad una stringa) in intero
    entity_length = atoi(h[i].v);

    // Stampo la lunghezza della Content-Length che corrisponde alla entity_length
    printf("* (%d) ",entity_length);

  }

  //------------------------PARTE NUOVA------------------

  //separo posizione iniziale con posizione finale e valore totale--> byte 0-10000/679600


  if(strcmp(h[i].n, "Content-Range")==0){
        value_content_range=h[i].v; 

        //IMPORTNATE !!!!!
        sscanf(value_content_range, "bytes %d-%d/%d", &pos_iniziale, &pos_finale, &lunghezza_tot)
        printf("Valore del content range attuale %s\n", value_content_range); 
  }

  
  printf("%s ----> %s\n",h[i].n, h[i].v);

}

total = lunghezza_tot; 
offset=pos_iniziale+range; 

entity = (char * ) malloc(entity_length);

for(j = 0; (t = read(s_socket, entity + j, entity_length - j)) > 0; j += t);

write_entity_buffer(entity_buffer, entity,pos_iniziale, entity_lenght); //devo scrivere quanti byte ci sono, non sempre range

}


//se arrivo alla fine del file allora lo salvo in file locale chiamato: fileSaved
//DICHIARARE fileSaved
char fileSaved[1000000]; 
strcpy(fileSaved, "fileSaved");


FILE *f=fopen(fileSaved, "w+");
if(f!=NULL){
    fwrite(entity_buffer, 1, total, f); 

}
fclose(f); 



//-------------------FINE PARTE NOUVA-------------------



// se la Content-Length NON è presente assegno il valore 1000000 alla entity_length
if(entity_length == -1) entity_length = 1000000;

entity = (char * ) malloc(entity_length);

for(j = 0; (t = read(s_socket, entity + j, entity_length - j)) > 0; j += t);
//if ( t == -1) { perror("Read fallita"); return 1;}
printf("j= %d\n", j);

for(i = 0; i < j; i++) printf("%c", entity[i]);

}



/*int offset = 0;
int total_size = -1;
int chunk = 10000;

while (total_size == -1 || offset < total_size) {
    // 1. Riconnetti (gestisce rete non affidabile)
    s_socket = socket(...);
    connect(s_socket, ...);

    // 2. Richiedi il prossimo segmento
    sprintf(request,
        "GET /file HTTP/1.1\r\nHost:...\r\n"
        "Range: bytes %d-%d\r\n\r\n",
        offset, offset + chunk - 1);
    write(s_socket, request, strlen(request));

    // 3. Leggi headers, estrai Content-Range e Content-Length
    // Content-Range: bytes 0-9999/678900
    //   → sscanf(h[i].v, " bytes %d-%d/%d",
    //             &start, &end, &total_size);

    // 4. Leggi body e salvalo nella posizione giusta
    int bytes_read = 0;
    while (bytes_read < content_length) {
        int n = read(s_socket, entity_buffer + offset + bytes_read,
                     content_length - bytes_read);
        if (n <= 0) break; // connessione persa: il while esterno riprova
        bytes_read += n;
    }
    offset += bytes_read;
    close(s_socket);
}

// Salva il file completo
FILE *f = fopen("fileSaved", "wb");
fwrite(entity_buffer, 1, total_size, f);
fclose(f);*/