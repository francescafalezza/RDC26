Un HTTP proxy è un server che intermedia le request tra un CLIENT e un SERVER web.
- Il CLIENT invia le sue request HTTP al proxy, che poi le inoltra al SERVER.
- Il SERVER web risponde al proxy, che a sua volta restituisce la risposta al
CLIENT.


L’unica differenza è che il CLIENT nella request, deve inviare l'URL COMPLETO (ES:
http://www.example.com/index.html)della risorsa, piuttosto che solo il percorso relativo
sul SERVER (FILE PATH) (ES: /index.html), perchè il proxy deve sapere quale SERVER
(ES: www.example.com) contattare oltre che quale risorsa richiedere (ES: /index.html)

ES: request con SOLO FILEPATH (NO PROXY)
GET /index.html HTTP/1.1
Host: www.example.com


ES: request con URL COMPLETO (PROXY)
GET http://www.example.com/index.html HTTP/1.1
Host: www.example.com

L'URL COMPLETO include:
- protocollo (ES: http:// )
- nome del dominio o IP del SERVER di destinazione (ES: www.example.com)
- percorso della risorsa richiesta (ES: /index.html)


Proxy con e senza Tunneling e applicazioni:
I proxy possono essere configurati per funzionare in 2 modalità alternative:
- senza tunneling => GET request
- con tunneling => CONNECT request


Proxy SENZA TUNNELING (Forward Proxy) => GET request
Un proxy senza tunneling funge da intermediario CON VISIBILITA’ (<-> può vedere e
modificare le HTTP request e response) tra il CLIENT e il SERVER web.
Funziona attraverso delle semplici GET request da parte del CLIENT

Applicazioni:
- Caching: Memorizza le risposte dei server per servire richieste future più
velocemente.
- Filtro dei Contenuti: Può bloccare contenuti inappropriati o non sicuri.
ES: GET request ad un proxy SENZA TUNNELING
GET http://www.example.com/index.html HTTP/1.1
Host: www.example.com



Proxy CON TUNNELING => CONNECT request
Un proxy senza tunneling funge da intermediario SENZA VISIBILITA’ (<-> NON può
vedere e modificare le HTTP request e response) tra il CLIENT e il SERVER web, perchè
sono criptate <-> si dice che c’è un “tunnel”.

Visibilità: Il proxy non può vedere né modificare il contenuto del traffico una volta stabilito il
tunnel, poiché il traffico è criptato (HTTPS).
Un proxy con tunneling utilizza il metodo HTTP CONNECT request per creare una
connessione diretta (tunnel) tra il CLIENT e il SERVER web.
Questa modalità è spesso utilizzata per gestire connessioni HTTPS, dove il proxy stabilisce
una connessione TCP diretta tra il client e il server, passando attraverso il proxy.

Gestisce principalmente traffico HTTPS.
1. Il CLIENT invia una request CONNECT al proxy,
2. Il proxy che stabilisce un tunnel TCP diretto al SERVER web.

Il Tunneling utilizza il TLS (Transport Layer Security) che presuppone che tra CLIENT e
SERVER ci sia un canale sicuro criptato, quindi cifratura end-to-end.

Quando un CLIENT desidera stabilire una connessione TLS tramite un proxy HTTP, utilizza
il metodo HTTP CONNECT.

Questo metodo richiede al proxy di aprire una connessione TCP al server di destinazione e
semplicemente inoltrare i dati tra il client e il server.



Ecco una panoramica dei passaggi:
- Richiesta di Connessione: Il CLIENT invia una richiesta HTTP CONNECT al
proxy, specificando il server di destinazione e la porta.
ES:
CONNECT www.server.com:443 HTTP/1.1
Host: www.server.com:443

- Connessione Stabilita: Se il proxy è configurato per consentire la connessione,
risponde con un codice di stato 200 (Connection Established).
ES:
HTTP/1.1 200 Connection Established

- Tunneling del Traffico: Una volta stabilita la connessione, il client inizia a stabilire
una sessione TLS direttamente con il server di destinazione. Il proxy inoltra tutti i dati
tra il client e il server senza interpretarli.


Dopo che il metodo CONNECT ha stabilito la connessione tunnel, il client e il server
procedono con lo scambio di byte cifrati.
NB: si parla di tunneling perchè il proxy NON salva su un buffer le request e
response, ma le scrive direttamente nei socket destinatari (di rispettivamente SERVER
e CLIENT).

Applicazioni:
- Sicurezza: Utilizzato per mantenere la sicurezza e la privacy del traffico HTTPS.
- Firewall Traversal: il proxy come firewall effettua il controllo su attacchi malevoli ai SERVER


ES:
CONNECT www.example.com:443 HTTP/1.1
Host: www.example.com:443
NB: www.example.com:433 = INDIRIZZO IP SERVER + PORTA STANDARD HTTPS
www.example.com:443
- www.example.com = NOME (o IP) SERVER
- 443 = PORTA STANDARD per HTTPS

Esercizio: pw.c <-> implementazione SERVER PROXY SENZA/CON TUNNELING:
- Esegui pw.c
- Impostazioni browser > config manuale proxy > IP: (solito putty: 88.80.187.84 ),
porta: 8369 (quella che c’è sul file)
- da browser qualsiasi cosa cerchi passa per il tuo programmino.
- GET: cerca: “ES: http://147.162.235.155/”
- GET: cerca: “ES: http://172.217.169.4/”
- CONNECT: se cerchi “ES: https://www.example.com”, o una cosa a caso
fa una CONNECT (perchè lavora tutto con HTTPS) infatti c’è l’URL
COMPLETO
- GET: se cerchi “ES: http://www.example.com”, o una cosa a caso fa una
GET (perchè lavora tutto con HTTP) infatti c’è l’URL NON COMPLETO

- se chiudi il programma del proxy non va più il browser chiaramente

Quando ricevo il metodo GET => connessione standard (SENZA TUNNELING):
– faccio parsing dell’URI
– prendo il nome del server
– faccio gethostbyname per risolvere l’indirizzo
– impacchetto e mando la request al SERVER
– ottengo la response dal SERVER e la mando al CLIENT


Quando ricevo il metodo CONNECT => connessione tunnel (CON TUNNELING):
– parsing della CONNECT per individuare hostname
– gethostbyname per risolvere l’indirizzo
– genera due figli per lavorare parallelamente nei due versi



gethostbyname(hostname); -> è una funzione della libreria di rete standard in C che
risolve un nome di host (hostname) in un indirizzo IP.
La utilizziamo per convertire l’hostname del proxy nel corrispettivo indirizzo IP.
E’ definita così:

struct hostent *gethostbyname(const char *name);

Restituisce un puntatore a una struttura hostent che contiene le informazioni sull'host.

* struct hostent {
char *h_name; // Nome ufficiale dell'host.
char **h_aliases; // Lista di alias.
int h_addrtype; // Tipo di indirizzo (AF_INET per IPv4).
int h_length; // Lunghezza dell'indirizzo in byte.
char **h_addr_list; // Lista di indirizzi (array di indirizzi IP).
char *h_addr; // Puntatore al primo host_address dell'h_addr_list
};

*/
NB: si parla di tunneling perchè il proxy NON salva su un buffer le request e
response, ma le scrive direttamente nei socket destinatari (di rispettivamente SERVER
e CLIENT)