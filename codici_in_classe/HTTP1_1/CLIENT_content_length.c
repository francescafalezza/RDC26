#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int tmp;

struct header{ //viene definita una struttura per memorizzare gli header HTTP ricevuti dal server, 
                //separando nome e valore
	char * n;
	char * v;
}h[100];n//permette di indicizzare fino a 100 intestazioni.

int main()
{
	char * statusline;
	struct sockaddr_in addr;
	int i,j,k,s,t;
	char request[5000],response[1000000];

    //viene configurato l'indirizzo di destinazione
	unsigned char targetip[4] = { 216, 58 ,213,100 };

	//unsigned char targetip[4] = { 213,92,16,101 };

	s =  socket(AF_INET, SOCK_STREAM, 0);   //viene creato un socket, cioè l'interfaccia tra 
	if ( s == -1 ){                         //il processo applicativo e il protocollo. Rappresenta l'end point di comunicazione bidirezionale in una rete.
		tmp=errno;                             //AF_INET, indica l'uso di IPv4, SOCK_STREAM specifica l'uso di TCP
                                                //garantendo flusso dati affidabile 
                                                // ritorna un file descriptor
		perror("Socket fallita");
		printf("i=%d errno=%d\n",i,tmp);
		return 1;
	}
	addr.sin_family = AF_INET;

    //Viene configurato la porta 80, porta standard per il traffico web.
    //htons converte il num della porta nel formato Big Endian (oridne dei byte è indipendente dall'archittetura del computer)
	addr.sin_port = htons(80);
	addr.sin_addr.s_addr = *(unsigned int*)targetip; // <indirizzo ip del server 216.58.213.100 >

	/*La system call connect avvia l'operazione di Three-Way_handshake per stabilire connessione con il server 
    se la funzione ha successo il sistema operativo riserva un canale bidirezonale attraverso il quale
    il client può iniziare a parlare linguaggio del protocollo HTTP*/
    
	if ( -1 == connect(s,(struct sockaddr *)&addr, sizeof(struct sockaddr_in)))
		perror("Connect fallita");

	printf("%d\n",s);

    /*Viene preparata una richiesta HTTP GET
    La sequenza \r\n\r\n è fondamentale per lo standard http, indica al server che la sezione degli header è terminata*/
	sprintf(request,"GET / HTTP/1.0\r\n\r\n");

	if ( -1 == write(s,request,strlen(request))){
        perror("write fallita"); 
        return 1;
        }

	statusline = h[0].n=response;

    /*Il codice legge un byte alla volta, così da identificare i teminatori di riga
    quando trova un : nome dell'header è finito, segna l'inizio del valore dell'header
    quando trova \r\n conclude ll'header corrente e passa al successivo
    sostituendo i separatori con terminatori stringa 0*/

    /*Il ciclo legge dal socket s un signolo carattere alla volta (1)
    e lo salva nel buffer alla posizione response+j*/

	for( j=0,k=0; read(s,response+j,1);j++){
		if(response[j]==':' && (h[k].v==0) ){
			response[j]=0;
			h[k].v=response+j+1; //assegna a h[k].v l'indirizzo di memoria subito dopo, dove inizia il valore
		}
		else if((response[j]=='\n') && (response[j-1]=='\r') ){ //sezione del'header è finita e sta per iniziare il corpo del messaggio
			response[j-1]=0;
			if(h[k].n[0]==0) 
                break;

			h[++k].n=response+j+1; //se non è una riga vuota incrementa k e imposta il puntatore del nome dell'header successivo all'indirizzo del prossimo carattere nel buffer
		}
	}

	printf("Header risposta:\n");
	printf("Status line = %s\n",statusline);
	for(i=1;i<k;i++)
		printf("%s ----> %s\n",h[i].n, h[i].v);

	//-- gestione header Content-length
	int entitybody_length = -1;

	//verifico se presente Content-length tra header ricevuti
	//eventualmente estraggo la lunghezza del entity body
    /*Passaggio critico perchè il corpo può contenere anche dati binari
    e il client deve sapere quanti byte leggere prima di chiudere la connessione
    
    il ciclo scorre tutti gli header salvati nell'array di strutture h.
    strcmp confronta il nome dell'header corrente con la stringa fissa
    Se trova corrispondeza la funzione atoi prende il valore testuale e lo converte in binario.
    Ci serve per gestire il corpo del mess*/

	for(i=0;i<k;i++){
		if(strcmp(h[i].n,"Content-Length")==0){
			entitybody_length = atoi(h[i].v);
			break;
		}
	}

	printf("\nThe entity body length is  %d bytes \n",entitybody_length);

	//in mancancanza della lunghezza imposto la lunghezza
	//dell'entity body uguale allo dimensione dello spazio libero nel buffer
	entitybody_length = entitybody_length==-1 ? sizeof(response)-j-1 : entitybody_length;

	//-- lettura contenuto entity body

	// creo un puntatore per entity body
	char* entitybody = response+j;

	//consumo entity body della risposta tenendo conto
	// dell'eventuale presenza dell header Content-length
	while((t=read(s,&response[j],entitybody_length))>0){
		j+=t;
		entitybody_length-=t;
	}

	//inserisco terminatore alla fine dell'entity body per poterla trattare come stringa
	response[j] = 0;

	if ( t == -1) { perror("Read fallita"); return 1;}

	printf("%s",entitybody);
	printf("\n");

}