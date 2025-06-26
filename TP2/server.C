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

#define BUFSZ 1024
#define COUNTDOWN_TIME 10 // Tempo da janela de apostas
#define MAX_CLIENTS 10 // Máximo de clientes

// Variáveis globais
int32_t next_player_id = 1;
int round_active = 0; // 0 = não iniciada, 1 = ativa, 2 = encerrada
time_t round_start_time = 0;

// Array para clientes conectados
int connected_clients[MAX_CLIENTS];
int num_clients = 0;

// Array para apostas
float round_bets[MAX_CLIENTS];
int round_bet_count = 0;

// Struct para argumentos da thread
typedef struct {
    int client_socket;
    int32_t player_id;
} thread_args_t;

void usage (int argc, char **argv) {
    printf("Usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("Example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

// Calcula o ponto de explosão baseado nas apostas
float calculate_explosion_point(int num_players, float bets[]) {
    float total_bets = 0.0f;
    
    for (int i = 0; i < num_players; i++) {
        total_bets += bets[i];
    }
    
    float explosion_point = sqrtf(1.0f + (float)num_players + 0.01f * total_bets);
    return explosion_point;
}

// Adiciona cliente (versão simples)
void add_client(int socket_cliente) {
    if (num_clients < MAX_CLIENTS) {
        connected_clients[num_clients] = socket_cliente;
        num_clients++;
    }
}

// Remove cliente (versão simples)
void remove_client(int socket_cliente) {
    for (int i = 0; i < num_clients; i++) {
        if (connected_clients[i] == socket_cliente) {
            // Move elementos para frente
            for (int j = i; j < num_clients - 1; j++) {
                connected_clients[j] = connected_clients[j + 1];
            }
            num_clients--;
            break;
        }
    }
}

// Notifica fim da rodada para todos os clientes
void notify_round_end() {
    float explosion_point = 0.0;
    
    if (round_bet_count > 0) {
        explosion_point = calculate_explosion_point(round_bet_count, round_bets);
    }
    
    printf("Ponto de explosão calculado: %.2f\n", explosion_point);
    
    // Prepara mensagem de fim de rodada
    aviator_msg msg_fim;
    memset(&msg_fim, 0, sizeof(aviator_msg));
    strcpy(msg_fim.type, "round_ended");
    msg_fim.value = explosion_point;
    msg_fim.player_id = 0;
    msg_fim.player_profit = 0.0;
    msg_fim.house_profit = 0.0;

    // Envia para todos os clientes conectados
    for (int i = 0; i < num_clients; i++) {
        send(connected_clients[i], &msg_fim, sizeof(aviator_msg), 0);
    }
    
    // Reseta variáveis para próxima rodada
    round_bet_count = 0;
    memset(round_bets, 0, sizeof(round_bets));
    round_active = 0; // Permite nova rodada
}

// Thread que controla o tempo
void* countdown_thread(void* arg) {
    sleep(COUNTDOWN_TIME);
    
    if (round_active == 1) {
        round_active = 2;
        printf("Apostas encerradas!\n");
        notify_round_end();
    }
    
    return NULL;
}

// Calcula tempo restante
int find_remaining_time() {
    if (round_active != 1) return 0;
    
    time_t tempo_atual = time(NULL);
    int tempo_decorrido = (int)(tempo_atual - round_start_time);
    int tempo_restante = COUNTDOWN_TIME - tempo_decorrido;
    
    return tempo_restante > 0 ? tempo_restante : 0;
}

// Thread para cada cliente
void * client_thread_handler(void *args_ptr) {
    thread_args_t *args = (thread_args_t *)args_ptr;
    int csock = args->client_socket;
    int32_t player_id = args->player_id;
    free(args_ptr);

    // Adiciona cliente
    add_client(csock);

    // Se for o primeiro cliente, inicia rodada
    if (round_active == 0) {
        round_active = 1;
        round_start_time = time(NULL);
        printf("event = start | id = * | N = 1\n");
        
        // Cria thread de tempo
        pthread_t time_thread;
        pthread_create(&time_thread, NULL, countdown_thread, NULL);
        pthread_detach(time_thread);
    }

    // Envia mensagem inicial
    aviator_msg start_msg;
    memset(&start_msg, 0, sizeof(aviator_msg));
    start_msg.player_id = player_id;
    strcpy(start_msg.type, "start");
    start_msg.value = (float)find_remaining_time();
    start_msg.player_profit = 0.0;
    start_msg.house_profit = 0.0;

    if(send(csock, &start_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
        logexit("send");
    }

    // Loop principal de comunicação
    aviator_msg received_msg;
    size_t count;
    while((count = recv(csock, &received_msg, sizeof(aviator_msg), 0)) > 0) {
        if (count == sizeof(aviator_msg)) {
            received_msg.player_id = player_id;

            printf("event = %s | id = %d | bet = %.2f | N = %d\n", 
                   received_msg.type, player_id, received_msg.value, num_clients);

            // Verifica se pode apostar
            int remaining_time = find_remaining_time();
            int can_bet = (round_active == 1 && remaining_time > 0);

            aviator_msg response_msg = received_msg;
            
            if (strcmp(received_msg.type, "bet") == 0) {
                if (can_bet) {
                    // Aceita a aposta
                    if (round_bet_count < MAX_CLIENTS) {
                        round_bets[round_bet_count] = received_msg.value;
                        round_bet_count++;
                    }
                    
                    strcpy(response_msg.type, "bet_accepted");
                    response_msg.player_profit = 0.0;
                    response_msg.house_profit = 0.0;
                } else {
                    // Rejeita a aposta
                    strcpy(response_msg.type, "bet_rejected");
                    response_msg.value = 0.0;
                    response_msg.player_profit = 0.0;
                    response_msg.house_profit = 0.0;
                }
            }

            // Envia resposta
            if(send(csock, &response_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
                logexit("send");
            }
        }
    }

    // Limpa ao sair
    remove_client(csock);
    close(csock);
    printf("Conexão com o jogador %d encerrada.\n", player_id);

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
    //printf("Servidor aguardando conexões em %s...\n", addrstr);

    while(1) {
        struct sockaddr_storage client_storage;
        struct sockaddr *client_addr = (struct sockaddr *)&client_storage;
        socklen_t client_addr_len = sizeof(client_storage);

        // Aceita uma nova conexão de cliente
        int csock = accept(s, client_addr, &client_addr_len);
        if (csock == -1) {
            logexit("accept");
        }

        char client_addr_str[BUFSZ];
        addrtostr(client_addr, client_addr_str, BUFSZ);
        //printf("Accepted connection from %s\n", client_addr_str);

        pthread_t thread_id;
        // Aloca memória para os argumentos da thread (socket + player_id)
        thread_args_t *args = (thread_args_t *)malloc(sizeof(thread_args_t));
        if (!args) {
            logexit("malloc");
        }
        args->client_socket = csock;
        args->player_id = next_player_id++; // Atribui o ID atual e incrementa para o próximo

        // Cria uma nova thread para atender o cliente, passando a struct de argumentos
        if (pthread_create(&thread_id, NULL, client_thread_handler, args) < 0) {
            logexit("could not create thread");
        }

        pthread_detach(thread_id); // Libera recursos da thread automaticamente ao terminar
    }

    exit(EXIT_SUCCESS);
}