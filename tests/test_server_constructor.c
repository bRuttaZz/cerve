#include "../include/server.h"
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

void test_launch(struct Server * server) {
    int address_len = sizeof(server->address);
    char buffer[1024];
    char *resp_message = "HTTP/1.1 200 OK\nServer: The Super Duper Server\nContent-Type: text/plain\nContent-Length: 9\n\nHi There\n";
    int new_sock;

    while (1) {
        printf("===== Waiting for Connection =======\n");
        new_sock = accept(
            server->socket, (struct sockaddr *)&server->address, (socklen_t *)&address_len
        );
        printf("Got a new socket connection (%d)\n", new_sock);

        read(new_sock, buffer, sizeof(buffer));
        printf("Got message : %s\n", buffer);
        // jeem boomba .. (whatever be that, do the magic over here!)

        write(new_sock, resp_message, strlen(resp_message));

        close(new_sock);
        printf("===== Connection closed =======\n");
    }

}

void test_server_constructor() {
    struct Server test_server = server_constructor(
        AF_INET, SOCK_STREAM, 0, 8001, 2, INADDR_ANY, 1, test_launch
    );
    test_server.launch(&test_server);
}
