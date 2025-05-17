/*
-----------------------------------------------------
Arquivo common.H referente ao TP1
Implementacao do cabeçalho das funcoes comuns
Materia Redes de Computadores
Semestre 2025/01

Aluna: Raissa Gonçalves Diniz
Matricula: 2022055823
-----------------------------------------------------
*/
#pragma once
#include <stdlib.h>

#include <arpa/inet.h>

#define MSG_SIZE 256

typedef enum {
    MSG_REQUEST,
    MSG_RESPONSE,
    MSG_RESULT,
    MSG_PLAY_AGAIN_REQUEST,
    MSG_PLAY_AGAIN_RESPONSE,
    MSG_ERROR,
    MSG_END
} MessageType;

typedef struct {
    int type;              
    int client_action;     // (0 a 4)
    int server_action;     // (0 a 4)
    int result;            // Resultado: 1 (vitória cliente), 0 (empate), -1 (vitória servidor)
    int client_wins;       
    int server_wins;       
    char message[MSG_SIZE];
} GameMessage;

void logexit (const char *msg);

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage);