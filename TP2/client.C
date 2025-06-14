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
#include <ctype.h> 
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
        fprintf(stderr, "Error: Invalid number of arguments\n");
        //usage(argc, argv);
    }

    // Valida se tem a flag -nick
    if (strcmp(argv[3], "-nick") != 0) {
        fprintf(stderr, "Error: Invalid flag. Expected '-nick'\n");
        usage(argc, argv);
    }

    // Validação do tamanho do nickname
    if (strlen(argv[4]) > 13) {
        fprintf(stderr, "Error: Nickname too long (max 13)\n");
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

    //printf("Connected to %s\n", addrstr);
    printf("Rodada aberta! Digite o valor da aposta ou digite [Q] para sair (X) segundos restantes):\n");

    aviator_msg send_msg;
    aviator_msg recv_msg;
    char input_buf[BUFSZ];
    
    while(1) {
        if (fgets(input_buf, BUFSZ, stdin) == NULL) {
            break; 
        }

        input_buf[strcspn(input_buf, "\n")] = 0; // Remove o caractere de nova linha

        if (strcmp(input_buf, "Q") == 0) {
            // Usuário quer sair
            break;
        }

        if (isalpha(input_buf[0])) {
            // Se for uma letra, tem que ser Q
            fprintf(stderr, "Error: Invalid command\n");
            continue;
        }

        float bet_value;
        char trailing_char;
        
        if (sscanf(input_buf, "%f%c", &bet_value, &trailing_char) != 1 || bet_value <= 0) {
            // A aposta tem que ser um número maior que zero
            fprintf(stderr, "Error: Invalid bet value\n");
            continue; 
        }

        // Se tudo tá ok, envia a aposta para o servidor
        memset(&send_msg, 0, sizeof(aviator_msg)); 
        strcpy(send_msg.type, "bet");
        send_msg.value = bet_value;

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
    printf("Aposte com responsabilidade. A plataforma é nova e tá com horário bugado.\nVolte logo, %s!\n", argv[4]);
    exit(EXIT_SUCCESS);
}