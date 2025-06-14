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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void usage (int argc, char **argv) {
    //printf("Usage: %s <server IP> <server port>\n", argv[0]);
    //printf("Example: %s 127.0.0.1 51511\n", argv[0]);
    printf("Usage: %s <server IP> <server port> -nick <nickname>\n", argv[0]);
    printf("Example: %s 127.0.0.1 51511 -nick Flip\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024

int main (int argc, char **argv) {
    // Verifica se foram fornecidos o número correto de argumentos
    if (argc != 5) {
        fprintf(stderr, "Error: Invalid number of arguments.\n");
        //usage(argc, argv);
    }

    // Valida se tem a flag -nick
    if (strcmp(argv[3], "-nick") != 0) {
        fprintf(stderr, "Error: Invalid flag. Expected '-nick'.\n", argv[3]);
        usage(argc, argv);
    }

    // Validação do tamanho do nickname
    if (strlen(argv[4]) > 13) {
        fprintf(stderr, "Error: Nickname too long (max 13).\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage storage;
    // Preenche a estrutura de endereço do servidor
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    // Cria o socket TCP
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)&storage;
    // Conecta ao servidor
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    printf("Connected to %s\n", addrstr);
    printf("Digite mensagens para enviar.\n");

    aviator_msg send_msg;
    aviator_msg recv_msg;
    char input_buf[BUFSZ];
    
    while(1) {
        printf("> ");
        if (fgets(input_buf, BUFSZ, stdin) == NULL) {
            break; 
        }

        // Prepara a mensagem para enviar
        memset(&send_msg, 0, sizeof(aviator_msg)); 
        sscanf(input_buf, "%s %f", send_msg.type, &send_msg.value);

        // Envia a mensagem para o servidor
        if(send(s, &send_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
            logexit("send");
        }

        // Recebe a mensagem do servidor
        size_t recv_count = recv(s, &recv_msg, sizeof(aviator_msg), 0);
        if (recv_count <= 0) {
            printf("\nConexão encerrada pelo servidor.\n");
            break;
        }

        // Imprime os dados recebidos da struct
        printf("Eco -> ID: %d, Type: %s, Value: %.2f, Player Profit: %.2f, House Profit: %.2f\n", 
               recv_msg.player_id, recv_msg.type, recv_msg.value, recv_msg.player_profit, recv_msg.house_profit);
        
    }

    close(s);
    printf("\nCliente encerrado.\n");
    exit(EXIT_SUCCESS);
}