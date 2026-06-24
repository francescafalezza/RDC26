

## REQUEST HTTP

Una request è composta da tre parti principali separate da `\r\n`:

**1. Request Line**

```
Method SP URI SP HTTP-Version CRLF
GET /index.html HTTP/1.1\r\n
```

- `Method`: `GET` (recupera risorsa), `POST` (invia dati), `HEAD` (solo header, no body)
- `URI`: percorso della risorsa sul server, es. `/index.html` o `/cgi-bin/ls`
- `HTTP-Version`: `HTTP/1.0` o `HTTP/1.1`

In C si costruisce con `sprintf` e si invia con `write(s, request, strlen(request))`.

**2. Headers della Request**

Ogni header è nella forma `Nome: Valore\r\n`. Esempi rilevanti:

- `Host: www.example.com` — obbligatorio in HTTP/1.1, identifica il server virtuale da contattare
- `Connection: keep-alive` / `Connection: close` — controlla se la connessione rimane aperta dopo la risposta
- `If-Modified-Since: <http-date>` — usato per il caching: il server risponde solo se la risorsa è stata modificata dopo quella data
- `Authorization: Basic <base64>` — credenziali per l'autenticazione Basic

**3. Riga vuota (fine header)**

```
\r\n
```

Separa gli header dal body. È il marcatore che il server legge per sapere quando la request è terminata.

**4. Entity Body** (opzionale, presente in POST)

Contiene i dati inviati. La lunghezza è indicata dall'header `Content-Length`.

---

## RESPONSE HTTP

**1. Status Line**

```
HTTP-Version SP Status-Code SP Reason-Phrase CRLF
HTTP/1.1 200 OK\r\n
```

Codici più comuni:
- `200 OK` — successo
- `304 Not Modified` — risorsa non cambiata (caching)
- `401 Unauthorized` — richiesta autenticazione
- `404 Not Found` — risorsa non trovata

In C si legge questa come prima stringa del buffer di risposta. Si estrae con `sscanf(statusLine, "%s %d %s", version, &code, phrase)`.

**2. Headers della Response**

Il parsing avviene leggendo un byte alla volta con `read(s, buf+i, 1)`, identificando il separatore `:` per dividere nome e valore, e `\r\n` per terminare ogni header. Una riga vuota `\r\n\r\n` segnala la fine degli header.

Header chiave da gestire:

- `Content-Length: N` — il body ha esattamente N byte. Si legge con `atoi()` e poi si legge il body con un loop `while(read(...) > 0 && bytes_letti < N)`

- `Transfer-Encoding: chunked` — il body è diviso in chunk. Ogni chunk è preceduto dalla sua dimensione in esadecimale + `\r\n`, poi i dati, poi `\r\n`. L'ultimo chunk ha dimensione `0`. La dimensione si converte da hex con aritmetica sui caratteri (`'a'-'f'`, `'0'-'9'`)

- `Last-Modified: <http-date>` — data di ultima modifica della risorsa, usata per il caching

- `WWW-Authenticate: Basic realm="..."` — il server richiede autenticazione

- `Expires: <http-date>` — caching deterministico: la risorsa è valida fino a questa data

- `Pragma: no-cache` / `Cache-Control: no-cache` — il client non deve cachare

**3. Riga vuota (fine header)**

Come nella request, `\r\n` finale separa header da body.

**4. Entity Body**

Il contenuto vero e proprio (HTML, file, output di comando). Come leggerlo dipende dagli header:

- Se `Content-Length` è presente: si legge esattamente quel numero di byte
- Se `Transfer-Encoding: chunked`: si legge il numero hex del chunk, poi i dati, ripetendo fino al chunk `0`
- Se nessuno dei due (HTTP/1.0): si legge finché `read()` restituisce 0 (il server chiude la connessione)

---

## MECCANISMI SPECIALI

**Caching**

Tre strategie:

1. `Expires` — il client controlla la data locale contro la scadenza. Se non scaduta, usa la cache senza contattare il server
2. `If-Modified-Since` — il client manda la data dell'ultimo download; se il server risponde `304 Not Modified`, si usa la cache
3. `HEAD` + `Last-Modified` — il client fa una HEAD request (solo header), confronta `Last-Modified` con il timestamp locale, e fa una GET solo se serve

Per le date si usano `time(2)`, `localtime(3)` / `gmtime`, `mktime(3)`, `strftime(3)`, `strptime(3)`.

**Autenticazione Basic**

Il server risponde `401` con `WWW-Authenticate: Basic`. Il client reinvia la request con `Authorization: Basic <base64(user:password)>`. La codifica Base64 converte ogni 3 byte di input in 4 caratteri dell'alfabeto base64.

**CGI / Gateway**

Se l'URI inizia con `/cgi-bin/` o `/exec/`, il server estrae il comando, lo esegue con `system(command)`, redirige l'output su un file temporaneo, e poi serve quel file come body della risposta.

**Reflect**

Se l'URI è `/reflect`, il server rimanda al client la sua stessa request, più l'IP sorgente (letto da `struct sockaddr_in` con `inet_ntoa()`) e la porta (`ntohs(remote.sin_port)`).



 I punti chiave da tenere a mente:

**Request**: si costruisce con `sprintf` e si manda in un colpo con `write`. In HTTP/1.1 l'header `Host` è obbligatorio, altrimenti il server risponde `400 Bad Request`.

**Response**: si legge un byte alla volta con `read(s, buf+i, 1)` per fare parsing preciso degli header, poi si cambia strategia per il body in base a ciò che si trova: `Content-Length` → loop contato, `Transfer-Encoding: chunked` → loop con parsing hex, nessuno dei due → loop fino a `read()` = 0 (solo HTTP/1.0).

**Chunked e Content-Length sono mutuamente esclusivi**: non possono mai comparire insieme nella stessa response.



