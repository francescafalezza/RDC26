/*Universita' degli Studi di Padova

               Dipartimento di Ingengeria Informatica

            Prof. Nicola Zingirian Prof. Federico Botti

              PROVA DI LABORATORIO RETI DI CALCOLATORI

                          04 FEBBRAIO 2026


Modificare il programma web client HTTP/1.1 <matricola>.c che si trova
nella vostra home in modo che si comporti come segue. Il client deve:

1.  inviare una richiesta verso una risorsa HTTP/1.1
2.  determinare la lunghezza totale dell’entity body
3.  verificare il supporto alle range requests tramite Accept-Ranges
    (RFC 2616, 14.5).
4.  suddividere logicamente l’entity body in tre intervalli di byte
    consecutivi che coprano l’intero contenuto.
5.  inviare tre richieste GET consecutive con header Range (RFC 2616,
    14.35), ciascuna per uno degli intervalli calcolati.
6.  interpretare le risposte alle richieste parziali, includendo lo
    status 206 (RFC 2616, 10.2.7) e l’header Content-Range (RFC 2616,
    14.16).
7.  ricostruire localmente l’entity body completo a partire dalle tre
    porzioni ricevute.


Effettuare la richiesta a http://www.example.com/ per fare il test del
vostro client (il client è già predisposto per collegarsi a quel sito).

Output minimo richiesto stampato su console:

  a)  lunghezza dell'entity body rilevato
  b)  presenza/assenza di Accept-Ranges
  c)  intervalli richiesti (Range)
  d)  status code ricevuti
  e)  dimensione finale del contenuto ricostruito
  f)  stampa del contenuto aggregato*/


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

//buffer per salvare le risposte parziali
char risposte_parziali[10000]; 

int main() {

  // chiamata a sistema che apre una comunicazione e restituisce un INT,
  // che è un File Descriptor ovvero l’indice della tabella con tutto ciò
  // che serve per gestire la comunicazione
  // int socket(int domain, int type, int protocol);
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

struct hostent *he; 
char hostname[100]; 
sprintf(hostname, "www.example.com"); 

he= gethostbyname(hostname);
addr.sin_addr.s_addr = *(unsigned int*)he->h_addr_list[0];

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

sprintf(request, "HEAD / HTTP/1.1\r\nHost:www.example.com\r\n\r\n");

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

printf("Status line = %s\n",statusline);

for(i = 1; i < k; i++) {

  // verifico se la Content-Length è presente o meno nello header
  if(strcmp(h[i].n, "Content-Length") == 0) {

    // se la Content-Length è presente assegno il valore della lunghezza della Content-Length
    // atoi() converte ina stringa (o un puntatore ad una stringa) in intero
    entity_length = atoi(h[i].v);

    // Stampo la lunghezza della Content-Length che corrisponde alla entity_length
    printf("* (%d) ",entity_length);

  }

  printf("%s ----> %s\n",h[i].n, h[i].v);

}


char *content_length;
char *range_accettati;
int content_length_int =0;
char accept_ranges [10];

for (int i=0; i<k; i++){
         if(strcmp(h[i].n, "Content-Length")==0){
                content_length=h[i].v+1;
                printf("Valore content_length: %s\n", content_length);
                sscanf(h[i].v, " %d", &content_length_int);
        }   
}


//Divido il content_length in 3 intervalli
//Client fa 3 richieste con Range:
/*1. bytes=0-fine_primo_intervallo
2. bytes=fine_primo-fine_secondo
3. bytes=fine_secondo-fine_terzo

Le tre richieste vengono fatte con un ciclo for e ogni volta aggiorno il range richiesto.
Aggiornamento range con variabili offset e range1 range2 range3
*/


int range=(int)(content_length_int/3);

int offset=0;

//tre richieste le client
int numero_richieste =0;

//vaariabile per salvare dimensione finale 
int dim_finale =0; 

//IMPORTANTE: o riapro la connessione ad ogni ciclo o devo mettere Connection: keep-alive
while(numero_richieste<3){
    int fine=0; 
    if(numero_richieste==2){
        fine=content_length-1; 
    }
    else{
        fine=offset+range-1; 
    }

    sprintf(request, "GET /index.html HTTP/1.1\r\nHost: www.example.com\r\nConnection: keep-alive\r\nRange: bytes=%d-%d\r\n\r\n", offset, fine);
    write(s_socket, request, strlen(response));

//client riceve risposta  dal server: fa parsing degli header e della status line
// Metto a zero tutti bite nell'header e della response
bzero(h, sizeof(struct header)*100);
bzero(response, sizeof(response)); 

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

char ver[100];
char status_code[100];
char reason_phrase[100];
char *content_range; 
char *lunghezza_body; 
int byte_body=0; 

sscanf(statusline, "%s %s %s", ver, status_code, reason_phrase);

printf("Status line = %s\n", statusline);


//salvo i valori degli header Accept range e Content-range e content-length
for(int a =0; a<k; a++){
     if(strcmp(h[i].n, "Accept-Ranges")==0){   //da portare dopo nelle 3 request con range NON ORA.
                range_accettati=h[i].v+1;
                sscanf(h[i].v, " %s", &accept_ranges[0]);
        }

    if(strcmp(h[i].n, "Content-Range")==0){
        content_range=h[i].v+1; 
        printf("Valore content-range: %s\n", content_range); 
    }

    if(strcmp(h[i].n, "Content-Length")==0){
        lunghezza_body=h[i].v+1; 
        byte_body=atoi(lunghezza_body); 
        printf("Lunghezza del body parziale: %d\n", byte_body); 
    }

}

//leggo il body fino a byte_body e aggiorno offset 
int byte_letti=0; 
while (byte_letti<byte_body){
    int n = read(s_socket, risposta_parziale+offset+byte_letti, byte_body-byte_letti); 
    byte_letti+=n; 
}

printf("Dimensione finale del contenuto: %d\n", byte_letti);
offset+=byte_letti; 

dim_finale+=offset; 

numero_richieste++; 

}

//Stampo tutto il contenuto del body
printf("Stampa del contenuto totale della risposta di dimesione:%d \n", dim_finale); 

for(int count=0; count<dim_finale; count++){
    printf(risposta_parziale[count]); 
}

/*
// se la Content-Length NON è presente assegno il valore 1000000 alla entity_length
if(entity_length == -1) entity_length = 1000000;

entity = (char * ) malloc(entity_length);

for(j = 0; (t = read(s_socket, entity + j, entity_length - j)) > 0; j += t);
//if ( t == -1) { perror("Read fallita"); return 1;}
printf("j= %d\n", j);

for(i = 0; i < j; i++) printf("%c", entity[i]);

}
