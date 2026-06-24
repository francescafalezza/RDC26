/*Esame di Reti di Calcolatori - 13 Luglio 2021

Si modifichi il programma del web proxy riportato nel file esame.c presente nella vostra cartella personale 
in modo tale che si comporti come segue 

1)	il web proxy, non appena riceverà dal web client una request di una risorsa, effettuerà a sua volta 
molte request al web server, ciascuna delle quali scaricherà un segmento dell’entity body 
della risorsa richiesto di lunghezza pari a 1000 bytes fino a che l’intero l’entity body 
risulterà scaricato (l’ultimo segmento avrà ovviamente una lunghezza ≤ 1000 bytes).

2)	Il web proxy invierà l’intero entity body della risorsa al web client tramite un’unica response, 
riportando così in un unico stream tutti i segmenti scaricati dal server nell’ordine corretto, 
sì che per il web client lo scaricamento a segmenti risulterà completamente trasparente. 

Al fine di implementare la funzione, si faccia riferimento all’header Range dell’HTTP/1.1 definito nella RFC 2616: 
sezioni 14.35, 3.12, 14.16 .


Per la sperimentazione collegarsi con il web client (configurato per utilizzare il proxy modificato) all’URL  http://88.80.187.84/image.jpg 

IMPLEMENTAZIONE: 
arriva la richiesta dal client 
il proxy deve fare molte richieste allo stesso server in cui chiede 1000bytes dell'entity body
il server risponde con header Content-Range: bytes inizio-fine/totale
web proxy aspetta l'arrivo di tutto il body prima di inviarlo al client

Agisco solo se il metodo è GET (senza tunneling):

- mantengo delle variabili globali: 
int offset
int byte_totali
int byte_letti

finchè tutto il file non è stato mandato : 

- proxy manda al server richiesta uguale a quella del client ma aggiunge header Range: bytes=offset-offset+range
    ad ogni richiesta il primo e secondo valore cambiano

- il server risponde con header Content-Range: bytes inizio-fine/totale e Content-Length: valore

- il proxy salva in un file filetosend.png con fread 
    devo salvare il numero di bytes_letti 
    la scrittura nel file deve iniziare da offset e terminare a offset+content_length

    l'offset sarà poi uguale a offset+=bytes_letti (potrebbe non essere uguale a content_length se si perde prima la connessione)



*/

#include <sys/types.h>          /* See NOTES */
#include <signal.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

//
struct hostent *he;
//
struct sockaddr_in local, remote, server;

// Buffer contenenti le request
char request[2000], response[2000], request2[2000], response2[2000];
char entity_buffer[100000]; 
// Puntatori
char * method, *path, *version, *host, *scheme, *resource,*port;

// Struttura contenente gli header
struct headers {
char *n;
char *v;
}h[30];

struct headers h1[30]; 

//variabili globali per gestire il range
int offset=0; 
int bytes_totali = -1; 
int bytes_letti=0; 

int main() {


	// Puntatore al file
	FILE *f;
	char command[100];
	int i,s,t,s2,s3,n,len,c,yes=1,j,k,pid;

    int r, w; //per il secondo parsing degli header della risposta del server

	// chiamata a sistema che apre una comunicazione e restituisce un INT,
	// che è un File Descriptor ovvero l’indice della tabella con tutto ciò
	// che serve per gestire la comunicazione
	// int socket(int domain, int type, int protocol);
	s = socket(AF_INET, SOCK_STREAM, 0);

	if ( s == -1) {

		// Printa testo di errore
		perror("Socket Fallita\n");
		return 1;
	}

	// Indirizzo ipv4 del server che attende la richiesta
	local.sin_family = AF_INET;
	// Port del server
	// host to network short (htons)
	local.sin_port = htons(12101);
	// Puntatore all'array contenente l'indirizzo ip del server
  // Essendo io il serve è zero
	local.sin_addr.s_addr = 0;

	// Indico al sistema operativo che in caso di riavvi ecc
  // Voglio poter riutilizzare la stessa porta usata in precedenza
	setsockopt(s,SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	// Connette il socket indicando il tipo (AF_INET), la porta da usare e in questo caso
  // Indico che sono un serve con addr.sin_addr.s_addr = 0
	t = bind(s, (struct sockaddr *) &local, sizeof(struct sockaddr_in));

	if ( t == -1) {
		// Printa testo di errore
		perror("Bind Fallita \n");
		return 1;
	}
	// Dico al socket che avrà un backlog max di 10 cliente
  // Ovvero max 10 tentativi diversi di accedere alla risorsa
	// mentre sto soddisfando la prima richiesta
	t = listen(s, 10);

	if ( t == -1) {

		// Printa testo di errore
		perror("Listen Fallita \n");
		return 1;
	}

	while( 1 ){

		// "Svuota" il puntatore al file
		f = NULL;
		// Come parametro per la creazione del socket tra parte server ed il client
		// che fa la richiesta isa un protocollo di tipo AF_INET
		remote.sin_family = AF_INET;

		// Lunghezza della struttura che ricevo dal client
		len = sizeof(struct sockaddr_in);
		// crea un socket "figlio" della socket s e scrive nella mia strutturra
    // remote_addr le informazioni del client e collega questo
    // nuovo socket a quello del cliente
		s2 = accept(s, (struct sockaddr *) &remote, (unsigned int *)&len);
		// Crea un nuovo thread per ogni connessione se il fork ritorna 0
		// allora è il processo figlio e prosegu al codice seguente
		// altrimenti è il processo genitore e vai all'inizio del ciclo
		if(fork()) continue;

		if (s2 == -1) {

			// Printa testo di errore
			perror("Accept Fallita\n");
			return 1;
		}

		// Legge i vari header in arrivo dal client
		// e li dispone per nome e valore nella struct header
		j = 0;
		k = 0;
	  h[k].n = request;

		while(read(s2, request + j, 1)){

			if((request[j] == '\n') && (request[j-1] == '\r')) {

				request[j-1] = 0;
				if (h[k].n[0] == 0) break;
				h[++k].n = request + j + 2;

			}
			if (request[j] == ':' && (h[k].v == 0) && k ){

				request[j] = 0;
				h[k].v = request + j + 1;

			}

		j++;

		}

		// Stampa la request
		printf("%s",request);
		// punta all'inizio della request
		method = request;

		// Con questo ciclo appena trova uno spazio tra i caratteri della prima richiesta
    // esce dal ciclo e sostituisce alla spazio il valore zero, così da avere il tipo di richiesta separato
    // dal resto che viene messo in path
		for(i = 0; (i < 2000) && (request[i] != ' '); i++){} request[i] = 0;
		path = request + i + 1;

		// Con questo ciclo appena trova uno spazio tra i caratteri della prima richiesta
    // esce dal ciclo e sostituisce alla spazio il valore zero, così da avere il tipo di richiesta separato
    // dal resto che viene messo in vet
		for(   ;( i < 2000) && (request[i] != ' '); i++){} request[i] = 0;
		version = request + i + 1;

		printf("Method = %s, path = %s , version = %s\n", method, path, version);

		if(!strcmp("GET", method)){
			//  http://www.google.com/path
			scheme = path;
			for(i = 0; path[i] != ':'; i++){} path[i] = 0;
			host = path + i + 3;
			for(i = i + 3; path[i] != '/'; i++){} path[i] = 0;
			resource = path + i + 1;

			printf("Scheme=%s, host=%s, resource = %s\n", scheme, host, resource);
			// Trova l'indirizzo ip del server dove è posizinata la risorsa
			// richiesta. (Se cerco: www.google.com mi restituirà l'indirizzo ip
			// del server di google)
			he = gethostbyname(host);
			//he = gethostbyname("www.example.com");

			// Se non trova l'indirizzo ip del server mi da errore
			if (he == NULL) {

				printf("Gethostbyname Fallita\n");
				return 1;
			}

			// Stampa l'indirizzo ip del server alla quale inoltrare la richiesta
			printf("Server address = %u.%u.%u.%u\n", (unsigned char ) he->h_addr[0],(unsigned char ) he->h_addr[1],(unsigned char ) he->h_addr[2],(unsigned char ) he->h_addr[3]);



			s3 = socket(AF_INET, SOCK_STREAM, 0);

			if(s3 == -1){

				perror("Socket to server fallita");
				return 1;
			}

			// Indirizzo ipv4 del client che fa la richiesta
			server.sin_family = AF_INET;
			// Port del client
			// host to network short (htons)
			server.sin_port = htons(80);
			// Puntatore all'array contenente l'indirizzo ip del server
		 	server.sin_addr.s_addr =* (unsigned int *) he -> h_addr;

			// connect(): è una chiamata a sistema per connettersi al socket riferito dal
			// file descriptor sockfd all’indirizzo specificato da addr. addrlen specifica
			// la lunghezza dell’indirizzo.

            int range=1000; 
            

            //PER CREARE UN FILE SU CUI POTREI SCRIVERCI:

            FILE *filetosend=fopen("filetosend.png", "w"); 

            if(filetosend ==NULL){
                printf("Errore apertura file\n"); 
                sprintf(response2, "HTTP/1.1 404 Not Found\r\n\r\n"); 
                write(s2, response2, strlen(response2)); 
                close(s2);
                continue; 
            }

			// sockaddr è generica
            while(offset<bytes_totali || bytes_totali = -1){ //continue richieste al server 
			// sockaddr_in è dedicata all' IPv4 cioè per essere usata con
			// l'Internet Protocol (TCP o UDP per il trasporto).
			t = connect(s3, (struct sockaddr *)&server, sizeof(struct sockaddr_in));

			if(t == -1){

				// Printa testo di errore
				perror("Connect to server fallita");
				return 1;
			}


			sprintf(request2, "GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\nRange: bytes=%d-%d\r\n", resource, host, offset, offset+range);

            //proxy riceve risposta dal server e fa parsing degli header. 
            //devo salvare Content-Range e Content-Length


			// la write riceve un file descriptor fd (in questo caso corrisponde a s)
			// Un puntatore ad un array di caratteri (request)
			// la dimensione del buffer (o array di carattery) (strlen(request)))
			// se -1 da errore
			write(s3, request2, strlen(request2));


            r = 0;
		    w = 0;
	        h1[w].n = response2;

		    while(read(s3, response2 + r, 1)){

			    if((response2[r] == '\n') && (response2[r-1] == '\r')) {

				    response2[r-1] = 0;
				    if (h1[w].n[0] == 0) break;
				    h1[++w].n = response2 + r + 2;

			    }
			    if (response2[r] == ':' && (h1[w].v == 0) && w ){

				    response2[r] = 0;
				    h1[w].v = response2 + r + 1;

			    }

		    r++;

		    }
            char *content_length; //se avessi voluto salvarlo in int avrei dovuto fare content_length = atoi(&h1[i].v)
            char *content_range; 
            for (int i =0; i<w; i++){
                if(strcmp(h1[i].n, "Content-Length")==0){
                    content_length=h1[i].v; 
                    printf("Valore content-length: %s\n", content_length); 
                }

                if(strcmp(h1[i].n, "Content-Range")==0){
                    content_range=h1[i].v; 
                }
            }
            int pos_iniziale=0; 
            int pos_finale=0; 
            int byte_tot=0; 
            sscanf(content_range, "bytes %d-%d/%d", &pos_iniziale, &pos_finale, &byte_tot); //& perchè devono essere puntatori, quindi indirizzi di memoria

            bytes_totali=byte_tot; //aggiorno valore della dimensione totale della risorsa

            /*Il proxy deve leggere dal socket s3 del server il body della response
            deve leggere e salvare nel buffer entity_body globale usando come posizioni offset e content_length*/

            int byte_letti =0; 
            
            while(byte_letti<content_length){
                int n = read(s3, entity_buffer+offset+byte_letti, content_length-byte_letti); 
                byte_letti+=n; 
            }
            offset+=byte_letti; 



			/*while((t = read(s3, response2, 2000)) != 0) {

				write(s2, response2, t);

			}*/
            

        }

        //quando esce dal while vuol dire che è arrivato tutto il contenuto: il proxy può inviare tutto l'entity_buffer al client
        sprintf(response2, "HTTP/1.1 200 OK\r\nContent-Length: %s\r\n\r\n", content_range); 
        write(s2, repsonse2, strlen(response2)); 

        write(s2, entity_buffer, strlen(entity_buffer)); 

        //
			shutdown(s3, SHUT_RDWR);
			close(s3);
		}
		else if(!strcmp("CONNECT", method)) { // it is a connect  host:port

			host = path;

			for(i = 0; path[i] != ':'; i++){} path[i] = 0;

			port = path + i + 1;

			printf("host:%s, port:%s\n", host, port);
			printf("Connect skipped ...\n");
			// Trova l'indirizzo ip del server dove è posizinata la risorsa
			// richiesta. (Se cerco: www.google.com mi restituirà l'indirizzo ip
			// del server di google)
			he = gethostbyname(host);

			// Se non trova l'indirizzo ip del server mi da errore
			if (he == NULL) {

				// Printa testo di errore
				printf("Gethostbyname Fallita\n");
				return 1;
			}
			// Stampa l'indirizzo ip del server alla quale connettersi
			printf("Connecting to address = %u.%u.%u.%u\n", (unsigned char ) he->h_addr[0],(unsigned char ) he->h_addr[1],(unsigned char ) he->h_addr[2],(unsigned char ) he->h_addr[3]);

			s3 = socket(AF_INET,SOCK_STREAM,0);

			if(s3 == -1){
				// Printa testo di errore
				perror("Socket to server fallita");
				return 1;
			}

			server.sin_family = AF_INET;
			server.sin_port = htons((unsigned short)atoi(port));
		 	server.sin_addr.s_addr =* (unsigned int*) he->h_addr;

			t = connect(s3, (struct sockaddr *)&server, sizeof(struct sockaddr_in));

			if(t==-1){

				// Printa testo di errore
				perror("Connect to server fallita");
				exit(0);
			}

			sprintf(response, "HTTP/1.1 200 Established\r\n\r\n");
			write(s2, response, strlen(response));
				// <==============

			if(!(pid = fork())){ //Child


				while((t = read(s2, request2, 2000)) != 0){

					write(s3, request2, t);
				//printf("CL >>>(%d)%s \n",t,host); //SOLO PER CHECK
					}
				exit(0);
				} else { //Parent

					while((t = read(s3, response2, 2000)) != 0){
						write(s2, response2, t);
						//printf("CL <<<(%d)%s \n",t,host);
					}

				kill(pid, SIGTERM);
				shutdown(s3, SHUT_RDWR);
				close(s3);

				}
			}

			shutdown(s2, SHUT_RDWR);
			close(s2);
			exit(0);

		}
}