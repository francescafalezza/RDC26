#include<stdio.h>
#include<errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

// COPYRIGHT: MATTIA MENEGALE 2024 e LUCA BORDIN
// Supportami su: https://www.mattiamenegale.com/


//programma web server (identico al precedente) ma che implementa il SERVER come GATEWAY APPLICATIVO
//permette di eseuguire sulla shell del SERVER il comando passato dalla request del CLIENT, e restituisce l'output al CLIENT
//per eseguire i comandi nella SHELL del SERVER, devo passare il path "/exec" nell URI della request dal CLIENT
//NB: utilizziamo l'URI e non l'URL perchè NON ci serve specificare alcun PARAMETRO (vedi appunti)
//ho messo il path "/home/2008369/output" per poter creare e accedere al file "output" all'interno della mia directory di putty, dove reindirizzerò
//l'output del comando eseguito da SHELL del server (ovvero, se non specifico il path, che viene eseguito nella stessa directory in cui è eseguito questo programma (il web sever)

//esempio di utilizzo: 
//1-eseguo il programma da terminale
//2-da browser cerco: "http://88.80.187.84:8077/exec/ls" o semplicemente "88.80.187.84:8077/exec/ls" (entrambi sono URI corretti)

// Definizione delle variabili globali. 
//Con le variabili globali, basta una istanza perchè ogni processo figlio ne abbia una copia
char command[1000];
char hbuf[10000];
char entity[1000];

// Struttura per gli header HTTP
struct headers {
    char *n; // Nome dell'header
    char *v; // Valore dell'header
} h[100]; // Array di 100 headers

// Strutture per l'indirizzo del server e del client remoto
struct sockaddr_in srvaddr, remote;

int main() {
    FILE *fin; // Puntatore al file
    char *method, *filename, *ver; // Puntatori per metodo, nome del file e versione HTTP
    char request[3001]; // Buffer per la richiesta
    char response[3001]; // Buffer per la risposta
    int s, t, s2, len; // Variabili per i socket e la lunghezza degli indirizzi
    int yes = 1; // Variabile per l'opzione del socket
    char *commandline; // Puntatore alla linea di comando
    int i, j; // Variabili di ciclo

    // Creazione del socket TCP (SOCK_STREAM) su IPv4 (AF_INET)
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        perror("Socket fallita");
        return 1;
    }

    // Configurazione del socket per riutilizzare l'indirizzo
    t = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (t == -1) {
        perror("setsockopt fallita");
        return 1;
    }

    // Configurazione dell'indirizzo del server. 
    //ASSOCIA IL SOCKET A UN INDIRZZO E UNA PROTA CONCRETA DELLA MACCHINA
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(8369); // Porta 8077
    srvaddr.sin_addr.s_addr = INADDR_ANY; // Ascolta su tutti gli indirizzi disponibili IP del server

    // Binding del socket all'indirizzo
    t = bind(s, (struct sockaddr *)&srvaddr, sizeof(struct sockaddr_in));
    if (t == -1) {
        perror("Bind fallita");
	    close(s); //NB: serve per risolvere il problema della bind fallita!!! e poter riutilizzare il socket
        return 1;
    }

    // Impostazione del socket in modalità ascolto
    t = listen(s, 5);
    if (t == -1) {
        perror("Listen fallita");
	    close(s);//NB: serve per poter risolvere il problema della bind fallita!!! e poter riutilizzare il socket 
        return 1;
    }

    len = sizeof(struct sockaddr);

    //rimani in ascolto di connessioni
    while (1) {
        close(s2);
	//s2 = socket del client
        s2 = accept(s, (struct sockaddr *) &remote, &len); // Accetta connessioni in ingresso

        // Fork per gestire ogni connessione in un processo figlio
        /*Fork duplica il processo, nel padre restituisce il PID del filgio (>0)
        nel filgio restituisce 0, e prosegue a gestire la connessione*/
        if (fork()) continue;

        if (s2 == -1) {
            perror("Accept fallita");
            return 1;
        }

        // Lettura degli header HTTP
        commandline = h[0].n = hbuf;
        for (j = 0, i = 0; read(s2, hbuf + i, 1); i++) {
            if ((hbuf[i] == ':') && (h[j].v == NULL)) {
                h[j].v = &hbuf[i + 1]; //fa puntare .v al valore che segue
                hbuf[i] = 0; //per terminare il nome dell'header
            }
            if (hbuf[i] == '\n' && hbuf[i - 1] == '\r') {
                hbuf[i - 1] = 0;
                if (h[j].n[0] == 0) break; //riga vuota: fine degli header. Nel ciclo prima la \r era stata sostiruita con 0 quindi il campo nome è nullo
               
                /*Accedi alla struttura all'indice appena incrementato e selezona il campo .n
                &hbuff è l'indirizzo del byte di memoria il cui valore di trova in hbuff[i+1]
                Risultato: h[j] punta al primo carattere del nome del rpossimo header
                */
                h[++j].n = &hbuf[i + 1];
            }
        }

        // Stampa degli header letti
        for (i = 0; i < j; i++) {
            printf("%s ----> %s\n", h[i].n, h[i].v);
        }

        // Parsing della request line
        //assegna il puntatore method all'inzia della stringa
        /*Quando trova uno spazio, ho terminato il metodo e lo sostituisce cn 0 (terminatore stringa)*/
        method = commandline;
        for (i = 0; commandline[i] != ' '; i++) {}
        commandline[i] = 0; i = i + 1;

        /*Fa puntare filename all'indirizzo corrente della stringa e cerca il secondo spazio bianco*/
        filename = commandline + i;
        for (; commandline[i] != ' '; i++) {}
        commandline[i] = 0; i = i + 1;

        /*Fa puntare ver all'inizio  della versione HTTP (cioè H)*/
        ver = commandline + i;
        for (; commandline[i] != 0; i++) {}
        commandline[i] = 0; i = i + 1;
        printf("Method = %s, URI = %s, VER = %s \n", method, filename, ver);

	//---- INIZIO PARTE NUOVA ----
	//es: dal client faccio: "http://88.80.187.84:8077/exec/ls" => request = "GET /exec/ls HTTP/1.1\r\nHEADERS\r\n\r\n"  
	//es: commandline = method = "GET\0" => filename="/exec/ls\0", => ver="HTTP/1.1\0"
	
        // Se il path dell'URI inizia con "/exec/" => ESEGUI I COMANDI sulla SHELL 
        if (strncmp("/exec/", filename, 6)==0) { //se i primi 6 (da 0 a 6 non compreso) caratteri del filename

	    //sprintf copia il contenuto della stringa "filename" da dopo "/exec/" (è quel +6) e "/home/2008369/output" in "command"
	    //ad esempio se l'URI è "/exec/ls" command="ls > /home/2008369/output", ovvero il comando ora dice di reindirizzare l'output di ls nel file specificato
	    //NB: il comando "ls" viene eseguito dalla directory corrente (dove sto eseguendo) del processo del server web
            sprintf(command, "%s > /home/2008369/output", filename + 6);

	    //es: command = "ls > /home/2008369/output" //comando che ti dice di printare sul file "output" il risultato del comando "ls" eseguito sulla directory corrente
            //stampa su terminale
	    printf("eseguo comando %s\n", command);
            //system: funzione che esegue il comando "command" su SHELL del SERVER, restitituisce 0 se il comando non va a buon fine, altrimenti un intero che identifica il comando nella shell
	    system(command);
	    //copia la stringa "/output" in "filename"
	    // così (fuori da questo if) il programma, invece di aprire il file indicato dall'URI originale, 
	    // cercherà di aprire il file "/output", che contiene il risultato dell'esecuzione di "command" e lo stamperà nella response 
            strcpy(filename, "/output");
        }
	//---- FINE PARTE NUOVA ---
        
	// Apertura del file richiesto, ovvero di "/output"
	// e stampa del suo contenuto nella response inviata al client
        fin = fopen(filename + 1, "rt");
        if (fin == NULL) {
            // Se il file non esiste, invia una risposta 404
            sprintf(response, "HTTP/1.1 404 NOT FOUND\r\nConnection:close\r\n\r\n<html><h1>File %s non trovato</h1></html>", filename);
            write(s2, response, strlen(response));
            close(s2);
            exit(1);
        }

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
    }
}