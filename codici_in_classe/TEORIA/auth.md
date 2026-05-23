WWW-Authenticate
Il campo dell'intestazione di risposta (response-header) WWW-Authenticate deve essere incluso nei messaggi di risposta 401 (unauthorized).

Il valore del campo consiste in almeno una richiesta di credenziali (challenge) che indica lo schema (o gli schemi) di autenticazione e i parametri applicabili alla 
Request-URI.WWW-Authenticate = "WWW-Authenticate" ":" 1#challengeIl processo di autenticazione dell'accesso HTTP è descritto nella Sezione 11. 

Gli user agent devono prestare particolare attenzione nell'analizzare il valore del campo WWW-Authenticate se questo contiene più di una challenge, o se viene fornito più di un campo header WWW-Authenticate, poiché il contenuto di una challenge può a sua volta contenere un elenco di parametri di autenticazione separati da virgole.



401 Unauthorized (Non autorizzato)
La richiesta richiede l'autenticazione dell'utente. 

La risposta deve includere un campo header WWW-Authenticate (Sezione 10.16) contenente una challenge applicabile alla risorsa richiesta. 

Il client può ripetere la richiesta inserendo un campo header Authorization appropriato (Sezione 10.2). Se la richiesta includeva già delle credenziali in Authorization, la risposta 401 indica che l'autorizzazione è stata rifiutata per tali credenziali.

Se la risposta 401 contiene la stessa challenge della risposta precedente, e lo user agent ha già tentato l'autenticazione almeno una volta, all'utente dovrebbe essere mostrata l'entità allegata alla risposta, poiché tale entità potrebbe includere informazioni diagnostiche rilevanti. 

L'autenticazione dell'accesso HTTP è spiegata nella Sezione 11.10.2 Authorization



Uno user agent che desideri autenticarsi presso un server — solitamente, ma non necessariamente, dopo aver ricevuto una risposta 401 — può farlo includendo un campo header di richiesta Authorization all'interno della richiesta. 

Il valore del campo Authorization consiste nelle credenziali che contengono le informazioni di autenticazione dello user agent per l'ambito (realm) della risorsa richiesta.

Authorization = "Authorization" ":" credentials

L'autenticazione dell'accesso HTTP è descritta nella Sezione 11. 
Se una richiesta viene autenticata e viene specificato un realm, le stesse credenziali dovrebbero essere valide per tutte le altre richieste all'interno di questo realm.
Le risposte alle richieste contenenti un campo Authorization non sono memorizzabili in cache (not cachable).

11. Access Authentication (Autenticazione dell'Accesso)
HTTP fornisce un semplice meccanismo di autenticazione di tipo challenge-response (sfida-risposta) che può essere utilizzato da un server per sfidare la richiesta di un client e da un client per fornire informazioni di autenticazione. 

Utilizza un token estensibile e insensibile alle maiuscole/minuscole (case-insensitive) per identificare lo schema di autenticazione, seguito da un elenco separato da virgole di coppie attributo-valore che contengono i parametri necessari per ottenere l'autenticazione tramite tale schema.

auth-scheme = token
auth-param  = token "=" quoted-string

Il messaggio di risposta 401 (unauthorized) viene utilizzato da un server di origine per sfidare l'autorizzazione di uno user agent. 

Questa risposta deve includere un campo header WWW-Authenticate contenente almeno una challenge applicabile alla risorsa richiesta.

challenge   = auth-scheme 1*SP realm *( "," auth-param )
realm       = "realm" "=" realm-value
realm-value = quoted-string

L'attributo realm (case-insensitive) è obbligatorio per tutti gli schemi di autenticazione che emettono una challenge. Il valore di realm (sensibile alle maiuscole/minuscole, case-sensitive), in combinazione con l'URL radice canonico del server a cui si accede, definisce lo spazio di protezione (protection space). 

Questi realm consentono di suddividere le risorse protette su un server in un insieme di spazi di protezione, ciascuno con il proprio schema di autenticazione e/o database di autorizzazione. 

Il valore di realm è una stringa, generalmente assegnata dal server di origine, che può avere una semantica aggiuntiva specifica dello schema di autenticazione.

Uno user agent che desideri autenticarsi presso un server — solitamente, ma non necessariamente, dopo aver ricevuto una risposta 401 — può farlo includendo un campo header Authorization nella richiesta. 
Il valore del campo Authorization consiste in credenziali contenenti le informazioni di autenticazione dello user agent per il realm della risorsa richiesta.


credentials = basic-credentials | ( auth-scheme #auth-param )

Il dominio su cui le credenziali possono essere applicate automaticamente da uno user agent è determinato dallo spazio di protezione. 
Se una richiesta precedente è stata autorizzata, le stesse credenziali possono essere riutilizzate per tutte le altre richieste all'interno di quello spazio di protezione per un periodo di tempo determinato dai parametri e/o dalle preferenze dell'utente. 
A meno che non sia definito diversamente dallo schema di autenticazione, un singolo spazio di protezione non può estendersi al di fuori dell'ambito del proprio server.

Se il server non desidera accettare le credenziali inviate con una richiesta, dovrebbe restituire una risposta 403 (forbidden).

Il protocollo HTTP non limita le applicazioni a questo semplice meccanismo di challenge-response per l'autenticazione dell'accesso. Possono essere utilizzati meccanismi aggiuntivi, come la crittografia a livello di trasporto o tramite l'incapsulamento del messaggio, e con campi header aggiuntivi che specificano le informazioni di autenticazione. Tuttavia, questi meccanismi aggiuntivi non sono definiti da questa specifica.

I proxy devono essere completamente trasparenti per quanto riguarda l'autenticazione dello user agent. Cioè, devono inoltrare gli header WWW-Authenticate e Authorization inalterati, e non devono memorizzare in cache la risposta a una richiesta contenente Authorization. 

HTTP/1.0 non fornisce un mezzo per l'autenticazione di un client presso un proxy.



11.1 Schema di Autenticazione di Base (Basic Authentication Scheme)

Lo schema di autenticazione "basic" si basa sul modello in cui lo user agent deve autenticarsi con un ID utente (user-ID) e una password per ogni realm. Il valore di realm deve essere considerato come una stringa opaca che può essere solo confrontata per verificarne l'uguaglianza con altri realm su quel server. Il server autorizzerà la richiesta solo se in grado di convalidare l'ID utente e la password per lo spazio di protezione della Request-URI. 

Non ci sono parametri di autenticazione opzionali.

Al ricevimento di una richiesta non autorizzata per un URI all'interno dello spazio di protezione, il server dovrebbe rispondere con una challenge simile alla seguente:

WWW-Authenticate: Basic realm="WallyWorld"dove "WallyWorld" è la stringa assegnata dal server per identificare lo spazio di protezione della Request-URI.

Per ricevere l'autorizzazione, il client invia l'ID utente e la password, separati da un singolo carattere di due punti (":"), all'interno di una stringa codificata in base64 nelle credenziali.

basic-credentials = "Basic" SP basic-cookie
basic-cookie      = <codifica base64 di userid-password,tranne per il fatto che non è limitata a 76 caratteri per riga>
userid-password   = [ token ] ":" *TEXT

Se lo user agent desidera inviare l'ID utente "Aladdin" e la password "open sesame", utilizzerà il seguente campo header:

Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==


Lo schema di autenticazione di base è un metodo non sicuro per filtrare l'accesso non autorizzato alle risorse su un server HTTP. Si basa sul presupposto che la connessione tra il client e il server possa essere considerata un canale sicuro (trusted carrier). Poiché questo non è generalmente vero su una rete aperta, lo schema di autenticazione di base dovrebbe essere utilizzato di conseguenza. Nonostante ciò, i client dovrebbero implementare questo schema per poter comunicare con i server che lo utilizzano.



Il Processo di Codifica Base64 (dall'RFC 1521)
Il processo di codifica rappresenta gruppi di input a 24 bit sotto forma di stringhe di output composte da 4 caratteri codificati. Procedendo da sinistra a destra, un gruppo di input a 24 bit viene formato concatenando 3 gruppi di input a 8 bit (byte). Questi 24 bit vengono poi trattati come 4 gruppi concatenati da 6 bit ciascuno, ognuno dei quali viene convertito in un singolo carattere dell'alfabeto base64.

Quando si codifica un flusso di bit tramite la codifica base64, si deve presumere che il flusso di bit sia ordinato partendo dal bit più significativo (most-significant-bit). Cioè, il primo bit nel flusso sarà il bit di ordine superiore nel primo byte, l'ottavo bit sarà il bit di ordine inferiore nel primo byte, e così via.

Ogni gruppo a 6 bit viene utilizzato come indice all'interno di un array di 64 caratteri stampabili. 
Il carattere referenziato dall'indice viene inserito nella stringa di output. Questi caratteri, identificati nella Tabella 1 (sotto), sono selezionati in modo da essere rappresentabili universalmente, e l'insieme esclude i caratteri con un significato particolare per il protocollo SMTP (ad esempio ".", CR, LF) e per i confini di incapsulamento definiti in questo documento (ad esempio "-").

Il flusso di output (i byte codificati) deve essere rappresentato in righe composte da non più di 76 caratteri ciascuna. Tutti i domini di riga o altri caratteri non presenti nella Tabella 1 devono essere ignorati dal software di decodifica. 

Nei dati base64, i caratteri diversi da quelli della Tabella 1, i ritorni a capo e altri spazi vuoti indicano probabilmente un errore di trasmissione, riguardo al quale potrebbe essere appropriato un messaggio di avviso o persino il rifiuto del messaggio in determinate circostanze.Un trattamento speciale viene eseguito se alla fine dei dati da codificare sono disponibili meno di 24 bit. 

Al termine di un blocco di dati viene sempre completato un blocco di codifica intero (quantum). Quando in un gruppo di input sono disponibili meno di 24 bit, vengono aggiunti bit a zero (sulla destra) per formare un numero intero di gruppi a 6 bit. Il riempimento alla fine dei dati (padding) viene eseguito utilizzando il carattere '='. Poiché tutti gli input base64 sono costituiti da un numero intero di ottetti (byte), possono verificarsi solo i seguenti casi... (il testo si interrompe qui)


Value Encoding  Value Encoding  Value Encoding  Value Encoding 
           0 A            17 R            34 i            51 z 
           1 B            18 S            35 j            52 0 
           2 C            19 T            36 k            53 1 
           3 D            20 U            37 l            54 2 
           4 E            21 V            38 m            55 3 
           5 F            22 W            39 n            56 4 
           6 G            23 X            40 o            57 5 
           7 H            24 Y            41 p            58 6 
           8 I            25 Z            42 q            59 7 
           9 J            26 a            43 r            60 8 
          10 K            27 b            44 s            61 9 
          11 L            28 c            45 t            62 + 
          12 M            29 d            46 u            63 / 
          13 N            30 e            47 v 
          14 O            31 f            48 w         (pad) = 
          15 P            32 g            49 x 
          16 Q            33 h            50 y 

 