/*
-----------------------------------------------------
Arquivo common.C referente ao TP2
Implementacao de funcoes comuns
Materia Redes de Computadores
Semestre 2025/01

Aluna: Raissa Gonçalves Diniz
Matricula: 2022055823
-----------------------------------------------------
*/
# include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>

/*
    Descricao: Funcao que imprime a mensagem de erro e encerra o programa
    Argumentos: msg - mensagem de erro
    Retorno: Nao possui           
*/
void logexit (const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/*
    Descricao: Funcao que converte o endereco e a porta para uma estrutura sockaddr_storage
    Argumentos: addrstr - endereco IP
               portstr - porta
               storage - estrutura onde sera armazenado o endereco
    Retorno: 0 se sucesso, -1 se falha           
*/
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) {
    if (addrstr == NULL || portstr == NULL) {
        return -1; // Argumentos inválidos
    }

    uint16_t port = (uint16_t)atoi(portstr); // unsigned short

    if (port == 0) {
        return -1; // Port inválido
    }

    port = htons(port); // Converter para ordem de bytes de rede

    struct in_addr inaddr4; // IPv4 address - 32 bits
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0; // Success
    }

    struct in6_addr inaddr6; // IPv6 address - 128 bits
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6;
        memcpy(&addr6->sin6_addr, &inaddr6, sizeof(inaddr6)); // Copia o o endereço IPv6
        return 0; // sucesso
    }

    return -1; // Endereço inválido
}

/*
    Descricao: Funcao que converte o endereco e a porta para uma string
    Argumentos: addr - endereco IP
               str - string onde sera armazenado o endereco
               strsize - tamanho da string
    Retorno: Nao possui           
*/
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    int version; 
    char addrstr[INET6_ADDRSTRLEN + 1] = ""; // Buffer para armazenar o endereço
    uint16_t port;

    if (addr->sa_family == AF_INET) {
        // IPv4
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            logexit("inet_ntop");
        }
        port = ntohs(addr4->sin_port); // Converte a porta para a ordem de bytes do host
    } else if (addr->sa_family == AF_INET6) {
        // IPv6
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            logexit("inet_ntop");
        }
        port = ntohs(addr6->sin6_port); // Converte a porta para a ordem de bytes do host
    } else {
        logexit("Unknown address family");
    }

    if(str) {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port); // Formata a string
    }
}

/*         
    Descricao: Funcao que preenche uma estrutura de endereco para socket servidor IPv4 ou IPv6, escutando em todas as interfaces na porta especificada
    Argumentos: proto - protocolo (v4 ou v6)
               portstr - porta
               storage - estrutura onde sera armazenado o endereco
    Retorno: 0 se sucesso, -1 se falha
*/
int server_sockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1; // Port inválido
    }
    port = htons(port); // Converte para ordem de bytes de rede

    memset(storage, 0, sizeof(*storage)); // Limpa a estrutura
    if (0 == strcmp(proto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr.s_addr = INADDR_ANY; // Bind
    } else if (0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6->sin6_addr = in6addr_any; // Bind
    } else {
        return -1; //Protocolo inválido
    }

    return 0; // Sucesso
}