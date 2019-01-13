#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct client {
	char nome[36];
	int udp_port;
	int socket;
	int occupato;
	struct sockaddr_in ip_address;
	struct client* next;
};

struct client* testa = NULL;
struct client* coda = NULL;

void add(char* nome, int porta, int sk, struct sockaddr_in addr) {
	struct client* nuovo;
	nuovo = (struct client*)malloc(sizeof(struct client));
	strcpy(nuovo->nome, nome);
	nuovo->udp_port = porta;
	nuovo->occupato = 0;
	nuovo->socket = sk;
	nuovo->ip_address = addr;	// da rivedere
	nuovo->next = NULL;
        if(testa == NULL) {
                testa = nuovo;
        } else {
                coda->next = nuovo;
        }
        coda = nuovo;
}

void destroy(int sk) {
	struct client* p, *prec = 0;
	p = testa;
	while(p->socket != sk) {
		prec = p;
		p = p->next;
	}
	if(p == testa) {
		testa = testa->next;
	} else {
		prec->next = p->next;
	}
	free(p);
}

int size() {
	int ris;
	ris = 0;
	struct client* p;
	p = testa;
	while(p != NULL) {
		ris ++;
		p = p->next;
	}
	return ris;
}

struct client* search(char* nome, int sk) {
	struct client* p = testa;
	if(nome != NULL) {
		while(p != NULL) {
			if(strcmp(p->nome, nome) == 0) {
				return p;
			}
			p = p->next;
		}
	}
	if(sk != -1) {
		while(p != NULL) {
			if(p->socket == sk) {
				return p;
			}
			p = p->next;
		}
	}
	return p;
}
