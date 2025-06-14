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
int32_t next_player_id  = 1; // Variável global para ID do jogador

// Struct para passar argumentos para a thread do cliente.
typedef struct {
    int client_socket;
    int32_t player_id;
} thread_args_t;

void usage (int argc, char **argv) {
    printf("Usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("Example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

// Função que será executada por cada thread
void * client_thread_handler(void *args_ptr) {
    // Extrai os argumentos (socket e player_id) e libera a memória da struct
    thread_args_t *args = (thread_args_t *)args_ptr;
    int csock = args->client_socket;
    int32_t player_id = args->player_id;
    free(args_ptr);

    char caddrstr[BUFSZ];
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)&cstorage;
    socklen_t caddrlen = sizeof(cstorage);

    // Obtém informações do cliente conectado para log
    getpeername(csock, caddr, &caddrlen);
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("Thread de cliente iniciada para %s | Player ID: %d\n", caddrstr, player_id);

    // Lógica de comunicação com o cliente
    aviator_msg msg;
    size_t count;
    while((count = recv(csock, &msg, sizeof(aviator_msg), 0)) > 0) {
        // Garante que uma struct completa foi recebida
        if (count == sizeof(aviator_msg)) {
            // NOVO: Atribui o ID do jogador à mensagem recebida
            msg.player_id = player_id;

            printf("[msg from player %d] type: %s, value: %.2f\n", msg.player_id, msg.type, msg.value);

            // Ecoa a mesma mensagem (agora com o ID) de volta para o cliente
            if(send(csock, &msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
                logexit("send");
            }
        }
    }

    close(csock);
    printf("Conexão com player %d (%s) encerrada.\n", player_id, caddrstr);

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
    //printf("Bound to %s, waiting connections\n", addrstr);

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
        // Aloca memória para os argumentos da thread (socket + player_id)
        thread_args_t *args = (thread_args_t *)malloc(sizeof(thread_args_t));
        if (!args) {
            logexit("malloc");
        }
        args->client_socket = csock;
        args->player_id = next_player_id++; // Atribui o ID atual e incrementa para o próximo

        // Cria uma nova thread para atender o cliente, passando a struct de argumentos
        if (pthread_create(&thread_id, NULL, client_thread_handler, args) < 0) {
            logexit("could not create thread");
        }

        pthread_detach(thread_id); // Libera recursos da thread automaticamente ao terminar
    }

    exit(EXIT_SUCCESS);
}