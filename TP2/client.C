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

// Variáveis globais
float player_total_profit = 0.0f; // Para armazenar profit acumulado
int waiting_next_round = 0;       // Flag para indicar aguardando próxima rodada

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
    size_t recv_count = recv(s, &initial_msg, sizeof(aviator_msg), 0);
    if (recv_count <= 0) {
        printf("Erro ao receber mensagem inicial do servidor.\n");
        close(s);
        exit(EXIT_FAILURE);
    }

    // Verifica se é mensagem de start e exibe o tempo restante
    if (strcmp(initial_msg.type, "start") == 0) {
        int remaining_time = (int)initial_msg.value;
        player_total_profit = initial_msg.player_profit; // Recebe profit acumulado
        printf("Rodada aberta! Digite o valor da aposta ou digite [Q] para sair (%d segundos restantes)\n", remaining_time);
    }

    aviator_msg send_msg;
    aviator_msg response_msg;
    char input_buffer[BUFSZ];
    int  round_active = 1; // controla loop principal
    int  bets_closed  = 0; // apostas encerradas
    int  user_quit    = 0; // saiu via Q
    int  has_bet      = 0; // flag para verificar se o usuário apostou
    float current_bet = 0.0f; // armazena o valor da aposta atual

    int connected = 1; // Controla conexão geral
    while (connected) {
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
                    // Tratar mensagem "start" para novas rodadas
                    if (strcmp(response_msg.type, "start") == 0) {
                        int remaining_time = (int)response_msg.value;
                        player_total_profit = response_msg.player_profit; // Atualiza profit
                        printf("Rodada aberta! Digite o valor da aposta ou digite [Q] para sair (%d segundos restantes)\n", remaining_time);
                        
                        // Resetar variáveis da rodada
                        round_active = 1;
                        bets_closed = 0;
                        has_bet = 0;
                        current_bet = 0.0f;
                        waiting_next_round = 0;
                    }
                    else if (strcmp(response_msg.type, "closed") == 0) {
                        printf("Apostas encerradas! Não é mais possível apostar nesta rodada. Digite [C] para sacar.\n");
                        bets_closed = 1;
                    }
                    else if (strcmp(response_msg.type, "multiplier") == 0) {
                        printf("Multiplicador atual: %.2fx\n", response_msg.value);
                    }
                    // Não sair após explosão, aguardar próxima rodada
                    else if (strcmp(response_msg.type, "explode") == 0) {
                        printf("Aviãozinho explodiu em: %.2fx.\n", response_msg.value);
                        player_total_profit = response_msg.player_profit; // Atualiza profit

                        // Se o jogador apostou e não fez cashout, ele perdeu
                        if (has_bet) {
                            printf("Você perdeu R$ %.2f. Tente novamente na próxima rodada! Aviãozinho tá pagando :).\n", current_bet);
                            printf("Profit atual: R$ %.2f\n", player_total_profit);
                        }
                        
                        printf("Profit da casa: R$ %.2f\n", response_msg.house_profit);
                        
                        // Não sair, aguardar próxima rodada
                        round_active = 0;  // Encerra rodada atual
                        waiting_next_round = 1; // Mas permanece conectado
                        has_bet = 0;
                        current_bet = 0.0f;
                        bets_closed = 0;
                    }
                    else if (strcmp(response_msg.type, "payout") == 0) {
                        float multiplier = (current_bet > 0) ? response_msg.value / current_bet : 0.0f;
                        player_total_profit = response_msg.player_profit; // Atualiza profit acumulado
                        printf("Você sacou em %.2fx e ganhou R$ %.2f!\nSeu profit atual: R$ %.2f\n", 
                            multiplier, response_msg.value, player_total_profit);
                        has_bet = 0;
                        current_bet = 0.0f;
                    }
                    else if (strcmp(response_msg.type, "cashout_rejected") == 0) {
                        printf("Não foi possível sacar. Você não apostou ou já sacou.\n");
                    }
                    else if (strcmp(response_msg.type, "bye") == 0) {
                        printf("O servidor caiu, mas sua esperança pode continuar de pé. Até breve!\n");
                        connected = 0; // Sai do loop principal
                    }
                }
                else if (recv_count == 0) {
                    printf("\nO servidor caiu, mas sua esperança pode continuar de pé. Até breve!\n");
                    connected = 0; // Sai do loop principal
                }
            }

            // Entrada do usuário - permitir comandos mesmo aguardando próxima rodada
            if (FD_ISSET(STDIN_FILENO, &read_fds) && connected) {
                if (fgets(input_buffer, BUFSZ, stdin) == NULL) {
                    connected = 0;
                    break;
                }
                input_buffer[strcspn(input_buffer, "\n")] = 0;

                if (strcmp(input_buffer, "Q") == 0) {
                    // Usuário quer sair
                    user_quit = 1;
                    connected = 0;
                    memset(&send_msg, 0, sizeof(send_msg));
                    strcpy(send_msg.type, "bye");
                    send(s, &send_msg, sizeof(aviator_msg), 0);
                    break;
                }

                // Tratar comando C (cashout) quando apostas estão fechadas
                if (bets_closed && strcmp(input_buffer, "C") == 0) {
                    if (has_bet) {
                        // Envia comando de cashout
                        memset(&send_msg, 0, sizeof(send_msg));
                        strcpy(send_msg.type, "cashout");
                        send_msg.value        = 0.0f;
                        send_msg.player_id    = 0;
                        send_msg.player_profit= 0.0f;
                        send_msg.house_profit = 0.0f;
                        if (send(s, &send_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
                            logexit("send");
                        }
                    } else {
                        // sei que essa validação não é obrigatória mas é útil na hora de depurar
                        printf("Você não apostou nesta rodada.\n");
                    }
                    continue;
                }

                if (!bets_closed) {
                    // Verifica se é um comando inválido (letra que não seja C)
                    if (isalpha(input_buffer[0]) && strcmp(input_buffer, "C") != 0) {
                        fprintf(stderr, "Error: Invalid command\n");
                        continue;
                    }
                    
                    float bet_value;
                    char extra_char;
                    
                    if (sscanf(input_buffer, "%f%c", &bet_value, &extra_char) != 1 || bet_value <= 0) {
                        fprintf(stderr, "Error: Invalid bet value\n");
                        continue;
                    }
                    
                    // Envia a aposta
                    memset(&send_msg, 0, sizeof(send_msg));
                    strcpy(send_msg.type, "bet");
                    send_msg.value        = bet_value;
                    send_msg.player_id    = 0;
                    send_msg.player_profit= 0.0f;
                    send_msg.house_profit = 0.0f;
                    if (send(s, &send_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
                        logexit("send");
                    }

                    printf("Aposta recebida: R$ %.2f\n", bet_value);
                    has_bet     = 1;         
                    current_bet = bet_value;
                } else {
                    // Se chegou aqui e não é o comando C, é um comando inválido
                    if (strcmp(input_buffer, "C") != 0) {
                        printf("Error: Invalid command\n");
                    }
                }
            }
        }
    }

    close(s);

    if (user_quit) {
        printf("Aposte com responsabilidade. A plataforma é nova e tá com horário bugado.\nVolte logo, %s!\n", argv[4]);
    }

    exit(EXIT_SUCCESS);
}