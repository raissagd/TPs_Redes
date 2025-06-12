/*
-----------------------------------------------------
Arquivo common.H referente ao TP2
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

#define STR_LEN 11

typedef struct{
    int32_t player_id;
    float value;
    char type[STR_LEN];
    float player_profit;
    float house_profit;
} aviator_msg ;

void logexit (const char *msg);

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage);