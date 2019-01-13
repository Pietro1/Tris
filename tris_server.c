#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "tris_list.c"

fd_set master;
fd_set read_fds;

char rcvname[36];

void get_error(int ret) {
	if(ret == -1) {
		perror("Server receive error!");
		exit(1);
	}
}

void quit(int i) {
	struct client* p = testa;
	while(p->socket != i) {
		p = p->next;
	}
	printf("%s si è disconnesso\n", p->nome);
        FD_CLR(i, &master);
        close(i);
        destroy(i);
}

void who(int i) {
	int dim, app;
	int ret, len;
	struct client* richiedente = search(NULL, i);
	dim = size();
	dim--;
	app = htonl(dim);
	ret = send(i, (void*)&app, sizeof(int), 0);
	get_error(ret);
	struct client* p;
	p = testa;
	while(p != NULL) {
		if(strcmp(p->nome, richiedente->nome) == 0) {
			p = p->next;
			continue;
		}
		len = sizeof(p->nome);
		app = htonl(len);
		ret = send(i, (void*)&app, sizeof(app), 0);
		get_error(ret);
		ret = send(i, (void*)p->nome, len, 0);
		get_error(ret);
		p = p->next;
	}
}

void connetti(int sk) {
	int ret, app;
	int lname;
	memset(rcvname, 0, sizeof(rcvname));
	//ricevo il nome dell'avversario app
	//ricevo richiesta
	ret = recv(sk, (void*)&app, sizeof(int), 0);
	get_error(ret);
	lname = ntohl(app);
	ret = recv(sk, (void*)rcvname, lname, 0);
	get_error(ret);
	//Cerco l'avversario in lista
	struct client* p = search(rcvname, -1);	
	struct client* richiedente = search(NULL, sk);	//Ricerca richiedente
	//Invia lo stato dell'avversario al richiedente
	if(p == NULL) {
		int not_exist = 2;
		app = htonl(not_exist);
		ret = send(sk, (void*)&app, sizeof(int), 0);
		get_error(ret);
		return;
	}
	if(strcmp(p->nome, richiedente->nome) == 0) {
		int yourself = 3;
		app = htonl(yourself);
		ret = send(sk, (void*)&app, sizeof(int), 0);
		get_error(ret);
		return;
	}
	app = htonl(p->occupato);
	ret = send(sk, (void*)&app, sizeof(int), 0);
	get_error(ret);
	//Invio richiesta di gioco all'avversario socket p->socket
	if(p->occupato == 0) {	//se l'avversario non è occupato
		int cod_req = 1;	//codice richiesta
		app = htonl(cod_req);
		//invio richiesta
		ret = send(p->socket, (void*)&app, sizeof(int), 0);
		get_error(ret);
		//Invio a p->socket il nome del richiedente
		lname = strlen(richiedente->nome);
		app = htonl(lname);
		ret = send(p->socket, (void*)&app, sizeof(int), 0);
		get_error(ret);
		ret = send(p->socket, (void*)richiedente->nome, lname, 0);
		get_error(ret);
		//printf("ricevo codice accettazione\n");
		ret = recv(p->socket, (void*)&app, sizeof(int), 0);
		get_error(ret);
		//printf("Codice ricevuto\n");
		cod_req = ntohl(app);
		if(cod_req == 1) {
			printf("L'avversario ha accettato la richiesta di gioco\n");
			cod_req = 2;
			app = htonl(cod_req);
			ret = send(richiedente->socket, (void*)&app, sizeof(int), 0);
			get_error(ret);
			
			//invio al richiedente l'indirizzo ip dell'avversario
			char* address = inet_ntoa(p->ip_address.sin_addr);
			lname = strlen(address);
			address[lname] = '\0';
			lname ++;
			app = htonl(lname);
			ret = send(richiedente->socket, (void*)&app, sizeof(int), 0);
			get_error(ret);
			ret = send(richiedente->socket, (void*)address, lname, 0);
			get_error(ret);

			//invio la porta udp al richiedente
			//printf("%s: %i\n", p->nome , p->udp_port);
			app = htonl(p->udp_port);
			ret = send(richiedente->socket, (void*)&app, sizeof(int), 0);
			get_error(ret);
		
			richiedente->occupato = 1;
			p->occupato = 1;
			
			printf("%s si è connesso a %s\n", richiedente->nome, p->nome);
		} else {
			cod_req = 3;	//richiesta rifiutata
			app = htonl(cod_req);
			ret = send(richiedente->socket, (void*)&app, sizeof(int), 0);
			get_error(ret);
		}
	} //fine if client libero
}

void disconnetti(int sk) {
	int app, lname, ret;
	char avvers[36];
	ret = recv(sk, (void*)&app, sizeof(int), 0);
	get_error(ret);
	lname = ntohl(app);
	ret = recv(sk, (void*)avvers, lname, 0);
	get_error(ret);
	avvers[lname++] = '\0';
	struct client* p = search(NULL, sk);
	struct client* avv = search(avvers, -1);
	//printf("%s\n", avv->nome);
	p->occupato = 0;
	avv->occupato = 0;
	printf("%s si è disconnesso da %s\n", p->nome, avv->nome);
	printf("%s è libero\n", p->nome);
	printf("%s è libero\n", avv->nome);
}

void fine_partita(int sk) {
	//ricevo il nome dell'avversario che ha perso
	int ret, app, lname;
	char avvers[36];
	ret = recv(sk, (void*)&app, sizeof(int), 0);
	get_error(ret);
	lname = ntohl(app);
	ret = recv(sk, (void*)avvers, lname, 0);
	get_error(ret);
	//printf("Nome ricevuto con successo\n");
	//segnalo la vincita e libero i due giocatori
	struct client* v = search(NULL, sk);
	struct client* p = search(avvers, -1);
	v->occupato = 0;
	p->occupato = 0;
	printf("%s ha vinto contro %s\n", v->nome, p->nome);
	printf("%s è libero\n", v->nome);
	printf("%s è libero\n", p->nome);
}

void pareggio(int sk) {
	int ret, app, lname;
	char avvers[36];
	//ricevo il nome dell'avversario
	ret = recv(sk, (void*)&app, sizeof(int), 0);
	get_error(ret);
	lname = ntohl(app);
	ret = recv(sk, (void*)avvers, lname, 0);
	get_error(ret);
	//segnalo la vincita e libero i due giocatori
	struct client* v = search(NULL, sk);
	struct client* p = search(avvers, -1);
	v->occupato = 0;
	p->occupato = 0;
	printf("%s e %s hanno pareggiato\n", v->nome, p->nome);
	printf("%s è libero\n", v->nome);
	printf("%s è libero\n", p->nome);
}

int main(int argc, char *argv[]) {
        //VARIABILI
        struct sockaddr_in serveraddr;
        struct sockaddr_in clientaddr;
        socklen_t addrlen;
        int fdmax;      // numero massimo di file descriptor
        int sk;         //listening socket
        int c_sk;       //connection socket
        int yes = 1;    //per la setsockopt()
        int port, ret;
        int i;
	int rcvport;	//porta UDP
	int comando;
        //CODICE
        if(argc != 3) {
                fprintf(stderr, "Errore nel passaggio parametri\n");
                exit(1);
        }

	if(atoi(argv[2]) < 1024) {
		fprintf(stderr, "Numero di porta non valido\n");
		exit(1);
	}

        printf("Indirizzo: %s (Porta: %s)\n", argv[1], argv[2]);

	//Il server accetta la connessione TCP da parte di un client
        FD_ZERO(&master);
        FD_ZERO(&read_fds);
        sk = socket(AF_INET, SOCK_STREAM, 0); //listening socket
        if(sk < 0) {
                perror("Server socket error!");
                exit(1);
        }
	if(setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                perror("Server setsockopt error!");
                exit(1);
        }
	memset(&serveraddr, 0, sizeof(serveraddr));
        port = atoi(argv[2]);
        serveraddr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, argv[1], &serveraddr.sin_addr.s_addr);
        serveraddr.sin_port = htons(port);

	ret = bind(sk, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
        if(ret == -1) {
                perror("Server bind error!");
                exit(1);
        }

	ret = listen(sk, 10);
        if(ret == -1) {
                perror("Server listen error!");
                exit(1);
        }

        FD_SET(sk, &master);            //aggiungo il listening socket al master set
        fdmax = sk;

	//Inizializzazione semaforo di mutua esclusione
	//sem_init(&sem, 0, 1);

	for(;;) {
                read_fds = master;

                ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
                if(ret == -1) {
                        perror("Server select error!");
                        exit(1);
                }

		for(i = 0; i <= fdmax; i++) {
                        if(FD_ISSET(i, &read_fds)) {
                                if(i == sk) {
                                        addrlen = sizeof(clientaddr);
                                        //printf("Accetto la connessione e creo il connection so$
                                        c_sk = accept(sk, (struct sockaddr*)&clientaddr, &addrlen);
                                        if(c_sk == -1) {
                                                perror("Server accept error!");
                                                exit(1);
                                        }
                                        printf("Connessione stabilita con il client\n");
                                        // connection socket creato con successo
                                        FD_SET(c_sk, &master);
                                        if(c_sk > fdmax)
                                                fdmax = c_sk;
					memset(rcvname, 0, sizeof(rcvname));    //pulitura
					// ricevo il nome del client in rcvname
					int size, app;
					ret = recv(c_sk, (void*)&size, sizeof(size), 0);
					get_error(ret);
					app = ntohl(size);
					//ricevo il nome del client
					ret = recv(c_sk, (void*)rcvname, app, 0);
					get_error(ret);
					
					//ricevo la porta UDP
					ret = recv(c_sk, (void*)&app, sizeof(int), 0);
					get_error(ret);
					rcvport = ntohl(app);

					//Aggiungo il client alla lista
					add(rcvname, rcvport, c_sk, clientaddr);
					printf("%s si è connesso\n", rcvname);
					printf("%s è libero\n", rcvname);
				} else {
					//ricevo un comando dal client
					int app;
					ret = recv(i, (void*)&app, sizeof(int), 0);
					get_error(ret);
					if(ret == 0) {
						struct client* p = search(NULL, i);
						printf("%s si è disconnesso\n", p->nome);
						FD_CLR(i, &master);
        					close(i);
						destroy(i);
						break;
					}
					comando = ntohl(app);
					
					if(comando == 0) {
						quit(i);
                                                break;
                                        }
					if(comando == 1) {
						who(i);
						break;
					}
					if(comando == 2) {
						connetti(i);
						break;
					}
					if(comando == 10) {
						disconnetti(i);
						break;
					}
					if(comando == 11) {
						//comando di vittoria
						fine_partita(i);
					}
					if(comando == 12) {
						//pareggio
						pareggio(i);
					}
				}
			}
		}
	}
}

