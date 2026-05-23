Request HTTP\1.0:
Request-Line *(General-Header | Request-Header | Entity-Header) CRLF [Entity-Body]

- Request-Line = Method SP URI SP HTTP-Version CRLF

    - Method = GET | POST | HEAD

    - URI = percorso relativo della risorsa su server + eventuali parametri

    - HTTP-Version = HTTP/1.0.

- *(General-Header | Request-Header | Entity-Header) = uno tra questi header può
essere ripetuto quante volte si vuole. Gli header sono nel formato
Header-Name:Header Value.

    - General-Header: delle informazioni generali che si possono trovare sia nella
    richiesta che nella risposta, sono relative alla transazione.

    - Request-Header: si trovano solamente nella richiesta.

    - Entity-Header: contiene informazioni sull’entity body.

    - [Entity-Body]: è il file (contenuto) che viene trasferito, la cosa che abbiamo
    richiesto. E’ opzionale. Anche nella richiesta ci può essere un contenuto.


Esempio di GET Request HTTP\1.0:
// nella GET i parametri (fields) vanno nell’URL (dopo l’URI)
// la GET NON ha l’Entity Body
GET /path/resource?field1=value1&field2=value2 HTTP/1.0\r\n
Host: www.example.com

Esempio di POST Request HTTP\1.0:
// la POST invece ha l’Entity Body (e metto lì i parametri)
POST /submit-form HTTP/1.0\r\n
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)r\n
Content-Type: application/x-www-form-urlencodedr\n
Content-Length: 27r\n
field1=value1&field2=value2 // nella POST i parametri (fields) vanno nell’entity body


Response HTTP\1.0:
Status-Line *(General-Header | Response-Header | Entity-Header) CRLF [Entity-Body]

- Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    - HTTP-Version = HTTP/1.0
    -Status-Code = Xxx <-> 3 cifre in cui la prima indica la famiglia di
        errori/eccezioni:
            - 200: OK (la richiesta è stata completata con successo).
            - 404: Not Found (la risorsa richiesta non è stata trovata).
            - 500: Internal Server Error (si è verificato un errore sul server).
    - Reason-Phrase = Una breve descrizione testuale del codice di stato

- *(General-Header | Responce-Header | Entity-Header)* = uno tra questi header può
essere ripetuto quante volte si vuole. Gli header sono nel formato
Header-Name:Header Value. //NB dopo i “:” potrebbe esserci lo spazio
    - General-Header: delle informazioni generali che si possono trovare sia nella
    richiesta che nella risposta, sono relative alla transazione.
    - Response-Header: si trovano solamente nella response.
    - Entity-Header: contiene informazioni sull’entity body.

- [Entity-Body]: è il file (contenuto) che viene trasferito, la cosa che abbiamo
richiesto. E’ opzionale. La response potrebbe volere solo gli header per conoscere lo
stato della risorsa (tipo se è stata aggiornata o meno).

Esempio di Response HTTP\1.0:
HTTP/1.0 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 137\r\n
Date: Tue, 15 Nov 2024 08:12:31 GMT\r\n
Server: Apache/2.4.41 (Ubuntu)\r\n
<html>
<body>
<h1>Welcome to Example</h1>
<p>This is an example page.</p>
</body>
</html>
