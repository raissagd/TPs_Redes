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

/*
    Descricao: Funcao que printa no terminal do cliente as opcoes de jogada
    Argumentos: nenhum
    Retorno: Nao possui
*/
void print_initial_message() {
    printf("Escolha sua jogada:\n");
    printf("0 - Nuclear Attack\n");
    printf("1 - Intercept Attack\n");
    printf("2 - Cyber Attack\n");
    printf("3 - Drone Strike\n");
    printf("4 - Bio Attack\n");
}

/*
    Descricao: Funcao que printa no terminal do cliente se ele deseja jogar novamente
    Argumentos: nenhum
    Retorno: Nao possui
*/
void print_playagain_message() {
    printf("Deseja jogar novamente?\n");
    printf("1 - Sim\n");
    printf("0 - Não\n");
}

/*
    Descricao: Funcao que passa a jogada do usuario de inteiro para seu respectivo nome
    Argumentos: opt - opcao escolhida pelo usuario
    Retorno: string com o nome da jogada
*/
const char* option_to_action(int opt) {
    switch (opt) {
        case 0: return "Nuclear Attack";
        case 1: return "Intercept Attack";
        case 2: return "Cyber Attack";
        case 3: return "Drone Strike";
        case 4: return "Bio Attack";
        default: return "Invalid Option";
    }
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

    char buf[BUFSZ];    
    
    while(1) {
        print_initial_message();
    
        fgets(buf, BUFSZ - 1, stdin);
        buf[strcspn(buf, "\n")] = '\0'; // remove o \n do final, se houver
        size_t count = send(s, buf, strlen(buf) + 1, 0); // Número de bytes enviados

        if(!valid_move(atoi(buf))) {
            printf("Por favor, selecione um valor de 0 a 4.\n");
            close(s);
            exit(EXIT_FAILURE);
        }
        
        if(count != strlen(buf) + 1) {
            logexit("send");
        }

        printf("Você escolheu: %s.\n", option_to_action(atoi(buf)));

        memset(buf, 0, BUFSZ);
        unsigned total = 0; // Número total de bytes recebidos	

        count = recv(s, buf + total, BUFSZ - total, 0); // Adiciona os bytes recebidos ao buffer, em ordem
        if (count ==  0) {
            // Se count for 0, significa que o servidor fechou a conexão
            break;
        }
        total += count;

        int result, server_move;
        sscanf(buf, "%d %d", &result, &server_move); // Lê o resultado e a jogada do servidor

        printf("Servidor escolheu: %s.\n", option_to_action(server_move));

        if (result == 1) {
        printf("Resultado: Vitória!\n");
        } else if (result == 0) {
            printf("Resultado: Empate!\n");
        } else {
            printf("Resultado: Você perdeu!\n");
        }

        while (1) {
            print_playagain_message();
            fgets(buf, BUFSZ - 1, stdin);
            buf[strcspn(buf, "\n")] = '\0';

            if (valid_playagain(atoi(buf))) {
                break; // entrada válida: sai do while
            } else {
                printf("Por favor, digite 1 para jogar novamente ou 0 para encerrar.\n");
            }
        }

        count = send(s, buf, strlen(buf) + 1, 0);
        
        if (count != strlen(buf) + 1) {
            logexit("send");
        }

        if (buf[0] == '0') {
            printf("Fim de jogo!\n");
            // printf("Placar final: ")
            printf("Obrigada por jogar!\n");
            break;
        }
    }

    close(s);

    exit(EXIT_SUCCESS);
}