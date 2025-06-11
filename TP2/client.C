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
    printf("Usage: %s <server IP> <server port>\n", argv[0]);
    printf("Example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024

int main (int argc, char **argv) {
    // Verifica se os argumentos necessários foram fornecidos
    if (argc < 3) {
        usage(argc, argv);
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
    printf("Digite mensagens para enviar. Pressione Ctrl+D ou Ctrl+C para sair.\n");

    char send_buf[BUFSZ];
    char recv_buf[BUFSZ];
    
    while(1) {
        printf("> ");
        if (fgets(send_buf, BUFSZ, stdin) == NULL) {
            break; 
        }

        // Envia a mensagem para o servidor
        size_t sent_count = send(s, send_buf, strlen(send_buf), 0);
        if(sent_count != strlen(send_buf)) {
            logexit("send");
        }

        // Recebe o eco do servidor
        memset(recv_buf, 0, BUFSZ);
        size_t recv_count = recv(s, recv_buf, BUFSZ - 1, 0);
        if (recv_count <= 0) {
            printf("\nConexão encerrada pelo servidor.\n");
            break;
        }

        printf("Eco: %s", recv_buf);
    }

    close(s);
    printf("\nCliente encerrado.\n");
    exit(EXIT_SUCCESS);
}