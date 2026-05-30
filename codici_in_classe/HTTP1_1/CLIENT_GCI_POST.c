
/*CODICE LUNGO PER NIENTE, CON DIVERSI ERRORI*/

#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// COPYRIGHT: MATTIA MENEGALE 2024
// Supportami su: https://www.mattiamenegale.com/

/*
 *  La pagina consentirà all’utente, per tramite dell’User Agent (browser),
 *  di inserire il nome di un comando di shell UNIX da eseguire (per esempio ls) e
 *  uno o due parametri (per esempio -l).
 *  Alla pressione del bottone “Invia”, il browser invierà al Web server una
 *  HTTP-request della risorsa /cgi- bin/command con il metodo POST contenente nel
suo Entity Body il nome del comando e i due parametri secondo il medesimo formato
(detto urlencoded) utilizzato nelle query string degli URL (v. RFC 1866 Cap. 8.2).
Il Web Server ricevendo la richiesta alla risorsa /cgi-bin/command dovrà eseguire
il comando specificato con i parametri e riportare l’output di quel comando come
Entity Body della HTTP-response.

 * Mattia Menegale  (laureando)
 * 1. Funzionalità da aggiungere:
 * - parsing della reqline della POST: se c'è /cgi-bin/command => esegui comando su shell
 * - read dell'entity body della POST (usando l'header "Content-Length") 
 * - parsing del comando e dei 2 parametri dall'entity body
 * - esecuzione del comando sulla shell con parametri specificati e reindirizzamento dell'output al file /home/2008369/output
 * - invio dell'output sulla response
 *
 * 2. Punti di intervento neĺ programma:
 * - dopo aver fatto il parsing degli headers, verifico se la status line corrisponde a una POST con /cgi-bin/command come URL
 * - da qui in avanti va inserita la parte nuova
 *
 * 3. Eventuali scelte implementative:
 * - ho scelto di utilizzare strstr per velocizzare il processo di sviluppo del parsing dell'url (comando, param1 e param2)
 * - ho scelto di testare con il comando ls senza parametri (non mi venivano in mente comandi con i parametri che mi avrebbero dato un output verificabile)
 *   Ad ogni modo ho implementato anche la versione con i parametri, basta solo commentare e de commentare 2 righe (scritto tutto nel codice sotto)
 * 4. Descrizione dell'esperimento
 * - Si tratta si sfruttare il cgi che è uno standard di gateway interface per fare eseguire a un sw uno script/un comando richiesto dal client e mandare l'output al client
 * - sfruttiamo una post e perciò ci aspettiamo i parametri passati nell'entity body
 * - si può riutilizzare parte del codice di sw-gateway per la stampa dell'output come response inviata al client
 * 5. Descrizione dell'esito e verifica correttezza
 * - ho testato il programma solo con il comando "ls" con i parametri "1" e "2" che tuttavia non ho inserito nel comando perchè ls non richiede parametri.
 *   Il programa gestisce correttamente la POST e il parsing, esegue il comando ls sulla directory dell'utente e reindirizza l'output a /output (file creato).
 *   Il contenuto di tale file viene poi passato all'utente tramite il meccanismo già implementato (senza dover scrivere altro codice) (forse ho fatto un ctr+c ctrl+v di codice già presente sotto
 *   perchè non avevo tempo di stare a pensare come renderlo più efficiente, ma teoricamente si può.
 */

struct sockaddr_in local, remote;
char request[100000];
char response[1000];

struct header {
  char * n;
  char * v;
} h[100];

unsigned char  envbuf[1000];  //buffer per le variabili d'ambiente
int pid;
int env_i, env_c;
char * env[100];
int new_stdin, new_stdout;
char * myargv[10];


/*Funzione helper che costiruisce varibiali d'ambiente nel formato CHIAVE=VALORE e le aggiunge all'array env[]
*/
void add_env(char * env_key, char* env_value){
		sprintf(envbuf+env_c,"%s=%s",env_key,env_value);
		env[env_i++]=envbuf+env_c;
		env_c+=(strlen(env_value)+strlen(env_key)+2);
		env[env_i]=NULL;
}


int main()
{
char hbuffer[10000];
char * reqline;
char * method, *url, *ver;
char * filename,*content_type;
char fullname[200];
FILE * fin;
int c;
int n;
int i,j,t, s,s2;
int yes = 1;
int len;
int length;
if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
	{ printf("errno = %d\n",errno); perror("Socket Fallita"); return -1; }
local.sin_family = AF_INET;
local.sin_port = htons(8369);
local.sin_addr.s_addr = 0;

t= setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
if (t==-1){perror("setsockopt fallita"); return 1;}

if ( -1 == bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in)))
{ perror("Bind Fallita"); return -1;}

if ( -1 == listen(s,10)) { perror("Listen Fallita"); return -1;}

remote.sin_family = AF_INET;
remote.sin_port = htons(0);
remote.sin_addr.s_addr = 0;

len = sizeof(struct sockaddr_in);

while ( 1 ){

    //accetta connessione da client
	s2=accept(s,(struct sockaddr *)&remote,&len);

    //azzera i buffer
bzero(hbuffer,10000);
bzero(h,sizeof(struct header)*100);

reqline = h[0].n = hbuffer; //req line è la prima stringa che viene letta

//parsing degli header
for (i=0,j=0; read(s2,hbuffer+i,1); i++) {
  if(hbuffer[i]=='\n' && hbuffer[i-1]=='\r'){
    hbuffer[i-1]=0; // Termino il token attuale
   if (!h[j].n[0]) break;
   h[++j].n=hbuffer+i+1;
  }
  if (hbuffer[i]==':' && !h[j].v){
    hbuffer[i]=0;
    h[j].v = hbuffer + i + 1;
  }
 }

 /*Scorre header salvati e se trova Content-Length lo converte in intero
 
 Se trova Content-Type lo mette nelle variabili d'ambiente per la CGI*/
length=0;
for(i=0;i<j;i++){
	printf("%s ---> %s\n",h[i].n,h[i].v);
	if(!strcmp(h[i].n,"Content-Length")){
		length=atoi(h[i].v);
		}
	
	if(!strcmp(h[i].n,"Content-Type")){
		add_env("CONTENT_TYPE",h[i].v+1);
	}
	}


	len=1000;
	printf("%s\n",reqline);
	if(len == -1) { perror("Read Fallita"); return -1;}

    //PARSING DELLA REQUEST LINE: METHOD, URL, VERSION
	method = reqline;
	len=1000;
	for(i=0;i<len && reqline[i]!=' ';i++); reqline[i++]=0; 
	url=reqline+i;
	for(;i<len && reqline[i]!=' ';i++); reqline[i++]=0; 
	ver=reqline+i;
	for(;i<len && reqline[i]!='\r';i++); reqline[i++]=0; 

	add_env("METHOD",method);

	filename = url+1; //filename punta all'url senza il primo carattere /


	// Se l'uri inizia con "/cgi-bin/command" allora devo eseguire un comando da shell 
	if (!strncmp(url,"/cgi-bin/command", 16)){ //CGI
		if (!strcmp(method,"POST")){
		
		//DEGUB: stampo:
		printf("DEBUG: sto gestendo %s con url = %s e Content-Length: %d\n", method, url, length); 
		
		//Read dell'entity body della POST: so esattamente quanti byte leggere grazie a Content Length
		char entity_body[1000];
		for(i=0;i<length && (t=read(s2,entity_body+i,length-i));i+=t);
		entity_body[length]='\0';//metto a carattere terminatore

		//DEBUG: stampo entity body
  		printf("DEBUG: the Entity Body received is:\n%s\n", entity_body);
		
		//Parsing di comando e di param1 e param2 a partire dall'entity_body
		//Potrei lavorare con i cicli for come sopra 
		char *comando;
		char *param1;
		char *param2;

		//es: entity_body="commando=ls&param1=1&param2=2\0"
		// faccio puntare comando, param1 e param2 all'inizio delle parti corrette
		comando = entity_body + 9; // salto "commando="	=> comando = "ls&param1=1&param2=2\0"
		param1 = strstr(entity_body, "param1=") + 7; // salto "param1" => param1 = "1&param2=2\0"
		param2 = strstr(entity_body, "param2=") + 7; // salto "param2" => param2 = "2\0"

		//sostituisco in entity_body gli '&' con '\0' in modo che dopo questi for:  comando= "ls\0", param1= "1\0" e param2= "2\0"
		for(i=0; entity_body[i]!= '&'; i++);
		entity_body[i++] = '\0';
		for(; entity_body[i]!= '&'; i++);
		entity_body[i++] = '\0';

		//DEBUG:
		printf("DEBUG: comando= %s, param1= %s, param2= %s\n", comando, param1, param2); // Output = "DEBUG: comando= ls, param1= 1, param2= 2"

		//Esecuzione del comando. Con reindirizzamento dell'output su file
		printf("eseguo comando %s\n", comando);

           	//system: funzione che esegue il comando "comando" su SHELL del SERVER, restitituisce 0 se il comando non va a buon fine, altrimenti un intero che identifica il comando nella shell
	   	sprintf(comando, "%s > /home/2008369/output", comando);
		
		//Per aggiungere i parametri basta commentare la funzione sopra e decommentare questa sotto, il punto è che se metto i parametri non posso testare il comando ls
		//sprintf(comando, "%s %s %s > /home/2008369/output", comando, param1, param2);

		printf("eseguo comando %s\n", comando);

		system(comando);
	   	//copia la stringa "/output" in "filename"
	  	// così (fuori da questo if) il programma, invece di aprire il file indicato dall'URI originale,
	   	// cercherà di aprire il file "/output", che contiene il risultato dell'esecuzione di "command" e lo stamperà nella response
           	strcpy(filename, "/output");
			




		//Restituzione dell'output al client sotto forma di response 
		//Riutilizzo filename con il path del file di output
		

		//DEBUG:
		printf("DEBUG: filename= %s\n", filename);

		fin = fopen(filename + 1, "rt");
        	if (fin == NULL) {
            	// Se il file non esiste, invia una risposta 404
           	 sprintf(response, "HTTP/1.1 404 NOT FOUND\r\nConnection:close\r\n\r\n<html><h1>File %s non trovato</h1></html>", filename);
           	 write(s2, response, strlen(response));
           	 close(s2);
           	 exit(1);
        	}
			

		char entity[10000];
        	// Invio della risposta HTTP 200 OK e il contenuto del file
        	sprintf(response, "HTTP/1.1 200 OK\r\nConnection:close\r\n\r\n<html>");
        	write(s2, response, strlen(response));
        	while (!feof(fin)) {
            	fread(entity, 1, 1000, fin);
            	write(s2, entity, 1000);
       		 }
        	fclose(fin);
       		 close(s2);
        	exit(0);

		}else { //Altrimenti se ho altro (ad esempio una GET) => non implementato
			sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
    	write(s2,response,strlen(response));
			close(s2);
			continue;
		}

		// Apro il file
		fin=fopen(filename,"rt");
		if (fin == NULL){
			sprintf(response,"HTTP/1.1 404 Not Found\r\n\r\n");
			write(s2,response,strlen(response));
		}
		else{ 
			sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
			write(s2,response,strlen(response));
			fclose(fin);
			for(i=0;env[i];i++)
				printf("environment: %s\n",env[i]);
			sprintf(fullname,"/home/2008369/%s",filename);
			myargv[0]=fullname;

			myargv[1]=NULL;
			printf("Executing %s\n",fullname);
			if(!(pid=fork())){ 
									dup2(s2,1);
									dup2(s2,0);
									if(-1==execve(fullname,myargv,env))
											{ perror("execve"); exit(1);}
										
							}
			waitpid(pid,NULL,0);
			printf("Il processo figlio e' terminato...\n");
			}

	// Se invece l'uri inizia con "/cgi/" esegui uno script CGI: cerco ? nell'url per separare il nome del file dalla query string (paramentri dopo ?)
	}else if (!strncmp(url,"/cgi/",5)){ //CGI
		filename=url+5;
		if (!strcmp(method,"GET")){
	  	for(i=0;filename[i] && (filename[i]!='?');i++);
			if(filename[i]=='?'){ 
				filename[i]=0;
				add_env("QUERY_STRING",filename+i+1);
			}
			add_env("CONTENT_LENGTH","0");
		}
		else if(!strcmp(method,"POST")){
			char tmp[10];
			sprintf(tmp,"%d",length);
			add_env("CONTENT_LENGTH",tmp);
		}
		else {
			sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
    	write(s2,response,strlen(response));
			close(s2);
			continue;
		}
		fin=fopen(filename,"rt");
		if (fin == NULL){
			sprintf(response,"HTTP/1.1 404 Not Found\r\n\r\n");
			write(s2,response,strlen(response));
		}
		else{ 
			sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
			write(s2,response,strlen(response));
			fclose(fin);
			for(i=0;env[i];i++)
				printf("environment: %s\n",env[i]);
			sprintf(fullname,"/home/utente/%s",filename);
			myargv[0]=fullname;
			myargv[1]=NULL;
			printf("Executing %s\n",fullname);
			if(!(pid=fork())){ 
									dup2(s2,1);
									dup2(s2,0);
									if(-1==execve(fullname,myargv,env))
											{ perror("execve"); exit(1);}
										
							}
			waitpid(pid,NULL,0);
			printf("Il processo figlio e' terminato...\n");
			}

	}	

	// Se non ho "cgi-bin/command" allora ritorna una risorsa
	else if ( !strcmp(method,"GET")){ //NOT CGI
		filename = url+1;
		printf("filename: %s\n",filename);
		fin=fopen(filename,"rt");
		if (fin == NULL){
			sprintf(response,"HTTP/1.1 404 Not Found\r\n\r\n");
			write(s2,response,strlen(response));
			}
		else{ 
			sprintf(response,"HTTP/1.1 200 OK\r\n\r\n");
			write(s2,response,strlen(response));
			while ( (c = fgetc(fin))!=EOF) write(s2,&c,1);
			fclose(fin);
			}
	}
else {
			sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
    	write(s2,response,strlen(response));
	}
	close(s2);
	env_c=env_i=0;
}
close(s);
}