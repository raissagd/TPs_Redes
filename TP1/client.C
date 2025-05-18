/*
-----------------------------------------------------
Arquivo client.c referente ao TP1
Implementacao do cliente, ou Unidade de Monitoramento
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

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*
    Descricao: Funcao que imprime a mensagem de uso do programa
    Argumentos: argc - numero de argumentos passados
               argv - vetor de argumentos passados
    Retorno: Nao possui           
*/
void usage (int argc, char **argv) {
    printf("Usage: %s <server IP> <server port>", argv[0]);
    printf("Example: %s 127.0.0.1 51511", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024

int main (int argc, char **argv) {
    // Verifica se o usuário forneceu o IP e a porta
    if (argc < 3) {
        usage(argc, argv);
    }

    // Tenta converter o endereço IP e a porta fornecidos pelo usuário para uma estrutura sockaddr_storage
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    // Cria um socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    // Conecta o socket ao endereço e porta fornecidos
    struct sockaddr *addr = (struct sockaddr *)&storage;
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ); // Converte o endereço para uma string
    printf("Conectado ao servidor.\n");

    // char buf[BUFSZ];    
    GameMessage msg;
    memset(&msg, 0, sizeof(msg));
    
    while(1) {
        // -------------------------------- Cliente recebe o pedido para jogar -------------------------------------
        
        memset(&msg, 0, sizeof(msg));
        size_t count = recv(s, &msg, sizeof(msg), 0);

        if (count == 0) {
            printf("Servidor fechou a conexão.\n");
            break;
        }

        if (msg.type == MSG_ERROR) {
            printf("%s\n", msg.message);
            continue; // Volta ao início para tentar de novo
        }

        printf("%s\n", msg.message);

        // -------------------------------- Cliente envia a jogada -------------------------------------
        
        fgets(msg.message, MSG_SIZE - 1, stdin);
        msg.message[strcspn(msg.message, "\n")] = '\0';

        while(msg.message[0] == '\0') {
            printf("Entrada vazia. Tente novamente.\n");
            fgets(msg.message, MSG_SIZE - 1, stdin);
            msg.message[strcspn(msg.message, "\n")] = '\0';
        }

        msg.client_action = atoi(msg.message);
        msg.type = MSG_RESPONSE;
        count = send(s, &msg, sizeof(msg), 0);

        if(count != sizeof(msg)) {
            // Se count for diferente de sizeof(msg), significa que houve erro no envio
            logexit("send");
        }

        // -------------------------------- Cliente recebe o resultado -------------------------------------

        memset(&msg, 0, sizeof(msg));
        count = recv(s, &msg, sizeof(msg), 0);

        if (msg.type == MSG_ERROR) {
            printf("%s\n", msg.message);
            continue; // Volta ao início para tentar de novo
        }

        if (count == 0) {
            printf("Servidor fechou a conexão.\n");
            break;
        }

        printf("%s\n", msg.message); 

        if (msg.result == 0) {
            continue; // Se o jogo empatou, pergunta novamente
        }

        // -------------------------------- Cliente recebe a mensagem se quer jogar de novo -------------------------------------

        memset(&msg, 0, sizeof(msg));
        count = recv(s, &msg, sizeof(msg), 0);
        printf("%s\n", msg.message);            

        if (count == 0) {
            printf("Servidor fechou a conexão.\n");
            break;
        }

        // -------------------------------- Cliente envia a resposta -------------------------------------

        while (1) {
            fgets(msg.message, MSG_SIZE - 1, stdin);
            msg.message[strcspn(msg.message, "\n")] = '\0';

            if (msg.message[0] == '\0') {
                printf("Entrada vazia. Digite 1 para jogar novamente ou 0 para sair.\n");
                continue;
            }

            msg.client_action = atoi(msg.message);
            break;
        }

        msg.type = MSG_PLAY_AGAIN_RESPONSE;

        count = send(s, &msg, sizeof(msg), 0);
        if (count != sizeof(msg)) {
            logexit("send");
        }

        if (msg.client_action == 0) {
            printf("Fim de jogo!\n");
            
            // -------------------------------- Cliente recebe a mensagem de fim de jogo-------------------------------------

            memset(&msg, 0, sizeof(msg));
            count = recv(s, &msg, sizeof(msg), 0);
            printf("%s\n", msg.message);

            printf("Obrigada por jogar!\n");
            break;
        }
    }

    close(s);

    exit(EXIT_SUCCESS);
}