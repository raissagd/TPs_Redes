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

void logexit (const char *msg);

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage);

int valid_move(int opt);

int valid_playagain(int opt);