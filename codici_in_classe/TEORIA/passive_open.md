In una comunicazione basata su socket, esistono due modi per aprire una connessione:

Active Open (Client): È l'azione di un processo che decide di contattare un altro host. 
Il client dice: "Voglio parlare con l'indirizzo X sulla porta Y". 
Nel  codice, questa azione corrisponde alla funzione connect().

Passive Open (Server): È l'azione di un processo che si mette in "ascolto" e aspetta che qualcuno 
lo contatti. 
Il server dice: "Io sono qui sulla porta 80; se qualcuno bussa, risponderò".

2. Le fasi del Passive Open nel Server
Per realizzare un Passive Open, un server deve eseguire una sequenza specifica di chiamate 
di sistema che il tuo client non fa:

        socket(): Crea l'endpoint (esattamente come il client).

        bind(): Lega il socket a una porta specifica (es. la porta 80). 
        Senza questa fase, il sistema operativo non saprebbe a quale processo consegnare i 
        pacchetti in arrivo su quella porta.

        listen(): Questa è la funzione che mette il socket in stato di Passive Open. 
        Il server comunica al kernel: "Da questo momento non inviare richieste, 
        ma accetta quelle in arrivo e mettile in una coda".

        accept(): Il server preleva una richiesta dalla coda e crea un nuovo socket 
        dedicato a quella specifica conversazione.



Il Client è proattivo: sceglie il momento e il destinatario.

Il Server è reattivo: rimane in attesa (stato LISTEN) finché un pacchetto SYN (l'inizio del Three-Way Handshake) non arriva dalla rete.