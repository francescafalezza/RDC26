Consiste nel salvare nella cache del CLIENT la risorsa ottenuta tramite response dal
SERVER, per poterla riutilizzare senza fare una nuova request.
Ci sono 3 tipi di HTTP CACHING:

->se l’aggiornamento della risorsa è DETERMINISTICO:
1. HEADER “Expires”: la response del SERVER include un header
“Expires: <expiring-date>”.
ES:
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 137
Date: Tue, 15 May 2024 07:28:00 GMT
Expires: Wed, 21 Oct 2024 07:28:00 GMT
<html>
<body>
<h1>Welcome to Example</h1>
<p>This is an example page.</p>
</body>
</html>
A livello CLIENT:
- se <actual-date> > <expiring-date> => il CLIENT fa una request GET per la
risorsa al SERVER.
ES:
GET /index.html HTTP/1.1
Host: www.example.com
- altrimenti => usa la versione salvata in CACHE
(“Expires: -1” indica di NON salvare mai in cache la risorsa)

RFC 1945: 
Il Campo Header "Expires"
Il campo dell'intestazione dell'entità Expires indica la data e l'ora dopo le quali l'entità deve essere considerata obsoleta (stale). Ciò consente ai fornitori di informazioni di suggerire la volatilità della risorsa, o una data dopo la quale le informazioni potrebbero non essere più valide. Le applicazioni non devono memorizzare in cache questa entità oltre la data indicata.

La presenza di un campo Expires non implica che la risorsa originale cambierà o cesserà di esistere in quel momento, prima o dopo. Tuttavia, i fornitori di informazioni che sanno o sospettano che una risorsa cambierà entro una certa data dovrebbero includere un header Expires con tale data. Il formato è una data e un'ora assolute come definito dal formato HTTP-date nella Sezione 3.3.

Expires = "Expires" ":" HTTP-date

Un esempio del suo utilizzo è:
Expires: Thu, 01 Dec 1994 16:00:00 GMT

Regole di Caching e Dinamismo
Se la data fornita è uguale o precedente al valore dell'header Date, il destinatario non deve memorizzare in cache l'entità allegata. Se una risorsa è dinamica per natura, come nel caso di molti processi di produzione dati, alle entità provenienti da tale risorsa dovrebbe essere assegnato un valore di Expires appropriato che rifletta tale dinamismo.

Il campo Expires non può essere utilizzato per forzare un user agent (browser) ad aggiornare la visualizzazione o ricaricare una risorsa; la sua semantica si applica solo ai meccanismi di caching, e tali meccanismi devono controllare lo stato di scadenza di una risorsa solo quando viene avviata una nuova richiesta per quest'ultima.

Meccanismi di Cronologia
Gli user agent dispongono spesso di meccanismi di cronologia, come i pulsanti "Indietro" e gli elenchi cronologici, che possono essere utilizzati per visualizzare nuovamente un'entità recuperata in precedenza durante una sessione. Per impostazione predefinita, il campo Expires non si applica ai meccanismi di cronologia. Se l'entità è ancora in memoria, un meccanismo di cronologia dovrebbe visualizzarla anche se l'entità è scaduta, a meno che l'utente non abbia specificamente configurato l'agente per aggiornare i documenti della cronologia scaduti.

Nota sull'Implementazione
Si incoraggiano le applicazioni a essere tolleranti verso implementazioni errate o approssimative dell'header Expires. Un valore pari a zero (0) o un formato di data non valido dovrebbero essere considerati equivalenti a una "scadenza immediata". Sebbene questi valori non siano legittimi per lo standard HTTP/1.0, un'implementazione robusta è sempre auspicabile.



-> se l’aggiornamento della risorsa è STOCASTICO:
2. (GET) HEADER “If-Modified-Since”: la request GET del CLIENT include un
header “If-Modified-Since: <last-resource-download>” (last-resource-download<->
ultima data in cui il CLIENT ha scaricato la risorsa dal SERVER).
ES:
GET /index.html HTTP/1.1
Host: www.example.com
If-Modified-Since: Wed, 21 Oct 2015 07:28:00 GMT
A livello SERVER:
- se <last-modified-date> > <last-resource-download> => il SERVER invia una response con la risorsa aggiornata al CLIENT
ES:
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 137
Date: Tue, 15 May 2024 07:28:00 GMT

Expires: Wed, 21 Oct 2024 07:28:00 GMT
<html>
<body>
<h1>Welcome to Example</h1>
<p>This is an example page.</p>
</body>
</html>
- altrimenti => il SERVER invia una response con Status-Code: 304
Not-Modified e NON include l’entity body nella risposta
ES:
HTTP/1.1 304 Not Modified

RDC 1945: 
Il campo dell'intestazione di richiesta If-Modified-Since viene utilizzato con il metodo GET per renderlo condizionale: se la risorsa richiesta non è stata modificata dopo il momento specificato in questo campo, il server non restituirà una copia della risorsa; al suo posto, verrà restituita una risposta 304 (not modified) senza alcun corpo dell'entità (Entity-Body).

If-Modified-Since = "If-Modified-Since" ":" HTTP-date

Un esempio del campo è:
If-Modified-Since: Sat, 29 Oct 1994 19:43:31 GMT

Il Metodo GET Condizionale
Un metodo GET condizionale richiede che la risorsa identificata sia trasferita solo se è stata modificata dopo la data indicata dall'header If-Modified-Since. L'algoritmo per determinare ciò include i seguenti casi:

a) Se la richiesta risultasse normalmente in un qualsiasi stato diverso da 200 (ok), o se la data passata in If-Modified-Since non è valida, la risposta è esattamente la stessa di una normale GET. Una data successiva all'ora corrente del server è considerata non valida.

b) Se la risorsa è stata modificata dopo la data indicata in If-Modified-Since, la risposta è esattamente la stessa di una normale GET.

c) Se la risorsa non è stata modificata a partire da una data If-Modified-Since valida, il server deve restituire una risposta 304 (not modified).

Scopo della Funzionalità
Lo scopo di questa funzione è consentire l'aggiornamento efficiente delle informazioni memorizzate in cache con il minimo dispendio di risorse nelle transazioni



3. (HEAD) HEADER “Last-Modified”: il CLIENT effettua una request HEAD (che
richiede SOLO gli HEADER di una risorsa e NON il l’ENTITY BODY) al SERVER.
ES:
HEAD /index.html HTTP/1.1
Host: www.example.com
Il SERVER invia una response che include un header
“Last-Modified: <last-resource-modifying-date>”.
ES:
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 137
Date: Tue, 15 May 2024 07:28:00 GMT
Last-Modified: Wed, 21 Oct 2020 07:28:00 GMT
A livello CLIENT:
- se <last-resource-modifying-date> > <last-resource-download> => il
CLIENT invia una request GET per ottenere la risorsa aggiornata dal SERVER
ES:
GET /index.html HTTP/1.1
Host: www.example.com
- altrimenti => il CLIENT usa la versione della risorsa salvata in CACHE.

NB: HEADER “Pragma: no-cache” oppure “Cache-Control: no-cache”
Il SERVER può inviare una response con l’header “Pragma: no-cache” oppure
“Cache-Control: no-cache” per indicare al CLIENT di NON salvare su CACHE la risorsa


RDC 1945:
Il campo dell'intestazione dell'entità Last-Modified indica la data e l'ora in cui il mittente ritiene che la risorsa sia stata modificata l'ultima volta. La semantica esatta di questo campo è definita in termini di come il destinatario dovrebbe interpretarlo: se il destinatario possiede una copia di questa risorsa che è più vecchia della data indicata dal campo Last-Modified, tale copia deve essere considerata obsoleta (stale).

Last-Modified = "Last-Modified" ":" HTTP-date

Un esempio del suo utilizzo è:
Last-Modified: Tue, 15 Nov 1994 12:45:26 GMT

Il significato esatto di questo campo header dipende dall'implementazione del mittente e dalla natura della risorsa originale:

Per i file: può essere semplicemente l'ora di ultima modifica del file system.

Per entità con parti incluse dinamicamente: può essere la più recente tra le date di ultima modifica delle sue parti componenti.

Per i gateway di database: può essere il timestamp dell'ultimo aggiornamento del record.

Per gli oggetti virtuali: può essere l'ultima volta in cui lo stato interno è cambiato.

Un server di origine non deve inviare una data di Last-Modified successiva all'ora di creazione del messaggio del server stesso. In tali casi, qualora l'ultima modifica della risorsa indichi un momento nel futuro, il server deve sostituire tale data con la data di creazione del messaggio.

10.12 Pragma
Il campo dell'intestazione generale Pragma viene utilizzato per includere direttive specifiche dell'implementazione che possono essere applicate a qualsiasi destinatario lungo la catena di richiesta/risposta. Tutte le direttive pragma specificano comportamenti opzionali dal punto di vista del protocollo; tuttavia, alcuni sistemi potrebbero richiedere che il comportamento sia coerente con le direttive.

Pragma = "Pragma" ":" 1#pragma-directive
pragma-directive = "no-cache" | extension-pragma
extension-pragma = token [ "=" word ]

La direttiva "no-cache"
Quando la direttiva "no-cache" è presente in un messaggio di richiesta, un'applicazione dovrebbe inoltrare la richiesta verso il server di origine anche se possiede una copia memorizzata in cache di ciò che viene richiesto. Questo permette a un client di insistere per ricevere una risposta autorevole alla propria richiesta. Consente inoltre al client di aggiornare una copia in cache che risulta essere corrotta o obsoleta.

Le direttive pragma devono essere trasmesse inalterate da un'applicazione proxy o gateway, indipendentemente dalla loro rilevanza per tale applicazione, poiché le direttive potrebbero essere applicabili a tutti i destinatari lungo la catena di richiesta/risposta. Non è possibile specificare un pragma per un destinatario specifico; tuttavia, qualsiasi direttiva pragma non pertinente per un destinatario dovrebbe essere ignorata da quest'ultimo.