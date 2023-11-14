#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void logexit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portaStr, 
              struct sockaddr_storage *storage){
    if(addrstr == NULL || portaStr == NULL){
        return -1;
    }

    uint16_t porta = (uint16_t)atoi(portaStr);
    if(porta == 0){
        return -1;
    }
    //host to network short
    porta = htons(porta);

    struct in_addr inaddr4; //32-bit IP address (IPV4)
    if(inet_pton(AF_INET, addrstr, &inaddr4)){
        struct sockaddr_in *addr4 = (struct sockaddr_in *)(storage);
        addr4-> sin_family = AF_INET;
        addr4->sin_port = porta;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; //128-bit IP address (IPV6)
    if(inet_pton(AF_INET6, addrstr, &inaddr6)){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)(storage);
        addr6-> sin6_family = AF_INET6;
        addr6->sin6_port = porta;
        addr6->sin6_addr = inaddr6;
        //memcpy(&(addr6->sin6_addr), inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize){
    
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1];
    uint16_t porta;

    if(addr->sa_family == AF_INET){
        version = 4;
        struct sockaddr_in *addr4 = (struct socktaddr_in *)(addr);
        if(!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1)){
            logexit("ntop");
        }
        porta = ntohs(addr4->sin_port); //network to host short
    }
    else if(addr->sa_family == AF_INET6){
        version = 6;
        struct sockaddr_in6 *addr6 = (struct socktaddr_in6 *)(addr);
        if(!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)){
            logexit("ntop");
        }
        porta = ntohs(addr6->sin6_port); //network to host short
    }
    else{
        logexit("unknown protocol family");
    }
    snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, porta);

}

int server_sockaddr_init(char *tipoEndereco, char *portaStr, 
                          struct sockaddr_storage *storage){
                       
    uint16_t porta = (uint16_t)atoi(portaStr);
    if(porta == 0){
        return -1;
    }
    
    //host to network short
    porta = htons(porta);
    
    
    memset(storage, 0, sizeof(*storage));

    //se for protocolo ipv4
    if(strcmp(tipoEndereco, "v4") == 0){
        struct sockaddr_in *addr4 = (struct socktaddr_in *)(storage);
        addr4-> sin_family = AF_INET;
        addr4->sin_port = porta;
        addr4->sin_addr.s_addr = INADDR_ANY;
        return 0;
    }

    //se for protocolo ipv6
    else if(strcmp(tipoEndereco, "v6") == 0){
        struct sockaddr_in6 *addr6 = (struct socktaddr_in6 *)(storage);
        addr6-> sin6_family = AF_INET6;
        addr6->sin6_port = porta;
        addr6->sin6_addr = in6addr_any;
        return 0;
    }

    else{
        return -1;
    }
     
}

char intToChar(int valor) {
    char caractere;
    
    if (valor == -1) {
        caractere = '*';
    } else if (valor == -2) {
        caractere = '-';
    } else if (valor == -3) {
        caractere = '>';
    } else {
        // Se o valor for um número, convertemos para caractere
        caractere = (char)('0' + valor);
    }
    
    return caractere;
}