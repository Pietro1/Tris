// client

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include<sys/time.h>

int DIM = 36;

struct sockaddr_in avvers_addr;
struct sockaddr_in udp_addr;

fd_set master;
fd_set read_fds;

char mappa[10];
char avv_mark;
char my_mark;
int turno;
int in_game;
int num_mosse;	//per controllo parità
int num_avv;

void map_init() {
	int i;
	for(i = 0; i < 9; i++) {
		mappa[i] = '_';
	}
	mappa[9] = '\0';
}

void stampa_comandi() {
        printf("\n");
        printf("Sono disponibili i seguenti comandi:\n");
        printf(" * !help --> mostra l'elenco dei comandi disponibili\n");
        printf(" * !who --> mostra l'elenco dei client connessi al server\n");
        printf(" * !connect nome_client --> avvia una partita con l'utente nome_client\n");
        printf(" * !disconnect --> disconnette il client dall'attuale partita intrapresa con un altro peer\n");
        printf(" * !quit --> disconnette il client dal server\n");
        printf(" * !show_map --> mostra la mappa del gioco\n");
        printf(" * !hit num_cell --> marca la casella num_cell (valido solo quando è il proprio turno)\n");
        printf("\n");
}

void get_error(int ret) {
        if(ret == -1) {
                perror("Client send/recv error!");
                exit(1);
        }
}

void who(int sk) {
	char nome[DIM];
	int app, comando;
	int ret, len;
	comando = 1;
	//invio il comando
	app = htonl(comando);
        ret = send(sk, (void*)&app, sizeof(int), 0);
        get_error(ret);
        //Ricevo la lunghezza della lista
       	ret = recv(sk, (void*)&len, sizeof(len), 0);
       	get_error(ret);
        app = ntohl(len);
       	int j, lh;
	printf("Client connessi al server: ");
        for(j = 0; j < app; j ++) {
            	ret = recv(sk, (void*)&len, sizeof(len), 0);
               	get_error(ret);
                lh = ntohl(len);
                ret = recv(sk, (void*)nome, lh, 0);
                get_error(ret);
       	        printf("%s ", nome);
	}
	if(app == 0) {
		printf("Non ci sono client connessi");
	}
	printf("\n");
}

void quit(int sk, int udp_sk, char* avvers) {
	int app, comando = 0;
	int ret;
        app = htonl(comando);
	//Invio comando al server
        ret = send(sk, (void*)&app, sizeof(int), 0);
        get_error(ret);
	close(sk);
	FD_CLR(sk, &master);
	if(avvers != NULL) {
		close(udp_sk);
		FD_CLR(udp_sk, &master);
	}
}

void connetti(int sk, char* avvers) {
	int ret, app;
	int comando = 2, lcommand;
	// Invio il comando al server
	app = htonl(comando);
	ret = send(sk, (void*)&app, sizeof(int), 0);
	get_error(ret);
	//invio il nome dell'avversario al server
	lcommand = strlen(avvers);
	app = htonl(lcommand);
	ret = send(sk, (void*)&app, sizeof(app), 0);
	get_error(ret);
	ret = send(sk, (void*)avvers, lcommand, 0);
	get_error(ret);
	int respingi;
	//printf("ricevo l'informazione sullo stato dell'avversario\n");
	ret = recv(sk, (void*)&app, sizeof(int), 0);
	get_error(ret);
	//printf("Info ricevuta\n");
	respingi = ntohl(app);
	if(respingi == 1) {
		printf("%s è già impegnato in una partita\n", avvers);
	} else if(respingi == 2){
		printf("Impossibile connettersi a %s: utente inesistente\n", avvers);
	} else if(respingi == 3){
		printf("Non puoi connetterti con te stesso\n");
	} else {
		printf("%s è libero.. richiesta di gioco in corso...\n", avvers);
	}
}

void contatta(int udp_sk) {
	int segnale = 1;
	int app, ret;
	app = htonl(segnale);
	ret = sendto(udp_sk, (void*)&app, sizeof(int), 0, (struct sockaddr*)&avvers_addr, sizeof(avvers_addr));
	if(ret == -1) {
		perror("sendto error");
		exit(1);
	}
	map_init();
}

void disconnetti(int sk, int udp_sk) {
	int ret; // comando = 10;	//comando di disconnessione 
	int app;
	/*app = htonl(comando);
	ret = send(sk, (void*)&app, sizeof(int), 0);
	get_error(ret);*/
	//notifica disconnessione all'avversario
	int notifica = 1;
	app = htonl(notifica);
	ret = sendto(udp_sk, (void*)&app, sizeof(int), 0, (struct sockaddr*)&avvers_addr, sizeof(avvers_addr));
	if(ret == -1) {
		perror("sendto error");
		exit(1);	
	}
	printf("Disconnessione avvenuta con successo: TI SEI ARRESO\n");
}

void show_map() {
	int i, j, k;
	printf("\n");
	for(i = 2; i >= 0; i--) {
		printf(" ");
		for(j = 1; j <= 3; j++) {
			k = 3*i + j;
			k --;
			printf("%c ", mappa[k]);	
		}
		printf("\n");
	}
	printf("\n");
}

int get_winner() {
	int i = 0, j;
	//controllo vincita per righe
	while(i < 9) {
		if((mappa[i] == my_mark) && (mappa[i+1] == my_mark) 
			&& (mappa[i+2] == my_mark)) {
				return 1;
		}
		i += 3;
	}
	//controllo vincita per colonne
	for(j = 0; j < 3; j ++) {
		if((mappa[j] == my_mark) && (mappa[3+j] == my_mark)
			&& (mappa[6+j] == my_mark)) {
				return 1;
		}
	}
	//controllo diagonali
	if((mappa[0] == my_mark) && (mappa[4] == my_mark) && (mappa[8] == my_mark)){
		return 1;
	}
	if((mappa[2] == my_mark) && (mappa[4] == my_mark) && (mappa[6] == my_mark)) {
		return 1;
	}
	if((num_mosse == 5) && (num_avv == 4)) {
		return 2;
	}
	return 0;
}

void hit(int sock, int udp_sock, char* avvers) {
	int app, ret;
	int cella, comando;
	scanf("%i", &cella);
	if((cella < 1) || (cella > 9)) {
		printf("Cella non valida\n");
		return;
	}
	cella--;
	if(mappa[cella] == '_') {
		mappa[cella] = my_mark;
	}
	else {
		printf("La cella è già occupata\n");
		return;
	}
	num_mosse++;
	//Invio all'avversario la cella che ho colpito
	comando = 2;
	app = htonl(comando);
	ret = sendto(udp_sock, (void*)&app, sizeof(int), 0, (struct sockaddr*)&avvers_addr, sizeof(avvers_addr));
	if(ret == -1) {
		printf("sendto error");
		exit(1);	
	}
	app = htonl(cella);
	ret = sendto(udp_sock, (void*)&app, sizeof(int), 0, (struct sockaddr*)&avvers_addr, sizeof(avvers_addr));
	if(ret == -1) {
		printf("sento error");
		exit(1);
	}
	turno = 0;
	if(get_winner() == 1) {
		printf("HAI VINTO\n");
		//invio messaggio di vincita all'avversario
		comando = 3;
		app = htonl(comando);
		ret = sendto(udp_sock, (void*)&app, sizeof(int), 0, (struct sockaddr*)&avvers_addr, sizeof(avvers_addr));
		if(ret == -1) {
			printf("sendto error");
			exit(1);
		}	
		in_game = 0;
		//Devo avvertire il server che la partita è finita
		comando = 11;
		app = htonl(comando);
		ret = send(sock, (void*)&app, sizeof(int), 0);
		get_error(ret);
		//invio il nome dell'avversario che ha perso
		int lname = strlen(avvers);
		app = htonl(lname);
		ret = send(sock, (void*)&app, sizeof(int), 0);
		get_error(ret);
		ret = send(sock, (void*)avvers, lname, 0);
		get_error(ret);
		return;
	} else if(get_winner() == 2) {	//pareggio
		printf("PAREGGIO\n");
		comando = 4;
		app = htonl(comando);
		ret = sendto(udp_sock, (void*)&app, sizeof(int), 0, (struct sockaddr*)&avvers_addr, sizeof(avvers_addr));
		if(ret == -1) {
			printf("sendto error");
			exit(1);
		}	
		in_game = 0;
		comando = 12;
		app = htonl(comando);
		ret = send(sock, (void*)&app, sizeof(int), 0);
		get_error(ret);
		//invio il nome dell'avversario che ha perso
		int lname = strlen(avvers);
		app = htonl(lname);
		ret = send(sock, (void*)&app, sizeof(int), 0);
		get_error(ret);
		ret = send(sock, (void*)avvers, lname, 0);
		get_error(ret);
		return;
	}
	printf("E' il turno di %s\n", avvers);
}

int main(int argc, char** argv) {
        //VARIABILI
        char nome[DIM];
        int porta_udp;
        int sk, ret;    //descrittore socket e variabile per controllo errori
        struct sockaddr_in server_addr;
        int port;	//appoggio per numero di porta argv[1]
        char comando[DIM];
	char avvers[DIM];
	int fd_in;	//file descriptor dello standard input
        int fdmax;
	int udp_sk;	//socket UDP
	struct timeval timeout;

	//CODICE
        if(argc != 3) {
                fprintf(stderr, "Errore nel passaggio parametri\n");
                exit(1);
        }

	//Stabilisco connessione TCP col server 
        sk = socket(AF_INET, SOCK_STREAM, 0);
        if(sk < 0) {
                perror("Client socket error!");
                exit(1);
        }

	memset(&server_addr, 0, sizeof(server_addr));
       	port = atoi(argv[2]);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        ret = inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);
	if(ret == -1) {
                perror("Error");
                exit(1);
        }

        ret = connect(sk, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if(ret == -1){
                perror("Connection failed!");
                exit(1);
        }

	printf("Connessione al server %s (porta %s) effettuata con successo\n", argv[1],
                                                        argv[2]);

	FD_ZERO(&master);
        FD_ZERO(&read_fds);

	fd_in = fileno(stdin);
	FD_SET(sk, &master);
        FD_SET(fd_in, &master);

	if(sk > fd_in) {
		fdmax = sk;
	} else {
		fdmax = fd_in;
	}

	stampa_comandi();

	// Fase di autenticazione
		printf("Inserisci il tuo nome: ");
		fflush(stdout);
		read_fds = master;
		ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
                if(ret == -1) {
                        perror("Client select error!");
                        exit(1);
                }

                if(FD_ISSET(fd_in, &read_fds)) {
                        scanf("%s", nome);
                
			int lname = strlen(nome);
			int app = htonl(lname);
			// invio al server la lunghezza della stringa ricevuta
			ret = send(sk, (void*)&app, sizeof(app), 0);
			get_error(ret);

			// invio nome
			ret = send(sk, (void*)nome, lname, 0);
			get_error(ret);

			printf("Inserisci la porta UDP di ascolto: ");
			fflush(stdout);
		}
		
		int ok = 1;
		while(ok) {
			ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
                	if(ret == -1) {
                        	perror("Client select error!");
                       	 	exit(1);
                	}

			if(FD_ISSET(fd_in, &read_fds)) {
                        	scanf("%d", &porta_udp);
                		
				if(porta_udp > 1023 && (porta_udp != port)) {
					//invio porta UDP al server
					int app = htonl(porta_udp);
					ret = send(sk, (void*)&app, sizeof(int), 0);
					get_error(ret);
					ok = 0;
				} else {
					printf("Porta UDP non valida\n");
					printf("Inserisci la porta UDP di ascolto: ");
					fflush(stdout);
				}
			}
		}//fine while porta udp
	//Fine autenticazione
	//Creazione socket UDP
	udp_sk = socket(AF_INET, SOCK_DGRAM, 0);
	if(udp_sk == -1) {
		perror("Socket UDP error");
		exit(1);
	}

	memset(&udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(porta_udp);
	udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	ret = bind(udp_sk, (struct sockaddr*)&udp_addr, sizeof(udp_addr));
	if(ret == -1) {
		perror("UDP bind error");
		exit(1);
	}
	FD_SET(udp_sk, &master);
	if(udp_sk > fdmax) {
		fdmax = udp_sk;
	}
	//Termina creazione socket UDP

	//Invio comandi base
	in_game = 0;
	int terminato = 0;
	while(!terminato){
		while(in_game == 0) {
			printf("> ");
			fflush(stdout);
			read_fds = master;
			ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
			if(ret == -1) {
				perror("Client select error!");
				exit(1);
			}
			
			if(FD_ISSET(fd_in, &read_fds)) {
				scanf("%s", comando);
				
				if(strcmp(comando, "!help") == 0) {
					stampa_comandi();
					continue;
						}
				if(strcmp(comando, "!who") == 0) {
					who(sk);	//codice 1
					continue;
				}
				if(strcmp(comando, "!quit") == 0) {
					terminato = 1;
					quit(sk, udp_sk, NULL);	//codice 0
					break;
				}
				if(strcmp(comando, "!connect") == 0) {
					scanf("%s", avvers);
					connetti(sk, avvers);	//codice 2
					continue;
				} else {
					printf("Comando errato\n");
				}
			} else if(FD_ISSET(sk, &read_fds)) {
				int richiesta, app, len;
				ret = recv(sk, (void*)&app, sizeof(int), 0);
				get_error(ret);
				if(ret == 0) {
					printf("Il server si è disconnesso\n");
					exit(1);
				}
				richiesta = ntohl(app);
				
				//Ricevo il nome dell'avversario
				int accettato; 		
				if(richiesta == 1){
					memset(&avvers, 0, sizeof(avvers));
					ret = recv(sk, (void*)&app, sizeof(int), 0);
					get_error(ret);
					len = ntohl(app);
					ret = recv(sk, (void*)avvers, len, 0);
					get_error(ret);
					printf("%s vuole iniziare una partita, accetti[s/n]? ", avvers);
					fflush(stdout);
					char yesno;
					for(;;) {
						scanf("%s", &yesno);
						if(yesno == 's') {
							accettato = 1;
							break;
						} else if(yesno == 'n') {
							accettato = 0;
							break;			
						}
						printf("Risposta non valida\n");
						printf("%s vuole iniziare una partita, accetti[s/n]? ", avvers);
					}
					app = htonl(accettato);
					ret = send(sk, (void*)&app, sizeof(int), 0);
					get_error(ret);
					
				} else if(richiesta == 2) {
					// ricevo IP e udp_port dell'avversario
					int avv_udp;
					char avv_addr[36];
					memset(&avv_addr, 0, sizeof(avv_addr));
					ret = recv(sk, (void*)&app, sizeof(int), 0);
					get_error(ret);
					len = ntohl(app);
					ret = recv(sk, (void*)avv_addr, len, 0);
					get_error(ret);
				
					ret = recv(sk, (void*)&app, sizeof(int), 0);
					get_error(ret);
					avv_udp = ntohl(app);
					//printf("IPavv: %s, UDPavv: %i\n", avv_addr, avv_udp);
					avvers_addr.sin_addr.s_addr = inet_addr(avv_addr);
					avvers_addr.sin_port = htons(avv_udp);
					my_mark = 'X';
					avv_mark = 'O';
					num_mosse = 0;
					num_avv = 0;
					in_game = 1;
					printf("%s ha accettato la partita\n", avvers);
					printf("E' il tuo turno\n");
					printf("Il tuo simbolo è %c\n", my_mark); //simbolo X
					//contatto l'avversario tramite UDP
					turno = 1;
					contatta(udp_sk);
					
				} else if(richiesta == 3) {
					printf("Richiesta di gioco rifiutata\n");
				}
			} else if(FD_ISSET(udp_sk, &read_fds)) {
				int segnale, app;
				socklen_t udplen = sizeof(avvers_addr);
				ret = recvfrom(udp_sk, (void*)&app, sizeof(int), 0, (struct sockaddr*)&avvers_addr, &udplen);
				if(ret == -1) {
					perror("recvfrom error");
					exit(1);
				}
				segnale = ntohl(app);
				if(segnale == 1) {
					in_game = 1;
					map_init();
					avv_mark = 'X';
					my_mark = 'O';
					num_mosse = 0;
					num_avv = 0;
					printf("Inizio partita con %s\n", avvers);
					printf("E' il turno di %s\n", avvers);
					printf("Il tuo simbolo è %c\n", my_mark);
				}
			}
		} // fine while non in gioco
		
		while(in_game == 1) {
			printf("# ");
			fflush(stdout);
			read_fds = master;
			timeout.tv_sec = 60;
	    		timeout.tv_usec=0;
			ret = select(fdmax + 1, &read_fds, NULL, NULL, &timeout);
			if(ret == -1) {
				perror("Client select error!");
				exit(1);
			}
			if(ret == 0) {
				printf("TIMEOUT SCADUTO\n");
				in_game = 0;
				disconnetti(sk, udp_sk);
				break;
			}
			
			
			if(FD_ISSET(fd_in, &read_fds)) {
				scanf("%s", comando);
		
				if(strcmp(comando, "!disconnect") == 0) {
					in_game = 0;
					disconnetti(sk, udp_sk);  //passo il socket TCP e UDP
					continue;
				}
				if(strcmp(comando, "!quit") == 0) {
					terminato = 1;
					quit(sk, udp_sk, avvers);	
					break;
				}
				if(strcmp(comando, "!hit") == 0) {
					if(turno == 0) {
						printf("Attendi il tuo turno\n");
						continue;
					}
					hit(sk, udp_sk, avvers);
					continue;
				}
				if(strcmp(comando, "!show_map") == 0) {
					show_map();
					continue;
				} else {
					printf("Comando errato\n");
				}
			} else if(FD_ISSET(udp_sk, &read_fds)) {
				int notifica, app;
				socklen_t udplen = sizeof(avvers_addr);
				ret = recvfrom(udp_sk, (void*)&app, sizeof(int), 0, (struct sockaddr*)&avvers_addr, &udplen);
				if(ret == -1) {
					perror("recvfrom error");
					exit(1);
				}
				//gestire il caso in cui un client cade durante il gioco
				notifica = ntohl(app);
				if(notifica == 1) {
					//invio al server un messggio per comunicare che sono libero
					int libero = 10;
					app = htonl(libero);
					ret = send(sk, (void*)&app, sizeof(int), 0);
					get_error(ret);
					int lname = strlen(avvers);
					app = htonl(lname);
					ret = send(sk, (void*)&app, sizeof(int), 0);
					get_error(ret);
					ret = send(sk, (void*)avvers, lname, 0);
					get_error(ret);
					printf("%s si è disconnesso: HAI VINTO\n", avvers);
					in_game = 0;
				}
				if(notifica == 2) {
					//ricevo la cella
					num_avv ++; 	//numero mosse avversario
					//L'avversario ha colpito una cella
					int cella;
					ret = recvfrom(udp_sk, (void*)&app, sizeof(int), 0, (struct sockaddr*)&avvers_addr, &udplen);
					if(ret == -1) {
						perror("recvfrom error");
						exit(1);
					}
					cella = ntohl(app);
					mappa[cella] = avv_mark;
					cella++;
					printf("%s ha marcato la casella %i\n", avvers, cella);
					turno = 1;
					printf("E' il tuo turno\n");
				}
				if(notifica == 3) {
					//ho perso
					printf("HAI PERSO\n");
					in_game = 0;
				}
				if(notifica == 4) {
					printf("PAREGGIO\n");
					in_game = 0;
				}
			}
		} //fine while in gioco
	}
	return 0;
}
