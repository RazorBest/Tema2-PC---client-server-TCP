# Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

Am implementat aplicatiile client si server pornind de la codul scris la
laboratorul 8, urmand ca ulterior sa decuplez anumite componente cu
functionalitati bine definite.

Voi descrie protocolul de nivel aplicatie prin formatul pachetelor care
circula intre clienti si server. Apoi, voi descrie protocolul de incadrare
al protocoalelor TCP.

Voi descrie cum s-a facut multiplexarea canalelor de comunicare pentru client
si server.

## Tipurile de pachete la nivelul aplicatie

1. News - pachete trimise de clientii UDP la server. Format:
    [ TOPIC | TIP_DATE | VALUE ]

    TOPIC - 50 de octeti
    TIP_DATE - un octet 
    VALUE - un sir variabil de maxim 1500 de octeti care va fi interpretat in
        functie de valoare din TIP_DATE.

2. Subscriber news - pachet de news trimis de la server la subscriberi prin
TCP. Asemanator cu News, dar se adauga si ip-ul si portul de unde a fost primit
initial pachetul. Format:
    [ UDP_IP | UDP_PORT | TOPIC | TIP_DATE | VALUE ]

    UDP_IP - 4 octeti care reprezinta ip-ul sursei de news
    UDP_PORT - 2 octeti care reprezinta portul sursei de news
  
3. Command - pachetete trimise intre subscriberii TCP si server care au rolul
de autentificare si comandare. Acestea au lungime fixa.

    [ TIP | DATE ]


    DATE - date de comanda al caror format depinde de TIP. Particularizarile
        sunt descrise la fiecare TIP.

    TIP - un octet care reprezinta tipul pachetului Command. Poate fi:
        - TYPE_AUTH - Indica cererea (subscriber->server) de autentificare
            sau confirmarea autentificarii (server->subscriber). Astfel,
            daca un server primeste un astfel de pachet, pentru confirmare,
            acesta va trebui sa trimita inapoi acelasi pachet (daca ID-ul este
            unic). Orice client trebuie sa trimita un astfel de pachet la
            inceput si trebuie validat. Daca nu, nu vor fi luate in considerare
            celelate comenzi.
            Format:
            [ "TYPE_AUTH" | ID | UNUSED_DATA ]

            ID - un sir de caractere care reprezinta id-ul subscriber-ului cu
                care acesta incearca sa se autentifice.

        - TYPE_AUTH_DUPLICATE - Indica rejecarea autentificarii
            (server->subscriber) deoarece id-ul este deja folosit de alt
            client. Clientul ramane in starea de autentificare si inca nu poate
            trimite alte comenzi in afara de TYPE_AUTH. Format:
            [ "TYPE_AUTH_DUPLICATE" | ID | UNUSED_DATA ]

            ID - un sir de caractere care reprezinta id-ul duplicat.

        - TYPE_SUB - Pachet subscriber->server care indica cererea de abonare
            la un topic. El va primi doar noutatile care sunt generate
            cat timp clientul e online. Format:
            [ "TYPE_SUB" | TOPIC | UNUSED_DATA ]
            
            TOPIC - Un sir de 50 de octeti care indica topic-ul la care
                clientul vrea sa se aboneze.

        - TYPE_SUB_SF - Pachet subscriber->server care indica cererea de
            abonare la un topic. La fel ca TYPE_SUB, dar diferenta este ca
            clientul vrea sa primeasca noutatile care au fost trimise chiar
            si cand clientul nu era conectat. Astfel, la conectare, clientul
            va primi toate noutatile cu care a ramas in urma, cat timp a fost
            deconectat. Format:

            [ "TYPE_SUB_SF" | TOPIC | UNUSED_DATA ]
            
            TOPIC - Un sir de 50 de octeti care indica topic-ul la care
                clientul vrea sa se aboneze online+offline.

        - TYPE_UNSUB - Pachet subscriber->server care indica cererea de
            dezabonare la un topic. Format:
 
            [ "TYPE_SUB_SF" | TOPIC | UNUSED_DATA ]

            TOPIC - Un sir de 50 de octeti care indica topic-ul la care
                clientul vrea sa se dezaboneze.

        - TYPE_NOT_AUTH - Pachet server->subscriber care indica ca clientul
            inca nu s-a autentificat cu un id. Este trimis atunci cand clientul
            trimite o comanda la care inca nu are drepturi.


## Protocolul de incadrare

Pentru ca TCP este tip flux, am aplicat un protocol simplu de incadrare
intre TCP si nivelul aplicatie. Acest protocol a fost implementat de clasa
BufferedTcpSocket.

Pachetul protocolului de incadrare are urmatorul format:
    [ LUNGIME | PACHET_APLICATIE ]

    - LUNGIME - numarul de octeti pe care ii are PACHET
    - PACHET_APLICATIE  - date care reprezinta un pachet de nivel aplicatie

Clasa BufferedTcpSocket are doua metode principale de interfatare: recv si
send. Metoda send apeleaza functia send() din sys/socket.h. Metoda recv
apeleaza functia globala recv() din sys/socket.h si stocheaza pachetul intr-un
buffer. Daca se cumuleaza mai multe pachete, acestea se pun intr-o coada de
buffere. Pentru a obtine aceste pachete 

Cum se foloseste un BufferedTcpSocket? In descrierea pasilor, prin
"utilizator" ne referim la entitatea care foloseste clasa BufferedTcpSocket:

    1. Se creeaza obiectul caruia i se da un socket. Acest socket e initializat
        de utilizator
    2. Se apeleaza metoda send() pentru a se trimite un mesaj de o anumta
        lungime. Acesta va arunca exceptii daca mesajul e prea lung sau daca
        exista o eroare de trimitere la nivel TCP.
    3. Se apeleaza metoda recv() pentru a citi mesaje de pe canalul TCP.
        Mesajele sunt stocate in bufferul, intern. Utilizatorul, inca nu are
        acces la ele.
    4. Se apeleaza packet_ready() pentru a verifica daca exista cel putin un
        pachet. Daca nu, se trece la pasul 6.
    5. Se apeleaza write_packet() care scrie un pachet in buffer. Pachetul
        e scos din coada interna. Ne intoarcem la pasul 4.
    6. Daca mai avem nevoie de date, asteptam cu select, si ne intoarcem la
        pasii 2 sau 3. Daca nu, trecem la pasul 7.
    7. Apelam reset ca sa golim bufferul si coada de pachete. Acest pas nu este
        obligatoriu.
    8. Utilizatorul apeleaza close pe socket si distruge obiectul de
        BufferedTcpSocket.



## Multiplexarea canalelor de comunicare

Multiplexarea clientilor TCP si a stdin-ului este facuta de clasa MultiSocket.


Ping, ping, ping...
Temele ma-nving,
Creierul da timeout,
Inima da seg fault
Si plange ca un flaut.

Sunt pierdut printre pachete
Ca o datagrama
Cunostinte incomplete
Si lipsit de mama.

Sperantele s-au ofilit
Cand am vazut finalul:
Un "0 puncte" necajit
Patase terminalul.
