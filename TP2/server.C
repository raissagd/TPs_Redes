/*
-----------------------------------------------------
Arquivo server.c referente ao TP2
Implementacao do servidor, ou Unidade Controladora
Materia Redes de Computadores
Semestre 2025/01

Aluna: Raissa Gonçalves Diniz
Matricula: 2022055823
-----------------------------------------------------
*/
#include "common.H"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 1024

void usage (int argc, char **argv) {
    printf("Usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("Example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

// Função que será executada por cada thread
void * client_thread_handler(void *socket_desc_ptr) {
    // Extrai o descritor do socket do argumento e libera a memória
    int csock = *(int*)socket_desc_ptr;
    free(socket_desc_ptr);

    char caddrstr[BUFSZ];
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)&cstorage;
    socklen_t caddrlen = sizeof(cstorage);

    // Obtém informações do cliente conectado para log
    getpeername(csock, caddr, &caddrlen);
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("Thread de cliente iniciada para %s\n", caddrstr);

    // Lógica de comunicação com o cliente dentro de um loop
    char buf[BUFSZ];
    size_t count;
    while((count = recv(csock, buf, BUFSZ - 1, 0)) > 0) {
        buf[count] = '\0'; // Garante que a string seja terminada
        printf("[msg] %s, %d bytes: %s", caddrstr, (int)count, buf);
        
        // Ecoa a mesma mensagem de volta para o cliente
        if((size_t)send(csock, buf, count, 0) != count) {
            logexit("send");
        }
    }
    
    close(csock);
    printf("Conexão com %s encerrada.\n", caddrstr);
    
    return NULL;
}

int main (int argc, char **argv) {
    // Verifica se os argumentos necessários foram fornecidos
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    // Preenche a estrutura de endereço do servidor (IPv4 ou IPv6)
    if (0 != server_sockaddr(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    // Cria o socket TCP
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }
    
    int enable = 1;
    // Permite reutilizar o endereço local (evita erro ao reiniciar o servidor rapidamente)
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        logexit("setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr *addr = (struct sockaddr *)&storage;
    // Associa o socket ao endereço e porta especificados
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    // Coloca o socket em modo de escuta para aceitar conexões (máx 10 na fila)
    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    // Converte o endereço do servidor para string e exibe
    addrtostr(addr, addrstr, BUFSZ);
    printf("Bound to %s, waiting connections\n", addrstr);

    while(1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)&cstorage;
        socklen_t caddrlen = sizeof(cstorage);

        // Aceita uma nova conexão de cliente
        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        char caddrstr[BUFSZ];
        addrtostr(caddr, caddrstr, BUFSZ);
        printf("Accepted connection from %s\n", caddrstr);
        
        pthread_t thread_id;
        // Aloca memória para passar o descritor do socket ao thread
        int *p_csock = (int *)malloc(sizeof(int));
        *p_csock = csock;

        // Cria uma nova thread para atender o cliente
        if (pthread_create(&thread_id, NULL, client_thread_handler, p_csock) < 0) {
            logexit("could not create thread");
        }
        
        pthread_detach(thread_id); // Libera recursos da thread automaticamente ao terminar
    }

    exit(EXIT_SUCCESS);
}