#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "common.h"

#define LIMITE_TOPICOS 10
#define LIMITE_CLIENTES 10

typedef struct Publicacao{
    int idCliente;
    char conteudo[LIMITE_BUFFER];
}Publicacao;

typedef struct Topico{
    char nomeTopico[LIMITE_TITULO_TOPICO];
}Topico;

typedef struct Inscricao{
    int idCliente;
    char nomeTopico[LIMITE_TITULO_TOPICO];
    int valida;
}Inscricao;

typedef struct dadosThread{
    int idCliente;
    int clientSock;
    struct sockaddr_storage clientStorage;
    pthread_mutex_t mutexTratamentoOperacao;
}dadosThread;

Topico topicos[LIMITE_TOPICOS];
int qtdeTopicos = 0;

Inscricao inscricoes[LIMITE_TOPICOS];
int qtdeInscricoes = 0;

dadosThread *threadsClientes[LIMITE_CLIENTES];
int listaIds[LIMITE_CLIENTES];

Topico* procuraTopico(char nomeTopico[LIMITE_TITULO_TOPICO]){
    int tamanho = strlen(nomeTopico);

    for(int i = 0; i<qtdeTopicos; i++){
        if(strncmp(topicos[i].nomeTopico, nomeTopico, tamanho) == 0){
            return &topicos[i];
        }
    }

    return NULL;
}

int buscaSocket(int idCliente){
    for(int i = 0; i<LIMITE_CLIENTES; i++){
        if(threadsClientes[i]->idCliente == idCliente){
            return threadsClientes[i]->clientSock;
        }
    }

    return -99;
}

void notificaInscritos(Topico topico, Publicacao publicacao){
    int tamanho = strlen(topico.nomeTopico);
    int sock;
    BlogOperation operacaoNotificacaoNovoPost;

    for(int i = 0; i<qtdeInscricoes; i++){
        //match topico
        if(strncmp(inscricoes[i].nomeTopico, topico.nomeTopico, tamanho) == 0){
            //se inscricao for valida
            if(inscricoes[i].valida){
                //obtem socket do cliente
                sock = buscaSocket(inscricoes[i].idCliente);
                //se cliente nao esta mais conectado, nao envia
                if(sock == -99){
                    continue;
                }

                //inicializa operacao de notificacao
                operacaoNotificacaoNovoPost.client_id = publicacao.idCliente;
                operacaoNotificacaoNovoPost.operation_type = 2;
                operacaoNotificacaoNovoPost.server_response = 1;
                strcpy(operacaoNotificacaoNovoPost.topic, topico.nomeTopico);
                strcpy(operacaoNotificacaoNovoPost.content, publicacao.conteudo);

                //envia para o cliente
                size_t count = send(sock, &operacaoNotificacaoNovoPost, sizeof(BlogOperation), 0);
                if(count != sizeof(BlogOperation)){
                    logexit("send");
                }
            }
        }
    }
}

void inscreveClienteTopico(Topico topico, dadosThread *dadosCliente){

    inscricoes[qtdeInscricoes].idCliente = dadosCliente->idCliente;
    strcpy(inscricoes[qtdeInscricoes].nomeTopico, topico.nomeTopico);
    inscricoes[qtdeInscricoes].valida = 1;
    pthread_mutex_lock(&dadosCliente->mutexTratamentoOperacao);
    qtdeInscricoes++;
    pthread_mutex_unlock(&dadosCliente->mutexTratamentoOperacao);
}

int desinscreveClienteTopico(int idCliente, char nomeTopico[LIMITE_TITULO_TOPICO]){
    int i;
    int tamanho = strlen(nomeTopico);

    for(i=0; i<qtdeInscricoes; i++){
        if(inscricoes[i].valida){
            if(strncmp(inscricoes[i].nomeTopico, nomeTopico, tamanho) == 0){
                if(inscricoes[i].idCliente == idCliente){
                    inscricoes[i].valida = 0;
                    return 1;
                }   
            }
        }
        else{
            continue;
        }
    }

    return 0;
}

int clienteJaInscrito(int idCliente, char nomeTopico[LIMITE_TITULO_TOPICO]){
    int i;
    int tamanho = strlen(nomeTopico);

    for(i=0; i<qtdeInscricoes; i++){
        if(inscricoes[i].valida){
            if(strncmp(inscricoes[i].nomeTopico, nomeTopico, tamanho) == 0){
                if(inscricoes[i].idCliente == idCliente){
                    return 1;
                }   
            }
        }
        else{
            continue;
        }
    }

    return 0;
}

void desconectaCliente(int idCliente){
    int i;
 
    for(i=0; i<qtdeInscricoes; i++){
        if(inscricoes[i].valida){  
            if(inscricoes[i].idCliente == idCliente){
                inscricoes[i].valida = 0;
            }   
        }
    }

    listaIds[idCliente-1] = 0;
}

void trataOperacao(dadosThread *dadosCliente, BlogOperation operacaoRecebida, int *estadoConexao){
    int i, posicao;
    BlogOperation operacaoRetorno;
    Topico *topicoEncontrado;
    Publicacao novaPublicacao;

    //se for operacao de nova conexao
    if(operacaoRecebida.operation_type == 1){
        BlogOperation operacaoResposta;

        operacaoResposta.client_id = dadosCliente->idCliente;
        operacaoResposta.operation_type = 1;
        operacaoResposta.server_response = 1;
        strcpy(operacaoResposta.topic, "");
        strcpy(operacaoResposta.content, "");

        size_t count = send(dadosCliente->clientSock, &operacaoResposta, sizeof(BlogOperation), 0);
        if(count != sizeof(BlogOperation)){
            logexit("send");
        }
    }

    //se for nova publicacao
    else if(operacaoRecebida.operation_type == 2){
        printf("\nnew post added in %s by %d", 
               operacaoRecebida.topic, operacaoRecebida.client_id);
        fflush(stdout); //forca impressao imediata

        topicoEncontrado = procuraTopico(operacaoRecebida.topic);
        
        //se o topico nao existe, cria um novo e inscreve o cliente
        if(topicoEncontrado == NULL){
            //inicializa novo topico e nova publicacao
            strcpy(topicos[qtdeTopicos].nomeTopico, operacaoRecebida.topic);
            pthread_mutex_lock(&dadosCliente->mutexTratamentoOperacao);
            qtdeTopicos++;
            pthread_mutex_unlock(&dadosCliente->mutexTratamentoOperacao);

            novaPublicacao.idCliente = operacaoRecebida.client_id;
            strcpy(novaPublicacao.conteudo, operacaoRecebida.content);

            //nao ha ninguem para notificar 
            //cliente nao eh inscrito em topico que criou automaticamente
        }

        //se o topico existe, publica nele
        else{
            novaPublicacao.idCliente = operacaoRecebida.client_id;
            strcpy(novaPublicacao.conteudo, operacaoRecebida.content);

            notificaInscritos(*topicoEncontrado, novaPublicacao);
        }
    }

    //se for listar topicos
    else if(operacaoRecebida.operation_type == 3){
        
        //se nao houverem topicos para listar
        if(qtdeTopicos == 0){
            operacaoRetorno.client_id = operacaoRecebida.client_id;
            operacaoRetorno.server_response = 1;
            operacaoRetorno.operation_type = operacaoRecebida.operation_type;
            strcpy(operacaoRetorno.content, "no topics available");
            strcpy(operacaoRetorno.topic, "");

            size_t count = send(dadosCliente->clientSock, &operacaoRetorno, sizeof(BlogOperation), 0);
            if(count != sizeof(BlogOperation)){
                logexit("send");
            }

            return;
        }

        posicao = 0;
        char listaTopicos[LIMITE_TITULO_TOPICO*LIMITE_TOPICOS*2];

        for(i=0; i<qtdeTopicos; i++){
            strcpy(listaTopicos + posicao, topicos[i].nomeTopico);
            posicao += strlen(topicos[i].nomeTopico);

            if (i < qtdeTopicos - 1) {
                listaTopicos[posicao] = ';';
                listaTopicos[posicao + 1] = ' ';
                posicao = posicao + 2;
            }
        }

        listaTopicos[posicao] = '\0';

        operacaoRetorno.client_id = operacaoRecebida.client_id;
        operacaoRetorno.server_response = 1;
        operacaoRetorno.operation_type = operacaoRecebida.operation_type;
        strcpy(operacaoRetorno.content, listaTopicos);
        strcpy(operacaoRetorno.topic, "");

        size_t count = send(dadosCliente->clientSock, &operacaoRetorno, sizeof(BlogOperation), 0);
        if(count != sizeof(BlogOperation)){
            logexit("send");
        }

        memset(listaTopicos, 0, sizeof(listaTopicos));
    }

    //se for pedido de inscricao
    else if(operacaoRecebida.operation_type == 4){
        Topico *topicoEncontrado = procuraTopico(operacaoRecebida.topic);

        //se o topico nao existe, cria um novo e inscreve o cliente
        if(topicoEncontrado == NULL){
            printf("\nclient %d subscribed to %s", 
                        operacaoRecebida.client_id, operacaoRecebida.topic);
            fflush(stdout); //forca impressao imediata

            //inicializa novo topico e nova publicacao
            strcpy(topicos[qtdeTopicos].nomeTopico, operacaoRecebida.topic);
            pthread_mutex_lock(&dadosCliente->mutexTratamentoOperacao);
            qtdeTopicos++;
            pthread_mutex_unlock(&dadosCliente->mutexTratamentoOperacao);

            novaPublicacao.idCliente = operacaoRecebida.client_id;
            strcpy(novaPublicacao.conteudo, operacaoRecebida.content);
            

            //inscreve cliente que criou o topico
            inscreveClienteTopico(topicos[qtdeTopicos-1], dadosCliente);
        }

        //se o topico existe, increve o cliente
        else{
            //se o cliente nao for inscrito, inscreve-o
            if(!clienteJaInscrito(operacaoRecebida.client_id, operacaoRecebida.topic)){
                printf("\nclient %d subscribed to %s", 
                        operacaoRecebida.client_id, operacaoRecebida.topic);
                fflush(stdout); //forca impressao imediata
                inscreveClienteTopico(*topicoEncontrado, dadosCliente);
            }

            //se o cliente ja for inscrito envia mensagem de falha
            else{
                operacaoRetorno.client_id = operacaoRecebida.client_id;
                operacaoRetorno.server_response = 1;
                operacaoRetorno.operation_type = operacaoRecebida.operation_type;
                strcpy(operacaoRetorno.content, "error: already subscribed");
                strcpy(operacaoRetorno.topic, operacaoRecebida.topic);

                size_t count = send(dadosCliente->clientSock, &operacaoRetorno, sizeof(BlogOperation), 0);
                if(count != sizeof(BlogOperation)){
                    logexit("send");
                }
                return;
            }
            
        }

        //mensagem de sucesso da inscricao
        operacaoRetorno.client_id = operacaoRecebida.client_id;
        operacaoRetorno.server_response = 1;
        operacaoRetorno.operation_type = operacaoRecebida.operation_type;
        strcpy(operacaoRetorno.content, "sucesso");
        strcpy(operacaoRetorno.topic, operacaoRecebida.topic);
        size_t count = send(dadosCliente->clientSock, &operacaoRetorno, sizeof(BlogOperation), 0);
        if(count != sizeof(BlogOperation)){
            logexit("send");
        }
        
    }

    //se for exit
    else if(operacaoRecebida.operation_type == 5){
        *estadoConexao = 0;

        //desconexao e impressao feita na funcao clientesThread apos return
        return;
    }

    //se for pedido de desinscricao
    else if(operacaoRecebida.operation_type == 6){
        //se a desinscricao ocorreu corretamente
        if(desinscreveClienteTopico(operacaoRecebida.client_id, operacaoRecebida.topic)){
            printf("\nclient %d unsubscribed to %s", 
                operacaoRecebida.client_id, operacaoRecebida.topic);
            fflush(stdout); //forca impressao imediata
        }
    }

    //operacao desconhecida
    else{
        return;
    }
}

void *clientesThread(void *data){
    int estadoConexao = 1;
    int bufRecv;
    
    dadosThread *dadosCliente = (struct dadosThread *)data;
    struct sockaddr *clientAddr = (struct sockaddr *)(&dadosCliente->clientStorage);
    BlogOperation operacaoRecebida;

    while (estadoConexao) {
        bufRecv = recv(dadosCliente->clientSock, &operacaoRecebida, sizeof(BlogOperation), 0);

        //caso hajam erro de comunicacao ou conexao encerrada
        if (bufRecv <= 0) {
            estadoConexao = 0;
            break;
        }

        //caso contrario, trata operacao
        else{
            trataOperacao(dadosCliente, operacaoRecebida, &estadoConexao);
            continue;
        }   
    }

    desconectaCliente(dadosCliente->idCliente);
    close(dadosCliente->clientSock);

    printf("\nclient %d was disconnected", dadosCliente->idCliente);    
    fflush(stdout); //forca impressao imediata

    free(dadosCliente);
    pthread_exit(EXIT_SUCCESS);
}

int obtemId(){
    int menorIdDisponivel = LIMITE_CLIENTES + 1;
    int i;

    for(i=0; i<LIMITE_CLIENTES; i++){
        //printf("\nid %d valido? %d\n", i+1, threadsClientes[i]->valida);
        if(!listaIds[i]){
            listaIds[i] = 1;
            return i+1;
        }
    }

    logexit("numero limite de ids atingido");
    return -1;
}

void inicializaEstruturas(){
    int i;
    for (i=0; i<LIMITE_CLIENTES; i++) {
        threadsClientes[i] = malloc(sizeof(dadosThread)); 
        if (!threadsClientes[i]) {
            logexit("malloc");
        }
        threadsClientes[i]->idCliente = i+1;
    }

    for (i=0; i<LIMITE_CLIENTES; i++) {
        listaIds[i] = 0;
    }

    for(i=0; i<LIMITE_TOPICOS; i++){
        inscricoes[i].valida = 0;
    }
}

void main(int argc, char *argv[]){

    //capturando argumentos de entrada
    char *tipoEndereco = argv [1];
    char *porta = (argv[2]);

    //converte as entradas em um endereco
    struct sockaddr_storage storage;
    if (server_sockaddr_init(tipoEndereco, porta, &storage) != 0){
        logexit("entradas");
    }
    
    //abre socket da internet, TCP
    int sock = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sock == -1){
        logexit("socket");
    }

    //seta socket para reutilizar portas
    int enable = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0){
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (bind(sock, addr, sizeof(storage)) != 0){
        logexit("bind");
    }

    if (listen(sock, 10) != 0){
        logexit("listen");
    }

    inicializaEstruturas();
    int estadoServidor = 1;
    int idObtido;

    //enquanto servidor estiver ativo
    while(estadoServidor){
        struct sockaddr_storage clientStorage;
        struct sockaddr *clientAddr = (struct sockaddr *)(&clientStorage);
        socklen_t clientAddrLen = sizeof(clientStorage);

        //espera e recebe novas conexoes
        int clientSock = accept(sock, (struct sockaddr *)&clientStorage, &clientAddrLen);
        if(clientSock == -1){
            logexit("accept");
        }
        
        idObtido = obtemId();
       
        dadosThread *dadosCliente = malloc(sizeof(*dadosCliente));
        if(!dadosCliente){
            logexit("malloc");
        }

        dadosCliente->idCliente = idObtido;
        dadosCliente->clientSock = clientSock;
        memcpy(&(dadosCliente->clientStorage), &clientStorage, sizeof(clientStorage));
        if(pthread_mutex_init(&dadosCliente->mutexTratamentoOperacao, NULL) != 0){
            logexit("mutex initialization failed");
        }

        threadsClientes[idObtido-1] = dadosCliente;
       
        printf("\nclient %d connected", dadosCliente->idCliente);
        fflush(stdout); //forca impressao imediata

        //dispara thread para tratar nova conexao
        pthread_t idThread;
        pthread_create(&idThread, NULL, clientesThread, dadosCliente);
    }
}