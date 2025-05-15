# include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>

void logexit (const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) {
    if (addrstr == NULL || portstr == NULL) {
        return -1; // Invalid arguments
    }

    uint16_t port = (uint16_t)atoi(portstr); // unsigned short

    if (port == 0) {
        return -1; // Invalid port
    }

    port = htons(port); // Convert to network byte order

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
        memcpy(&addr6->sin6_addr, &inaddr6, sizeof(inaddr6)); // Copy the address
        return 0; // Success
    }

    return -1; // Invalid address
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    int version; 
    char addrstr[INET6_ADDRSTRLEN + 1] = ""; // Buffer for IPv4 and IPv6 addresses
    uint16_t port;

    if (addr->sa_family == AF_INET) {
        // IPv4
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            logexit("inet_ntop");
        }
        port = ntohs(addr4->sin_port); // Convert port to host byte order
    } else if (addr->sa_family == AF_INET6) {
        // IPv6
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            logexit("inet_ntop");
        }
        port = ntohs(addr6->sin6_port); // Convert port to host byte order
    } else {
        logexit("Unknown address family");
    }

    if(str) {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port); // Format the address and port
    }
}

int server_sockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1; // Invalid port
    }
    port = htons(port); // Convert to network byte order

    memset(storage, 0, sizeof(*storage)); // Clear the storage
    if (0 == strcmp(proto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    } else if (0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6->sin6_addr = in6addr_any; // Bind to all interfaces
    } else {
        return -1; // Invalid protocol
    }

    return 0; // Success
}

int valid_move(int opt) {
    if (opt == 0 || opt == 1 || opt == 2 || opt == 3 || opt == 4) {
        return 1; // Valid move
    }
    return 0;
}

const char* option_to_action(int opt) {
    switch (opt) {
        case 0: return "Nuclear Attack";
        case 1: return "Intercept Attack";
        case 2: return "Cyber Attack";
        case 3: return "Drone Strike";
        case 4: return "Bio Attack";
        default: return "Invalid Option";
    }
}