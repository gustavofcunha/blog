#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define LIMITE_BUFFER 2048
#define LIMITE_TITULO_TOPICO 50

typedef struct BlogOperation {
    int client_id;
    int operation_type;
    int server_response;
    char topic[LIMITE_TITULO_TOPICO];
    char content[LIMITE_BUFFER];
}BlogOperation;

void logexit(char *msg);
int addrparse(const char *addrstr, const char *portaStr, struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int server_sockaddr_init(const char *tipoEndereco, char * portaStr, 
                          struct sockaddr_storage *storage);
char intToChar(int valor);

#endif