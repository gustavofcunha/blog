#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "common.h"

typedef struct dadosThread {
    int sock;
    int *estadoConexao;
} dadosThread;

void trataComandoCliente(char *bufTerminal, int *estadoConexao, int sock, int id) {
    char topico[LIMITE_TITULO_TOPICO];
    char textoTopico[LIMITE_BUFFER];
    BlogOperation operacaoEnviar;

    if (strncmp(bufTerminal, "publish in ", 11) == 0) {
        sscanf(bufTerminal, "publish in %[^\n]", topico);
        fgets(textoTopico, sizeof(textoTopico), stdin);

        operacaoEnviar.client_id = id;
        operacaoEnviar.operation_type = 2;
        operacaoEnviar.server_response = 0;
        strcpy(operacaoEnviar.topic, topico);
        strcpy(operacaoEnviar.content, textoTopico);

        size_t count = send(sock, &operacaoEnviar, sizeof(BlogOperation), 0);
        if (count != sizeof(BlogOperation)) {
            logexit("send");
        }
    }

    else if (strncmp(bufTerminal, "list topics", 11) == 0) {
        operacaoEnviar.client_id = id;
        operacaoEnviar.operation_type = 3;
        operacaoEnviar.server_response = 0;
        strcpy(operacaoEnviar.topic, "");
        strcpy(operacaoEnviar.content, "");

        size_t count = send(sock, &operacaoEnviar, sizeof(BlogOperation), 0);
        if (count != sizeof(BlogOperation)) {
            logexit("send");
        }

        BlogOperation retornoListaTopicos;
        recv(sock, &retornoListaTopicos, sizeof(BlogOperation), 0);

        printf("%s\n", retornoListaTopicos.content);
        fflush(stdout);
    }

    else if (strncmp(bufTerminal, "subscribe in ", 13) == 0) {
        sscanf(bufTerminal, "subscribe in %[^\n]", topico);

        operacaoEnviar.client_id = id;
        operacaoEnviar.operation_type = 4;
        operacaoEnviar.server_response = 0;
        strcpy(operacaoEnviar.topic, topico);
        strcpy(operacaoEnviar.content, "");

        size_t count = send(sock, &operacaoEnviar, sizeof(BlogOperation), 0);
        if (count != sizeof(BlogOperation)) {
            logexit("send");
        }

        BlogOperation retornoInscricao;
        recv(sock, &retornoInscricao, sizeof(BlogOperation), 0);

        if (strncmp(retornoInscricao.content, "error", 5) == 0) {
            printf("%s\n", retornoInscricao.content);
        }
    }

    else if (strncmp(bufTerminal, "exit", 4) == 0) {
        *estadoConexao = 0;

        operacaoEnviar.client_id = id;
        operacaoEnviar.operation_type = 5;
        operacaoEnviar.server_response = 0;
        strcpy(operacaoEnviar.topic, "");
        strcpy(operacaoEnviar.content, "");

        size_t count = send(sock, &operacaoEnviar, sizeof(BlogOperation), 0);
        if (count != sizeof(BlogOperation)) {
            logexit("send");
        }
    }

    else if (strncmp(bufTerminal, "unsubscribe ", 12) == 0) {
        sscanf(bufTerminal, "unsubscribe %[^\n]", topico);

        operacaoEnviar.client_id = id;
        operacaoEnviar.operation_type = 6;
        operacaoEnviar.server_response = 0;
        strcpy(operacaoEnviar.topic, topico);
        strcpy(operacaoEnviar.content, "");

        size_t count = send(sock, &operacaoEnviar, sizeof(BlogOperation), 0);
        if (count != sizeof(BlogOperation)) {
            logexit("send");
        }
    }

    return;
}

void *recebeNotificacoes(void *data) {
    dadosThread *dados = (struct dadosThread *)data;
    int *estadoConexao = (int *)(dados->estadoConexao);

    BlogOperation notificacaoRecebida;

    while (*estadoConexao) {
        recv(dados->sock, &notificacaoRecebida, sizeof(BlogOperation), 0);

        if (notificacaoRecebida.operation_type == 2 && notificacaoRecebida.server_response) {
            if (*estadoConexao) {
                printf("\nnew post added in %s by %d",
                       notificacaoRecebida.topic, notificacaoRecebida.client_id);
                printf("\n%s\n", notificacaoRecebida.content);
                fflush(stdout);
            }
        } else {
            continue;
        }
    }

    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    const char *enderecoIPServidor = argv[1];
    const char *porta = argv[2];

    struct sockaddr_storage storage;
    if (addrparse(enderecoIPServidor, porta, &storage) != 0) {
        logexit("entradas");
    }

    int sock = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sock == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);

    if (connect(sock, addr, sizeof(storage)) != 0) {
        logexit("connect");
    }

    int estadoConexao = 1;
    int id;

    BlogOperation operacao;
    operacao.client_id = 0;
    operacao.operation_type = 1;
    operacao.server_response = 0;
    strcpy(operacao.topic, "");
    strcpy(operacao.content, "");

    size_t count = send(sock, &operacao, sizeof(BlogOperation), 0);
    if (count != sizeof(BlogOperation)) {
        logexit("send");
    }

    BlogOperation operacaoRetorno;
    recv(sock, &operacaoRetorno, sizeof(BlogOperation), 0);
    if (operacaoRetorno.operation_type == 1 && operacaoRetorno.server_response == 1) {
        id = operacaoRetorno.client_id;
    } else {
        logexit("obtenção de id");
    }

    dadosThread *threadNotificacao = malloc(sizeof(dadosThread));
    threadNotificacao->estadoConexao = &estadoConexao;
    threadNotificacao->sock = sock;

    pthread_t idThreadNotificacoes;
    pthread_create(&idThreadNotificacoes, NULL, recebeNotificacoes, threadNotificacao);

    char bufTerminal[LIMITE_BUFFER];

    while (estadoConexao) {
        fgets(bufTerminal, sizeof(bufTerminal), stdin);
        trataComandoCliente(bufTerminal, &estadoConexao, sock, id);
    }

    pthread_join(idThreadNotificacoes, NULL);

    return 0;
}
