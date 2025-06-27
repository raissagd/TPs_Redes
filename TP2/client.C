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

#define BUFSZ 1024

/*
    Descricao: Funcao que imprime a mensagem de uso do programa
    Argumentos: argc - numero de argumentos passados
               argv - vetor de argumentos passados
    Retorno: Nao possui           
*/
void usage (int argc, char **argv) {
    printf("Usage: %s <server IP> <server port> -nick <nickname>\n", argv[0]);
    printf("Example: %s 127.0.0.1 51511 -nick Flip\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main (int argc, char **argv) {
    // Verifica se foram fornecidos o número correto de argumentos
    if (argc != 5) {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        //usage(argc, argv);
    }

    // Valida se tem a flag -nick
    if (strcmp(argv[3], "-nick") != 0) {
        fprintf(stderr, "Error: Invalid flag. Expected '-nick'\n");
        //usage(argc, argv);
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
    ssize_t recv_count = recv(s, &initial_msg, sizeof(aviator_msg), 0);
    if (recv_count <= 0) {
        printf("Erro ao receber mensagem inicial do servidor.\n");
        close(s);
        exit(EXIT_FAILURE);
    }

    // Verifica se é mensagem de start e exibe o tempo restante
    if (strcmp(initial_msg.type, "start") == 0) {
        int remaining_time = (int)initial_msg.value;
        printf("Rodada aberta! Digite o valor da aposta ou digite [Q] para sair (%d segundos restantes)\n", remaining_time);
    }

    aviator_msg send_msg, response_msg;
    char input_buffer[BUFSZ];
    int bets_closed   = 0;        // apostas encerradas
    int has_bet       = 0;        // flag para verificar se o usuário apostou
    float current_bet = 0.0f;     // armazena o valor da aposta atual

    // Loop principal: continua enquanto o servidor não fechar
    while (1) {
        // Monitora socket e stdin
        fd_set read_fds;
        struct timeval timeout;
        FD_ZERO(&read_fds);
        FD_SET(s, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 100000; // 100ms

        int activity = select(s + 1, &read_fds, NULL, NULL, &timeout);

        if (activity > 0) {
            // Mensagem do servidor
            if (FD_ISSET(s, &read_fds)) {
                recv_count = recv(s, &response_msg, sizeof(aviator_msg), MSG_DONTWAIT);
                if (recv_count > 0) {
                    char *t = response_msg.type;
                    // Agrupamos 'closed' e 'explode' como eventos de fim de rodada
                    switch (t[0]) {
                        case 's':  // start
                            {
                                int remaining_time = (int)response_msg.value;
                                printf("Rodada aberta! Digite o valor da aposta ou digite [Q] para sair (%d segundos restantes)\n", remaining_time);
                                bets_closed   = 0;
                                has_bet       = 0;
                                current_bet   = 0.0f;
                            }
                            break;
                        case 'c':  // "closed"
                            if (strcmp(t, "closed") == 0) {
                                printf("Apostas encerradas! Não é mais possível apostar nesta rodada. Digite [C] para sacar.\n");
                                bets_closed = 1;
                            }
                            break;
                        case 'e':  // "explode"
                            if (strcmp(t, "explode") == 0) {
                                printf("Aviãozinho explodiu em: %.2fx.\n", response_msg.value);
                                if (has_bet) {
                                    printf("Você perdeu R$ %.2f. Tente novamente na próxima rodada! Aviãozinho tá pagando :).\n", current_bet);
                                }
                                printf("Profit da casa: R$ %.2f\n", response_msg.house_profit);
                                bets_closed = 0;  // volta a esperar start
                            }
                            break;
                        case 'm':  // multiplier
                            printf("Multiplicador atual: %.2fx\n", response_msg.value);
                            break;
                        case 'p':  // payout
                            if (strcmp(t, "payout") == 0) {
                                float multiplier = (current_bet > 0) ? response_msg.value / current_bet : 0.0f;
                                printf("Você sacou em %.2fx e ganhou R$ %.2f!\nSeu profit atual: R$ %.2f\n", multiplier, response_msg.value, response_msg.player_profit);
                                has_bet       = 0;
                            }
                            break;
                        case 'b':  // bye
                            printf("O servidor caiu, mas sua esperança pode continuar de pé. Até breve!\n");
                            close(s);
                            exit(EXIT_SUCCESS);
                            break;
                        default:
                            break;
                    }
                }
                else { // recv_count <= 0: servidor fechou
                    printf("\nO servidor caiu, mas sua esperança pode continuar de pé. Até breve!\n");
                    break;
                }
            }

            // Entrada do usuário
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                if (!fgets(input_buffer, BUFSZ, stdin)) {
                    break;
                }
                input_buffer[strcspn(input_buffer, "\n")] = 0;

                if (strcmp(input_buffer, "Q") == 0) {
                    // Usuário quer sair
                    memset(&send_msg, 0, sizeof(send_msg));
                    strcpy(send_msg.type, "bye");
                    send(s, &send_msg, sizeof(aviator_msg), 0);
                    printf("Aposte com responsabilidade. A plataforma é nova e tá com horário bugado.\nVolte logo, %s!\n", argv[4]);
                    break;
                }

                if (bets_closed && strcmp(input_buffer, "C") == 0) {
                    if (has_bet) {
                        memset(&send_msg, 0, sizeof(send_msg));
                        strcpy(send_msg.type, "cashout");
                        send(s, &send_msg, sizeof(aviator_msg), 0);
                    } else {
                        printf("Você não apostou nesta rodada.\n");
                    }
                    continue;
                }

                if (!bets_closed) {
                    char *endptr;
                    double bet_value = strtod(input_buffer, &endptr);
                    if (endptr != input_buffer) {
                        memset(&send_msg, 0, sizeof(send_msg));
                        strcpy(send_msg.type, "bet");
                        send_msg.value = (float)bet_value;
                        send(s, &send_msg, sizeof(aviator_msg), 0);
                        printf("Aposta recebida: R$ %.2f\n", bet_value);
                        has_bet     = 1;
                        current_bet = (float)bet_value;
                    } else {
                        printf("Error: Invalid command or bet value\n");
                    }
                } else {
                    printf("Error: Invalid command\n");
                }
            }
        }
    }

    close(s);
    return 0;
}