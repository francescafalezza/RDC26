#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string.h>

int main() {

    int sockId = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sockId, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8085);
    serverAddr.sin_addr.s_addr = 0; // accetta da tutti

    if (bind(sockId, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Bind Error");
        exit(EXIT_FAILURE);
    }

    if (listen(sockId, 5) == -1) {
        perror("Listen Error");
        exit(EXIT_FAILURE);
    }

    while (1) {

        struct sockaddr_in client_addr;
        int sizeSockAddrClient = sizeof(client_addr);
        int clientSockId = accept(sockId, (struct sockaddr *)&client_addr, &sizeSockAddrClient);

        if (clientSockId == -1) {
            perror("Accept error");
            exit(EXIT_FAILURE);
        }

        // fork: ogni client viene gestito da un processo figlio
        int pid = fork();
        if (pid != 0) {
            // padre: chiude il socket del client e torna ad ascoltare
            close(clientSockId);
            continue;
        }

        // da qui in poi: solo il figlio
        close(sockId);

        // -------------------------------------------------------
        // LETTURA E PARSING DEGLI HEADER
        // -------------------------------------------------------

        char buffer[100000];
        struct header { char *n; char *v; } h[100];

        int header_index = 0;
        int leggoHeaderName = 0;
        h[0].n = buffer;
        h[0].v = NULL;

        int byteLetti = 0, n = 0;
        while ((n = read(clientSockId, buffer + byteLetti, 1)) > 0) {

            // fine degli header: riga vuota \r\n\r\n
            if (byteLetti >= 3 &&
                buffer[byteLetti]     == '\n' &&
                buffer[byteLetti - 1] == '\r' &&
                buffer[byteLetti - 2] == '\n' &&
                buffer[byteLetti - 3] == '\r') {
                buffer[byteLetti - 1] = 0; // termina l'ultimo header
                break;
            }

            // fine di una riga header: \r\n
            if (byteLetti >= 1 &&
                buffer[byteLetti]     == '\n' &&
                buffer[byteLetti - 1] == '\r') {
                buffer[byteLetti - 1] = 0;      // termina la stringa dell'header corrente
                h[++header_index].n = buffer + byteLetti + 1; // nome del prossimo header
                h[header_index].v = NULL;
                leggoHeaderName = 1;
            }

            // separatore nome:valore
            if (leggoHeaderName && buffer[byteLetti] == ':') {
                buffer[byteLetti] = 0;               // termina il nome
                h[header_index].v = buffer + byteLetti + 1; // valore inizia dopo
                leggoHeaderName = 0;
            }

            byteLetti += n;
        }

        // -------------------------------------------------------
        // PARSING DELLA REQUEST LINE
        // -------------------------------------------------------

        char *request_line = buffer;
        char method[10], uri[200], http_version[10];
        sscanf(request_line, "%s %s %s", method, uri, http_version);

        printf("method: %s  uri: %s  version: %s\n", method, uri, http_version);

        // stampa tutti gli header ricevuti
        for (int i = 1; i <= header_index; i++)
            printf("  %s:%s\n", h[i].n, h[i].v);

        // -------------------------------------------------------
        // LEGGI Content-Length (serve per la POST)
        // -------------------------------------------------------

        int content_length = 0;
        for (int i = 1; i <= header_index; i++) {
            if (strcmp(h[i].n, "Content-Length") == 0)
                content_length = atoi(h[i].v + 1); // +1 salta lo spazio dopo ":"
        }

        // -------------------------------------------------------
        // ROUTING: CGI o file statico?
        // -------------------------------------------------------

        char response[1000];

        if (strncmp(uri, "/cgi-bin/", 9) == 0) {

            // -------------------------------------------------------
            // RAMO CGI
            // -------------------------------------------------------

            // Separa il filename dalla query string (per la GET)
            // uri = "/cgi-bin/script.py?nome=Mario&eta=25"
            char filename[200];
            char query_string[1000] = "";

            strcpy(filename, uri + 9); // salta "/cgi-bin/" → "script.py?nome=Mario"

            // cerca il ? per separare filename da query string
            for (int i = 0; filename[i]; i++) {
                if (filename[i] == '?') {
                    filename[i] = 0;                      // taglia: "script.py\0"
                    strcpy(query_string, filename + i + 1); // "nome=Mario&eta=25"
                    break;
                }
            }

            printf("CGI filename: %s\n", filename);
            printf("query string: %s\n", query_string);

            // manda la response line prima del fork
            write(clientSockId, "HTTP/1.1 200 OK\r\n\r\n", 19);

            // fork per eseguire il programma CGI
            int cgi_pid = fork();
            if (cgi_pid == 0) {

                // processo figlio CGI: reindirizza stdin e stdout sul socket
                dup2(clientSockId, 0); // stdin  ← socket (per leggere body POST)
                dup2(clientSockId, 1); // stdout → socket (output va al browser)

                // variabili d'ambiente standard CGI. Variabili d''ambiente vengono ereditatae dal figlio
                setenv("REQUEST_METHOD", method, 1);
                setenv("QUERY_STRING", query_string, 1); // parametri GET

                if (strcmp(method, "POST") == 0) {
                    // per la POST: il body è già sullo stdin (dup2 fatto sopra)
                    // il programma CGI lo leggerà da stdin
                    char cl_str[20];
                    sprintf(cl_str, "%d", content_length);
                    setenv("CONTENT_LENGTH", cl_str, 1);
                }

                // costruisce il path completo del programma
                char fullpath[300];
                sprintf(fullpath, "./cgi-bin/%s", filename);

                char *argv[] = { fullpath, NULL };
                execv(fullpath, argv); // sostituisce il processo con il programma CGI. Il programma scrive su stdout, che è reindirizzato al socket

                // se execv ritorna c'è stato un errore
                perror("execv fallita");
                exit(1);

            } else {
                // aspetta che il CGI finisca
                waitpid(cgi_pid, NULL, 0);
            }

        } else if (strcmp(method, "GET") == 0) {

            // -------------------------------------------------------
            // RAMO FILE STATICO (solo GET)
            // -------------------------------------------------------

            // uri+1 salta il primo '/' → "index.html"
            FILE *f = fopen(uri + 1, "r");

            if (f == NULL) {
                sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
                write(clientSockId, response, strlen(response));
            } else {
                // legge il file in memoria per sapere il Content-Length
                unsigned char *body = malloc(1000000);
                int bodySize = 0;
                int c;
                while ((c = fgetc(f)) != EOF)
                    body[bodySize++] = c;
                fclose(f);

                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length:%d\r\n\r\n", bodySize);
                write(clientSockId, response, strlen(response));
                write(clientSockId, body, bodySize);
                free(body);
            }

        } else {
            // metodo non supportato fuori dal CGI
            sprintf(response, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
            write(clientSockId, response, strlen(response));
        }

        close(clientSockId);
        exit(EXIT_SUCCESS);
    }

    close(sockId);
    return 0;
}