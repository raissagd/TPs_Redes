/*
-----------------------------------------------------
Arquivo server.c referente ao TP1
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

#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 1024

/*
    Descricao: Funcao que imprime a mensagem de uso do programa
    Argumentos: argc - numero de argumentos passados
               argv - vetor de argumentos passados
    Retorno: Nao possui           
*/
void usage (int argc, char **argv) {
    printf("Usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("Example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

/*
    Descricao: Funcao auxiliar que retorna uma jogada aleatoria
    Argumentos: Nao possui
    Retorno: Retorna um inteiro entre 0 e 4
*/
int pick_random_move() {
    int move = rand() % 5; // Número alatório entre 0 e 4
    return move;
}

int who_wins(int opt1, int opt2) {
    /*
    0 - Nuclear Attack
    1 - Intercept Attack
    2 - Cyber Attack
    3 - Drone Strike
    4 - Bio Attack

    Resultados:
    1  - Vitória do Jogador 1
    0  - Empate
    -1 - Vitória do Jogador 2
    */

    if (opt1 == opt2) {
        return 0; // Empate
    }

    if ((opt1 == 0 && (opt2 == 1 || opt2 == 4)) ||
        (opt1 == 1 && (opt2 == 2 || opt2 == 3)) ||
        (opt1 == 2 && (opt2 == 0 || opt2 == 4)) ||
        (opt1 == 3 && (opt2 == 0 || opt2 == 2)) ||
        (opt1 == 4 && (opt2 == 1 || opt2 == 3))) {
        return -1; // Jogador 1 perde
    }

    return 1; // Jogador 1 ganha
}

/*
    Descricao: Funcao que verifica se a escolha do usuario é válida
    Argumentos: opt - opcao escolhida pelo usuario
    Retorno: 1 se a opcao for valida, 0 se for invalida           
*/
int valid_move(int opt) {
    if (opt == 0 || opt == 1 || opt == 2 || opt == 3 || opt == 4) {
        return 1; // Valid move
    }
    return 0;
}

/*
    Descricao: Funcao que verifica se a escolha do usuario para jogar novamente é válida
    Argumentos: opt - opcao escolhida pelo usuario
    Retorno: 1 se a opcao for valida, 0 se for invalida           
*/
int valid_playagain(int opt) {
    if (opt == 0 || opt == 1) {
        return 1; // Valid move
    }
    return 0;
}

int main (int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    // Cria um socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    // Faz o bind (vincula o socket ao IP/porta)
    struct sockaddr *addr = (struct sockaddr *)&storage;
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    // Coloca o socket em modo de escuta (esperando conexões)
    if (0 != listen(s, 1)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    
    if (storage.ss_family == AF_INET) {
        printf("Servidor iniciado em modo IPv4 na porta %s. Aguardando conexão...\n", argv[2]);
    } else {
        printf("Servidor iniciado em modo IPv6 na porta %s. Aguardando conexão...\n", argv[2]);
    }

    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)&cstorage;
    socklen_t caddrlen = sizeof(cstorage);

    // Aceita conexão de um cliente
    int csock = accept(s, caddr, &caddrlen);
    if (csock == -1) {
        logexit("accept");
    }

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("Cliente conectado.\n");

    // char buf[BUFSZ];
    GameMessage msg;
    memset(&msg, 0, sizeof(msg));
    
    int client_victories = 0;
    int server_victories = 0;
        
    while(1) {
        printf("Apresentando as opções para o cliente.\n");

        memset(&msg, 0, sizeof(msg));
        size_t count = recv(csock, &msg, sizeof(msg), 0); // Número de bytes recebidos

        if (count == 0) {
            // Se count for 0, significa que o cliente fechou a conexão
            close(csock);
            continue;
        }

        if(!valid_move(msg.client_action) ) {
            printf("Erro: opção inválida de jogada.\n");
            msg.type = MSG_ERROR;
            snprintf(msg.message, MSG_SIZE, "Erro: jogada inválida. Escolha um valor entre 0 e 4.");
            send(csock, &msg, sizeof(msg), 0);
            continue; // Pergunta novamente
        }

        printf("Cliente escolheu %d.\n", msg.client_action);

        int server_move = pick_random_move(); // Jogada aleatória do servidor
        int result = who_wins(msg.client_action, server_move); // Verifica quem ganhou

        if (result == 1) {
            client_victories++;
        } else if (result == -1) {
            server_victories++;
        } else {
            printf("Jogo empatado.\n");
        }
        
        printf("Servidor escolheu aleatoriamente %d.\n", server_move);
            
        msg.type = MSG_RESULT;
        msg.server_action = server_move;
        msg.result = result;
        msg.client_wins = client_victories;
        msg.server_wins = server_victories;
        snprintf(msg.message, MSG_SIZE, "Servidor escolheu %d. Resultado: %d", server_move, result);

        send(csock, &msg, sizeof(msg), 0);

        printf("Placar: Cliente %d x %d Servidor\n", client_victories, server_victories);

        while (1) {
            printf("Perguntando se o cliente deseja jogar novamente.\n");
            
            msg.type = MSG_PLAY_AGAIN_REQUEST;
            snprintf(msg.message, MSG_SIZE, "Deseja jogar novamente?\n1 - Sim\n0 - Não\n");
            send(csock, &msg, sizeof(msg), 0);

            memset(&msg, 0, sizeof(msg));
            count = recv(csock, &msg, sizeof(msg), 0);

            if (!valid_playagain(msg.client_action)) {
                printf("Erro: resposta inválida para jogar novamente.\n");
                msg.type = MSG_ERROR;
                snprintf(msg.message, MSG_SIZE, "Erro: resposta inválida. Escolha 0 ou 1.");
                send(csock, &msg, sizeof(msg), 0);
                continue;
            } else {
                break; // Resposta válida
            }
        }
            
        if (msg.client_action == 0) {
            printf("Cliente não deseja jogar novamente.\n");
            printf("Enviando placar final.\n");
            
            msg.type = MSG_END;
            snprintf(msg.message, MSG_SIZE, "Placar final: Você %d x %d Servidor", client_victories, server_victories);
            send(csock, &msg, sizeof(msg), 0);

            printf("Encerrando conexão.\n");
            break;
        } else {
            printf("Cliente deseja jogar novamente.\n");
        }
    }
    
    close(csock);
    close(s);

    exit(EXIT_SUCCESS);
}