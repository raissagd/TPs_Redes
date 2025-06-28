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
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>

// ------------------------------------------------------------------------------------------------

#define BUFSZ 1024
#define COUNTDOWN_TIME 10 // Tempo da janela de apostas
#define MAX_CLIENTS 10    // Máximo de clientes

// Struct para argumentos da thread
typedef struct {
    int      client_socket;
    int32_t  player_id;
} thread_args_t;

// Struct para guardar dados dos jogadores
typedef struct {
    float profit;
    float bet;
    int has_bet;
    int cashed_out;
} Player;

Player players[MAX_CLIENTS] = {0};

// Variáveis globais
int32_t next_player_id      = 1;
int    round_active         = 0;   // 0 = não iniciada, 1 = ativa, 2 = encerrada
time_t round_start_time     = 0;
float  current_multiplier   = 1.0f;
int    multiplier_running   = 0;   // Flag para controlar se está enviando multiplicadores
float  explosion_point_global = 0.0f; // Para armazenar o ponto de explosão global

float house_profit = 0.0f;
float total_bets_received = 0.0f;
float total_payouts = 0.0f;

// Array para clientes conectados
int connected_clients[MAX_CLIENTS];
int client_player_ids[MAX_CLIENTS]; // Para mapear socket para player_id
int num_clients = 0;

// Array para apostas
float round_bets[MAX_CLIENTS];
int   round_bet_count = 0;

// ------------------------------------------------------------------------------------------------
// toda hora ficava dando erro de não declarado, deixei aqui de uma vez
void* countdown_thread(void* arg);
void start_round(void);

// ------------------------------------------------------------------------------------------------

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
    Descricao: inicia uma nova rodada
    Argumentos: nenhum
    Retorno: nenhum
*/
void start_round() {
    round_active     = 1;
    round_start_time = time(NULL);
    printf("event=start | id=* | N=%d\n", num_clients);

    // dispara a thread de contagem regressiva
    pthread_t time_thread;
    pthread_create(&time_thread, NULL, countdown_thread, NULL);
    pthread_detach(time_thread);

    // notifica todos os clientes sobre o start
    aviator_msg start_msg;
    memset(&start_msg, 0, sizeof(start_msg));
    strcpy(start_msg.type, "start");
    start_msg.value        = (float)COUNTDOWN_TIME;
    start_msg.house_profit = house_profit;

    for (int i = 0; i < num_clients; i++) {
        int pid = client_player_ids[i];
        int idx = pid - 1;
        start_msg.player_id     = pid;
        start_msg.player_profit = (idx >= 0 && idx < MAX_CLIENTS) ? players[idx].profit : 0.0f;
        send(connected_clients[i], &start_msg, sizeof(start_msg), 0);
    }
}

/*
    Descricao: calcula o ponto de explosão baseado nas apostas
    Argumentos: número de jogadores e array de apostas
    Retorno: ponto de explosão calculado
*/
// Calcula o ponto de explosão baseado nas apostas
float calculate_explosion_point(int num_players, float bets[]) {
    float total_bets = 0.0f;

    for (int i = 0; i < num_players; i++) {
        total_bets += bets[i];
    }

    float explosion_point = sqrtf(1.0f + (float)num_players + 0.01f * total_bets);
    return explosion_point;
}

/*
    Descricao: adiciona um cliente conectado
    Argumentos: socket do cliente e ID do jogador
    Retorno: Nenhum
*/
void add_client(int socket_cliente, int player_id) {
    if (num_clients < MAX_CLIENTS) {
        connected_clients[num_clients] = socket_cliente;
        client_player_ids[num_clients] = player_id;
        num_clients++;
    }
}

/*
    Descricao: remove um cliente desconectado
    Argumentos: socket do cliente
    Retorno: nenhum
*/
void remove_client(int socket_cliente) {
    for (int i = 0; i < num_clients; i++) {
        if (connected_clients[i] == socket_cliente) {
            // Move elementos para frente
            for (int j = i; j < num_clients - 1; j++) {
                connected_clients[j] = connected_clients[j + 1];
                client_player_ids[j] = client_player_ids[j + 1];
            }
            num_clients--;
            break;
        }
    }
}

/*
    Descricao: notifica o fim da rodada para todos os clientes (os que tão na partida atual ao menos)
    Argumentos: nenhum
    Retorno: nenhum
*/
void notify_round_end() {
    float explosion_point = 0.0f;

    if (round_bet_count > 0) {
        explosion_point = calculate_explosion_point(round_bet_count, round_bets);
    }

    printf("event=profit | id=* | house_profit=%.2f\n", house_profit);

    // Prepara mensagem de fim de rodada
    aviator_msg msg_fim;
    memset(&msg_fim, 0, sizeof(aviator_msg));
    strcpy(msg_fim.type, "round_ended");
    msg_fim.value         = explosion_point;
    msg_fim.player_id     = 0;
    msg_fim.player_profit = 0.0f;
    msg_fim.house_profit  = house_profit;

    // Envia apenas para clientes que APOSTARAM
    for (int i = 0; i < num_clients; i++) {
        int player_id = client_player_ids[i];
        int player_index = player_id - 1;
        
        // Só envia round_ended para quem apostou
        if (player_index >= 0 && player_index < MAX_CLIENTS && players[player_index].has_bet) {
            msg_fim.player_profit = players[player_index].profit;
            msg_fim.player_id = player_id;
            send(connected_clients[i], &msg_fim, sizeof(aviator_msg), 0);
        }
    }

    // Reseta variáveis para próxima rodada (mas mantém player_profits acumulados)
    round_bet_count = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        players[i].bet = 0.0f;
        players[i].has_bet = 0;
        players[i].cashed_out = 0;
        // NÃO zere players[i].profit!
    }

    // apenas marca rodada como encerrada
    round_active = 0;
}

/*
    Descricao: thread responsável por enviar os multiplicadores (a cada 100ms) e disparar a explosão
    Argumentos: ponteiro para argumentos (não utilizado aqui, mas necessário para compatibilidade com pthread)
    Retorno: nenhum
*/
void* multiplier_thread(void* arg) {
    aviator_msg msg;
    memset(&msg, 0, sizeof(msg));
    strcpy(msg.type, "multiplier");
    msg.player_id     = 0;
    msg.player_profit = 0.0f;
    msg.house_profit  = 0.0f;

    multiplier_running = 1;
    current_multiplier = 1.0f;

    while (multiplier_running) {
        // verifica se chegou ao ponto de explosão ANTES de incrementar
        if (current_multiplier >= explosion_point_global) {
            // Log geral da explosão
            printf("event=explode | id=* | m=%.2f\n", current_multiplier);
            
            aviator_msg msg_exp;
            memset(&msg_exp, 0, sizeof(msg_exp));
            strcpy(msg_exp.type, "explode");
            msg_exp.value        = current_multiplier;
            msg_exp.player_id    = 0;
            
            // Calcula os profits para jogadores que não fizeram cash-out
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (players[i].has_bet && !players[i].cashed_out) {
                    // 2) Log individual para cada jogador que perdeu
                    printf("event=explode | id=%d | m=%.2f\n", i + 1, current_multiplier);
                    
                    players[i].profit -= players[i].bet; // Perde a aposta
                    house_profit += players[i].bet; // Casa ganha a aposta
                    
                    // 3) Log do profit individual
                    printf("event=profit | id=%d | player_profit=%.2f\n", i + 1, players[i].profit);
                }
            }
            
            msg_exp.player_profit= 0.0f; // Será definido individualmente
            msg_exp.house_profit = house_profit;
            
            // envia explode apenas para clientes que APOSTARAM
            for (int i = 0; i < num_clients; i++) {
                int player_id = client_player_ids[i];
                int player_index = player_id - 1;
                
                // Só envia explode para quem apostou nesta rodada
                if (player_index >= 0 && player_index < MAX_CLIENTS && players[player_index].has_bet) {
                    msg_exp.player_profit = players[player_index].profit;
                    msg_exp.player_id = player_id;
                    send(connected_clients[i], &msg_exp, sizeof(msg_exp), 0);
                }
            }
            
            multiplier_running = 0;
            
            // Chama notify_round_end() DEPOIS da explosão
            notify_round_end();
            
            // se ainda tiver jogadores, dispara a próxima rodada
            if (num_clients > 0) {
                start_round();
            }
            break; // Sai do loop
        }

        current_multiplier += 0.01f;
        msg.value = current_multiplier;

        // envia multiplicador apenas para clientes que APOSTARAM e não fizeram cashout
        for (int i = 0; i < num_clients; i++) {
            int player_id = client_player_ids[i];
            int player_index = player_id - 1;
            
            // Só envia se o jogador APOSTOU nesta rodada E não fez cashout
            if (player_index >= 0 && player_index < MAX_CLIENTS && players[player_index].has_bet && !players[player_index].cashed_out) {
                send(connected_clients[i], &msg, sizeof(msg), 0);
            }
        }
        // log no servidor
        printf("event=multiplier | id=* | m=%.2f\n", current_multiplier);

        usleep(100 * 1000); // 100ms
    }
    return NULL;
}

/*
    Descricao: thread responsável por controlar o tempo da rodada
    Argumentos: ponteiro para argumentos (não utilizado aqui, mas necessário para compatibilidade com pthread)
    Retorno: nenhum
*/
void* countdown_thread(void* arg) {
    sleep(COUNTDOWN_TIME);

    if (round_active == 1) {
        round_active = 2;

        // Notifica todos os clientes que a janela fechou
        aviator_msg msg_closed;
        memset(&msg_closed, 0, sizeof(aviator_msg));
        strcpy(msg_closed.type, "closed");
        msg_closed.value = 0.0f;
        msg_closed.player_id = 0;
        msg_closed.player_profit = 0.0f;
        msg_closed.house_profit = house_profit;

        for (int i = 0; i < num_clients; i++) {
            send(connected_clients[i], &msg_closed, sizeof(aviator_msg), 0);
        }

        // calcula e armazena ponto de explosão
        explosion_point_global = calculate_explosion_point(round_bet_count, round_bets);

        // log do evento closed
        float total_bets = 0.0f;
        for (int i = 0; i < round_bet_count; i++) {
            total_bets += round_bets[i];
        }
        printf("event=closed | id=* | N=%d | V=%.2f\n", round_bet_count, total_bets);

        // dispara a thread que vai enviar os multiplicadores e explode
        pthread_t tid;
        pthread_create(&tid, NULL, multiplier_thread, NULL);
        pthread_detach(tid);
    }
    return NULL;
}

/*
    Descricao: Calcula o tempo restante da rodada atual (pra mandar pros clientes que chegarem depois do primeiro)
    Argumentos: nenhum
    Retorno: tempo restante em segundos
*/
int find_remaining_time() {
    if (round_active != 1) return 0;

    time_t tempo_atual = time(NULL);
    int tempo_decorrido  = (int)(tempo_atual - round_start_time);
    int tempo_restante   = COUNTDOWN_TIME - tempo_decorrido;

    return tempo_restante > 0 ? tempo_restante : 0;
}

/*
    Descricao: thread que lida com cada cliente conectado
    Esta thread é responsável por gerenciar a comunicação com o cliente, processar apostas e cash
    Argumentos: ponteiro para os argumentos da thread, que contém o socket do cliente e o ID do jogador
    Retorno: nenhum
*/
void * client_thread_handler(void *args_ptr) {
    thread_args_t *args = (thread_args_t *)args_ptr;
    int csock        = args->client_socket;
    int32_t player_id= args->player_id;
    free(args_ptr);

    add_client(csock, player_id);

    // se não houver rodada ativa e somos o 1º cliente, começa
    bool first = (round_active == 0 && num_clients == 1);
    if (first) {
        start_round();
    }

    // se não formos o 1º cliente, mandamos o start “inicial”
    aviator_msg start_msg;
    memset(&start_msg, 0, sizeof(start_msg));
    start_msg.player_id = player_id;
    strcpy(start_msg.type, "start");
    start_msg.value = (float)find_remaining_time();
    int idx = player_id - 1;
    start_msg.player_profit = (idx >= 0 && idx < MAX_CLIENTS) ? players[idx].profit : 0.0f;
    start_msg.house_profit = house_profit;

    if (!first) {
        if (send(csock, &start_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
            logexit("send");
        }
    }

    // Loop principal de comunicação
    aviator_msg received_msg;
    size_t count;
    while ((count = recv(csock, &received_msg, sizeof(aviator_msg), 0)) > 0) {
        if (count == sizeof(aviator_msg)) {
            received_msg.player_id = player_id;

            // Log para bet
            if (strcmp(received_msg.type, "bet") == 0) {
                printf("event = %s | id = %d | bet = %.2f | N = %d\n", received_msg.type, player_id, received_msg.value, num_clients);
            }

            // Verifica se pode apostar
            int remaining_time = find_remaining_time();
            int can_bet = (round_active == 1 && remaining_time > 0);

            aviator_msg response_msg = received_msg;

            if (strcmp(received_msg.type, "bet") == 0) {
                if (can_bet) {
                    int player_index = player_id - 1;
                    
                    // Aceita a aposta
                    if (round_bet_count < MAX_CLIENTS && player_index >= 0 && player_index < MAX_CLIENTS) {
                        // Armazena a aposta tanto para cálculo do ponto de explosão quanto por player
                        round_bets[round_bet_count] = received_msg.value;
                        players[player_index].bet = received_msg.value;
                        players[player_index].has_bet = 1;
                        
                        total_bets_received += received_msg.value;
                        round_bet_count++;
                    }
                }
            }
            else if (strcmp(received_msg.type, "cashout") == 0) {
                int player_index = player_id - 1; // IDs começam em 1, arrays em 0
                
                // Verifica se o jogador apostou nesta rodada e ainda não sacou
                if (player_index >= 0 && player_index < MAX_CLIENTS && players[player_index].has_bet && !players[player_index].cashed_out && (round_active == 2 || multiplier_running)) {
                    
                    printf("event=cashout | id=%d | m=%.2f\n", player_id, current_multiplier);
                    
                    // Calcula o payout
                    float bet_amount = players[player_index].bet;
                    float payout = bet_amount * current_multiplier;
                    float profit = payout - bet_amount;
                    
                    printf("event=payout | id=%d | payout=%.2f\n", player_id, payout);
                    
                    // Atualiza os profits (ACUMULA!)
                    players[player_index].profit += profit;
                    total_payouts += payout;
                    house_profit = total_bets_received - total_payouts;
                    players[player_index].cashed_out = 1; // Marca como fez cash-out

                    printf("event=profit | id=%d | player_profit=%.2f\n", player_id, players[player_index].profit);
                    
                    // Prepara mensagem de resposta
                    strcpy(response_msg.type, "payout");
                    response_msg.value = payout;
                    response_msg.player_profit = players[player_index].profit;
                    response_msg.house_profit = house_profit;
                    
                } else {
                    // Jogador não pode fazer cash-out
                    strcpy(response_msg.type, "cashout_rejected");
                    response_msg.value = 0.0f;
                    response_msg.player_profit = players[player_index >= 0 ? player_index : 0].profit;
                    response_msg.house_profit = house_profit;
                }

                // Envia resposta para cashout
                if (send(csock, &response_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
                    logexit("send");
                }
            }
        }
    }
    
    remove_client(csock);
    close(csock);
    printf("event=bye | id=%d\n", player_id);

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

    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(s, &rfds); // escuta novo cliente
        FD_SET(STDIN_FILENO, &rfds); // escuta 'Q' do teclado
        int maxfd = s > STDIN_FILENO ? s : STDIN_FILENO;

        if (select(maxfd+1, &rfds, NULL, NULL, NULL) < 0) {
            logexit("select");
        }

        // 1) Se o usuário apertou Q + Enter
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            char c = getchar();
            if (c == 'Q') {
                aviator_msg msg_bye;
                memset(&msg_bye, 0, sizeof(msg_bye));
                strcpy(msg_bye.type, "bye");
                msg_bye.player_id = 0;
                msg_bye.value     = 0.0f;
                printf("event=bye | id=*\n");
                // envia bye e fecha todos
                for (int i = 0; i < num_clients; i++) {
                    send(connected_clients[i], &msg_bye, sizeof(msg_bye), 0);
                    close(connected_clients[i]);
                }
                close(s);
                exit(EXIT_SUCCESS);
            }
        }

        // 2) Se chegou conexão nova
        if (FD_ISSET(s, &rfds)) {
            struct sockaddr_storage client_storage;
            socklen_t client_addr_len = sizeof(client_storage);
            int csock = accept(s, (struct sockaddr*)&client_storage, &client_addr_len);
            if (csock < 0) logexit("accept");

            pthread_t thread_id;
            thread_args_t *args = (thread_args_t*)malloc(sizeof(thread_args_t));
            args->client_socket = csock;
            args->player_id     = next_player_id++;
            pthread_create(&thread_id, NULL, client_thread_handler, args);
            pthread_detach(thread_id);
        }
    }
    exit(EXIT_SUCCESS);
}