Siccome, con la crescente popolarità del web, divenne comune per un singolo server ospitare
più siti web (un processo noto come hosting virtuale), dal PROTOCOLLO HTTP 1.1 bisogna
specificare per forza il nome del server (=l’Host) dentro la GET
Esempio:
GET /index.html HTTP/1.1
Host: www.example.com
Nel protocollo HTTP 1.1, la connessione non viene più chiusa dopo l’invio del file (da
parte del server) (TRANSAZIONE), ma rimane aperta (CONNESSIONE) (keep alive), fino
ad un certo TIME OUT.

Per poter distinguere le varie response, è necessario che il client
conosca la lunghezza di ciascuna response (e dunque capire quando è terminata).

Nelle response, si utilizza il Chunked Transfer Encoding, che permette di
suddividere l’entity body della response in una serie di chunk (chunked-body).

Si tratta di una proprietà del messaggio, più che di una proprietà della response originaria.
Per indicare che una response utilizza il chunked transfer encoding, il server include
nell’header “Transfer-Encoding” della response HTTP la dicitura “chunked”:

NB:
- se la response contiene l’HEADER “Content-Length” => HTTP/1.1 (o 1.0) e
    body NON chunked (perchè il server sapeva a priori la sua lunghezza)

- se response contiene l’HEADER “Transfer-Encoding” => HTTP/1.1 (o 1.0) e ho
    body CHUNKED (solo se il valore dell’header è “chunked”, altrimenti potrei
    avere altri metodi di trasferimento del body) (perchè il server NON sapeva a
    priori la lunghezza dell'entity body)
- i due header NON possono ESSERCI CONTEMPORANEAMENTE


Ogni chunk è:
- preceduto dalla sua dimensione in esadecimale
- seguito da una sequenza di ritorno a capo <-> CTRF (\r\n).

L’ultimo chunk ha una dimensione di 0, segnalando la fine del messaggio.


Esempio di response HTTP/1.1 con codifica chunked:
HTTP/1.1 200 OK\r\n
Transfer-Encoding: chunked\r\n
4\r\n           //indica che il primo chunk ha una lunghezza di 4 byte.
Wiki\r\n        //è il primo chunk di dati.
5\r\n           //indica che il secondo chunk ha una lunghezza di 5 byte.
pedia\r\n       //è il secondo chunk di dati.
E\r\n           //indica che il terzo chunk ha una lunghezza di 14 byte.
in\r\n
\r\n
chunks.\r\n     //è il terzo chunk di dati.
0\r\n // segnala la fine della risposta (un chunk di lunghezza 0).
//TRAILER SECTION
\r\n //CRLF


- chunked-body: È il corpo del messaggio codificato in chunk. Consiste in una
sequenza di chunk seguiti dall’ultimo chunk (last-chunk), una sezione opzionale di
trailer (trailer-section), e termina con un CRLF.

- chunk: Rappresenta un singolo chunk, che include:
- chunk-size: La dimensione del chunk in formato esadecimale.
- [chunk-ext]: Un’estensione opzionale del chunk.
- CRLF: Una sequenza di ritorno a capo e nuova riga (\r\n).
- chunk-data: I dati effettivi del chunk della dimensione specificata.
- CRLF: Un’altra sequenza di ritorno a capo e nuova riga che segue i dati.
