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

#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 1024
#define COUNTDOWN_TIME 10 // Tempo em segundos da janela de apostas
#define MAX_CLIENTS 10 // Máximo de clientes simultâneos

int32_t next_player_id = 1; // Variável global para ID do jogador
int round_active = 0; // Flag para controlar se a rodada está ativa (0 = não iniciada, 1 = ativa, 2 = encerrada)
time_t round_start_time = 0; // Timestamp do início da rodada
pthread_mutex_t round_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para proteger variáveis da rodada

// Array para armazenar sockets dos clientes conectados
int connected_clients[MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para proteger array de clientes

// Struct para passar argumentos para a thread do cliente.
typedef struct {
    int client_socket;
    int32_t player_id;
} thread_args_t;

void usage (int argc, char **argv) {
    printf("Usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("Example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

// Função para adicionar cliente ao array de clientes conectados
void add_client(int socket_cliente) {
    pthread_mutex_lock(&clients_mutex);
    if (num_clients < MAX_CLIENTS) {
        connected_clients[num_clients] = socket_cliente;
        num_clients++;
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Função para remover cliente do array de clientes conectados
void remove_client(int socket_cliente) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (connected_clients[i] == socket_cliente) {
            // Move todos os elementos após este para uma posição anterior
            for (int j = i; j < num_clients - 1; j++) {
                connected_clients[j] = connected_clients[j + 1];
            }
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Função para notificar todos os clientes conectados que as apostas foram encerradas
void notify_timeout() {
    aviator_msg msg_encerramento;
    memset(&msg_encerramento, 0, sizeof(aviator_msg));
    strcpy(msg_encerramento.type, "round_ended");
    msg_encerramento.value = 0.0;
    msg_encerramento.player_id = 0;
    msg_encerramento.player_profit = 0.0;
    msg_encerramento.house_profit = 0.0;

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        send(connected_clients[i], &msg_encerramento, sizeof(aviator_msg), 0);
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Thread que controla o tempo da rodada
void* countdown_thread(void* arg) {
    sleep(COUNTDOWN_TIME); // Espera o tempo da rodada
    
    pthread_mutex_lock(&round_mutex);
    if (round_active == 1) {
        round_active = 2;
        printf("Apostas encerradas! Não é mais possível apostar nesta rodada.\n");
        notify_timeout(); // Notifica todos os clientes
    }
    pthread_mutex_unlock(&round_mutex);
    
    return NULL;
}
int find_remaining_time() {
    if (round_active != 1) return 0;
    
    time_t tempo_atual = time(NULL);
    int tempo_decorrido = (int)(tempo_atual - round_start_time);
    int tempo_restante = COUNTDOWN_TIME - tempo_decorrido;
    
    return tempo_restante > 0 ? tempo_restante : 0;
}

// Função que será executada por cada thread
void * client_thread_handler(void *args_ptr) {
    // Extrai os argumentos (socket e player_id) e libera a memória da struct
    thread_args_t *args = (thread_args_t *)args_ptr;
    int csock = args->client_socket;
    int32_t player_id = args->player_id;
    free(args_ptr);

    char client_addr_str[BUFSZ];
    struct sockaddr_storage client_storage;
    struct sockaddr *client_addr = (struct sockaddr *)&client_storage;
    socklen_t client_addr_len = sizeof(client_storage);

    // Obtém informações do cliente conectado para log
    getpeername(csock, client_addr, &client_addr_len);
    addrtostr(client_addr, client_addr_str, BUFSZ);
    //printf("Thread de iniciada o cliente de ID: %d\n", player_id);

    // Adiciona cliente ao array de clientes conectados
    add_client(csock);

    // Inicia a rodada se for o primeiro cliente
    pthread_mutex_lock(&round_mutex);
    if (round_active == 0) {
        round_active = 1;
        round_start_time = time(NULL);
        //printf("Primeira conexão detectada! Iniciando contagem regressiva.\n");
        printf("event = start | id = * | N = 1\n");
        
        // Cria thread para controlar o tempo da rodada
        pthread_t time_thread;
        pthread_create(&time_thread, NULL, countdown_thread, NULL);
        pthread_detach(time_thread);
    }
    pthread_mutex_unlock(&round_mutex);

    // Envia mensagem inicial de start para o cliente
    aviator_msg start_msg;
    memset(&start_msg, 0, sizeof(aviator_msg));
    start_msg.player_id = player_id;
    strcpy(start_msg.type, "start");
    start_msg.value = (float)find_remaining_time(); // Tempo restante no momento da conexão
    start_msg.player_profit = 0.0;
    start_msg.house_profit = 0.0;

    if(send(csock, &start_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
        logexit("send");
    }

    // Lógica de comunicação com o cliente
    aviator_msg received_msg;
    size_t count;
    while((count = recv(csock, &received_msg, sizeof(aviator_msg), 0)) > 0) {
        // Garante que uma struct completa foi recebida
        if (count == sizeof(aviator_msg)) {
            // Atribui o ID do jogador à mensagem recebida
            received_msg.player_id = player_id;

            printf("event = %s | id = %d | bet = %.2f | N = %d\n", received_msg.type, player_id, received_msg.value, num_clients);

            // Verifica se ainda é possível apostar
            pthread_mutex_lock(&round_mutex);
            int remaining_time = find_remaining_time();
            int can_bet = (round_active == 1 && remaining_time > 0);
            pthread_mutex_unlock(&round_mutex);

            // Prepara a resposta
            aviator_msg response_msg = received_msg;
            
            if (strcmp(received_msg.type, "bet") == 0) {
                if (can_bet) {
                    // Aposta aceita
                    strcpy(response_msg.type, "bet_accepted");
                    response_msg.player_profit = 0.0; // Por enquanto sem lucro
                    response_msg.house_profit = 0.0;
                } else {
                    // Aposta rejeitada (tempo esgotado)
                    strcpy(response_msg.type, "bet_rejected");
                    response_msg.value = 0.0;
                    response_msg.player_profit = 0.0;
                    response_msg.house_profit = 0.0;
                }
            }

            // Envia a resposta para o cliente
            if(send(csock, &response_msg, sizeof(aviator_msg), 0) != sizeof(aviator_msg)) {
                logexit("send");
            }
        }
    }

    // Remove cliente do array antes de fechar a conexão
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