//classiche
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define __USE_XOPEN

#include <netdb.h>
#include <time.h>
#include <fcntl.h>

char hbuf[100000];  //header buff
char bbuf[1000];	//body buff
int t;
//int chunked = 0;

struct hostent* he;
char* date;
char downloaddate[100];
char lastmodifieddate[100];
struct tm tmdl, tmlm;

struct headers
{
	char* n;
	char* v;
} h[100];

/*Funzione che legge dati dal socket e taglia la stringa degli header per metterli in h
Riceve puntatore al socket e lunghezza del body*/
void  HeaderLabel(int* ps, int* len)
{
	printf("---Header------Client---  \n");
	int i, j;
	for (i = 0, j = 0; t = read(*ps, hbuf + i, 1); i++)
	{
		//condizioni per spezzare i vari header: termina nome dell'header 
        //e preprara l'inizio del valore
		if ((hbuf[i] == ':') && (h[j].v == NULL))
		{
			h[j].v = &hbuf[i + 1];
			hbuf[i] = 0;
		}
		if (hbuf[i] == '\n' && hbuf[i - 1] == '\r') //termina riga delll'header
		{
			hbuf[i - 1] = 0;

			if (h[j].n[0] == 0) 
                break; //se trova riga vuota gli header sono finiti

			h[++j].n = &hbuf[i + 1]; //passa all'header successivo
		}
	}
	printf("numero header: %d\n", j);
	//stampa tabella, ciclo epr analizzare header salvati
	for (i = 0; i < j; i++)
	{
		printf("%s --> %s\n", h[i].n, h[i].v);
		//Se l'header è la lunghezza del file lo converte in intero
		if (!strcmp(h[i].n, "Content-Length"))
		{
			*len = atoi(h[i].v);
		}
		if (!strcmp(h[i].n, "Date"))
		{
			//printf("QUESTA E LA DATE:%s\n", h[i].v);
			date = (h[i].v);
			strcpy(downloaddate, date + 1);  //funzione per copiare il contenuto di una stringa in unl'altra

            //char *strcpy(char *destinazione, const char *sorgente);
		}
        //estrae data di ultima modifica della risorsa dal server
		if (!strcmp(h[i].n, "Last-Modified"))
		{
            //POSSIBILE ERRORE: salta due spazi.
			date = (h[i].v+1); //sposta puntantore per saltare lo spazio
			strcpy(lastmodifieddate, date + 1);
		}

	}
	printf("fine header client\n\n");
}

int main()
{
	
	char* statusline;
	int s, i, j;
	int len = 0;
	char size[4];
	char response[100000];
	struct sockaddr_in server;
	char* p;
	
	char filename[1500];
	char url[1000];
	char hostname[1000];
	int fd;
	//char date[1000];

    //Crea un socket TCP
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1){perror("socket fallita");return 1;}
	
	


	//REQUEST
	//FullRequest  = Request-Line  *Header         CRLF   [Entity-Body]
	//Request-Line = Method         Request-URI    SP     HTTP-Version    CRLF
	
	sprintf(hostname, "example.com"); //imposta sito da contattare
	//sprintf(url, "");
	//sprintf(request, "GET www.example.com HTTP/1.1\r\n Last-Modified:%s\r\n\r\n", date);
	
	he = gethostbyname(hostname); //risolve nome host in indirizzo IP, restituisce una struttura con informazioni sull'host

	//p = (unsigned char*)&server.sin_addr.s_addr;

    //Configura indirizzo del server con cui stabilire connessione, usando i dati ottenuti da gethostbyname
	server.sin_family = AF_INET;
	server.sin_port = htons(80);
	server.sin_addr.s_addr = *(unsigned int*)(he->h_addr);

    //tenta di stabilire connessione con il server usando la system call connect, se fallisce stampa errore e termina programma
	t = connect(s, (struct sockaddr*)&server, sizeof(struct sockaddr_in));
	if (t == -1) { perror("Connect fallita"); return 1; }

	char request[3000];
	sprintf (request,"GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n",url, hostname); //manca roba
	printf("%s",request);

    //invia la richiesta al server usando la system call write, (in più: se fallisce stampa errore e termina programma)
	write(s, request, strlen(request));

	//RESPONSE
	//FullResponse = Status-Line   *Header			CRLF		[Entity-Body]
	//Status-Line  = HTTP-Version   SP Status-Code	SP			Reason-Pharse   CRLF
	//Chunked-Body = *chunk			last-chunk		trailer		CRLF
	//Chunk		   = chunk-size		CRLF			chunk-data	CRLF
	
	
	for (i = 0; url[i]; i++)
	{
		if (url[i] == '/')
			url[i] = '_';
	}
	if (url != NULL)
		sprintf(filename, "cache/_%s.txt", url);
	else
		sprintf(filename, "cache/_.txt");
	printf("%s\n", filename);


	
	h[0].n = hbuf;
	HeaderLabel(&s, &len);
	fd = open(filename, O_RDONLY);
	if (fd == -1) perror("file non esistente");

    //se il file esiste, legge la prima riga, che contiene la data di download
	else
	{
		read(fd, downloaddate, 100);
		for (i = 0; downloaddate[i] != '\n'; i++);
		downloaddate[i] = 0;
	}

	//calcolo secondi
	strcpy(downloaddate, date); //creo una stringa
	printf("%s\n", downloaddate); //stampo la stringa	
	strptime(downloaddate, "%a, %d %b %Y %H:%M:%S %Z", &tmdl); //spezzo la stringa in vari valori
	/*
	printf("tm_year%d\n", tm.tm_year);
	printf("tm_month%d\n", tm.tm_mon);
	printf("tm_day%d\n", tm.tm_mday);
	printf("tm_hour%d\n", tm.tm_hour);
	printf("tm_min%d\n", tm.tm_min);
	printf("tm_sec%d\n", tm.tm_sec);
	*/
	
	if (mktime(&tmdl) - mktime(&tmlm) > 0)
	{
		printf("il file e gia stato scaricato\n");
	}
	//printf("secondi %lu\n", (unsigned long)time(NULL));
	
	
	
	//creazione stringa da aggiungere al file
	char downloadtime[100];
	sprintf(downloadtime,"%lu\n",mktime(&tmdl));
	printf("%s\n", downloadtime);


	
	
	
	//Logica chunked transfer encoding
	if (len == 0)
	{
		while (1)
		{
			//carico size e l'extension
			for (i = 0; read(s, bbuf + i, 1); i++)
			{
				if (*(bbuf + i) == '\n')
				{
					bbuf[i] = 0;
					printf("bbuf = %s\n", bbuf);
					break;
				}
			}
			//metto dentro size la dimensione del chunk lasciando bbuf quasi invariato
			for (i = 0; !((*(bbuf + i) == ';') || *(bbuf + i) == '\r'); i++)
			{
				size[i] = bbuf[i];
			}
			//carattere terminatore di size
			size[i] = 0;
			//conversione in decimale + \r\n
			len = (int)strtol(size, NULL, 16) + 2;

			for (i = 0; t = read(s, response + i, len - i); i += t);
			response[i] = 0;
			printf("response: %s\n", response);
			if (len == 2) break;
		}
	}

    //logica content-length
	else
	{
		//crea o apre file di cache in scrittura 
		fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
		//fopen(filename, 'w');
		if (fd == -1) perror("file non creato\n");

		for (i = 0; t = read(s, response + i, len - i); i += t);
		response[i] = 0;
		write(fd, downloadtime, strlen(downloadtime));
		write(fd, response, strlen(response));
		close(fd);
	}
	
	printf("trailer\n");
	
}