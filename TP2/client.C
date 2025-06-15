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
    printf("Usage: %s <server IP> <server port> -nick <nickname>\n", argv[0]);
    printf("Example: %s 127.0.0.1 51511 -nick Flip\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024

int main (int argc, char **argv) {
    // Verifica se foram fornecidos o número correto de argumentos
    if (argc != 5) {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        usage(argc, argv);
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

    // Aguarda mensagem inicial do servidor (start)
    aviator_msg initial_msg;
    size_t recv_count = recv(s, &initial_msg, sizeof(aviator_msg), 0);
    if (recv_count <= 0) {
        printf("Erro ao receber mensagem inicial do servidor.\n");
        close(s);
        exit(EXIT_FAILURE);
    }

    // Verifica se é mensagem de start e exibe o tempo restante
    if (strcmp(initial_msg.type, "start") == 0) {
        int remaining_time = (int)initial_msg.value;
        printf("Rodada aberta! Digite o valor da aposta ou digite [Q] para sair (%d segundos restantes):\n", remaining_time);
    }

    aviator_msg send_msg;
    aviator_msg response_msg;
    char input_buffer[BUFSZ];
    int round_active = 1; // Flag para controlar se ainda pode apostar
    int bets_closed = 0; // Flag para controlar se as apostas foram encerradas
    
    while(round_active) {
        // Verifica se há mensagens do servidor (não bloqueante)
        fd_set read_fds;
        struct timeval timeout;
        FD_ZERO(&read_fds);
        FD_SET(s, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms
        
        int activity = select(s + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity > 0) {
            // Verifica se há mensagem do servidor
            if (FD_ISSET(s, &read_fds)) {
                recv_count = recv(s, &response_msg, sizeof(aviator_msg), MSG_DONTWAIT);
                if (recv_count > 0) {
                    if (strcmp(response_msg.type, "round_ended") == 0) {
                        printf("Apostas encerradas! Não é mais possível apostar nesta rodada. Digite [C] para sacar.\n");
                        bets_closed = 1;
                    } else if (strcmp(response_msg.type, "bet_accepted") == 0) {
                        printf("Aposta recebida: R$ %.2f\n", response_msg.value);
                    } else if (strcmp(response_msg.type, "bet_rejected") == 0) {
                        printf("Apostas encerradas! Não é mais possível apostar nesta rodada. Digite [C] para sacar.\n");
                        bets_closed = 1;
                    } 
                } else if (recv_count == 0) {
                    printf("\nO servidor caiu, mas sua esperança pode continuar de pé. Até breve!\n");
                    break;
                }
            }
            
            // Verifica se há entrada do usuário
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                if (fgets(input_buffer, BUFSZ, stdin) == NULL) {
                    break; 
                }

                input_buffer[strcspn(input_buffer, "\n")] = 0; // Remove o caractere de nova linha

                if (strcmp(input_buffer, "Q") == 0) {
                    // Usuário quer sair
                    break;
                }
                
                if (strcmp(input_buffer, "C") == 0 && bets_closed) {
                    // Usuário quer sacar (implementar depois)
                    printf("Funcionalidade de saque será implementada posteriormente.\n");
                    continue;
                }

                if (!bets_closed) {
                    if (isalpha(input_buffer[0])) {
                        // Se for uma letra, tem que ser Q
                        fprintf(stderr, "Error: Invalid command\n");
                        continue;
                    }

                    float bet_value;
                    char extra_char;
                    
                    if (sscanf(input_buffer, "%f%c", &bet_value, &extra_char) != 1 || bet_value <= 0) {
                        // A aposta tem que ser um número maior que zero
                        fprintf(stderr, "Error: Invalid bet value\n");
                        continue; 
                    }

                    // Se tudo tá ok, envia a aposta para o servidor
                    memset(&send_msg, 0, sizeof(aviator_msg)); 
                    strcpy(send_msg.type, "bet");
                    send_msg.value = bet_value;
                    send_msg.player_id = 0; // Será preenchido pelo servidor
                    send_msg.player_profit = 0.0;
                    send_msg.house_profit = 0.0;

                    // Envia a mensagem para o servidor
                    if(send(s, &send_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
                        logexit("send");
                    }
                } else {
                    printf("Apostas já encerradas. Digite [C] para sacar.\n");
                }
            }
        }
    }

    close(s);
    printf("Aposte com responsabilidade. A plataforma é nova e tá com horário bugado.\nVolte logo, %s!\n", argv[4]);
    exit(EXIT_SUCCESS);
}