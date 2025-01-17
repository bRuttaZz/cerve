// contains some simple test utils
#include "../../../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>

/**
@brief a primitve function to raise an http request meant for testing purposes
@param hostname - hostname or ip address to be resolved
@param port - address port
@param path - request path
@param req_text - request string to be sent (note: the content type will always be text/plain)
@param method - http method to be used
@returns
    0: success
    1: error creating conneciton socket
    6: error resolving hostname
    7: error connecting to resolved host ip
    8: error sending request
    9: error recieving response
    -1: memory error
*/
int raise_http_request(char *hostname, char *port, char *path, char *req_text, char *method, char *resp_buffer, int max_recv_len) {
    const char *req_format = "%s %s HTTP/1.1\r\nContent-Type: text/plain\nContent-Length: %d\n\n%s";

    // get server address info
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // tcp

    // get address info (DNS lookup)
    if (getaddrinfo(hostname, port, &hints, &res) != 0) {
        g_logger.error("[raise_http_request] error resolving '%s:%s'\n", hostname, port);
        return 6; // exit code mimics `curl`
    }

    // create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        g_logger.error("[raise_http_request] socket creation failed!\n");
        return 1;
    }

    // connect to the server
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        g_logger.error("connection failed");
        close(sockfd);
        freeaddrinfo(res);
        return 7;    // exit code mimics `curl`
    }
    freeaddrinfo(res);

    char *req = (char *)malloc(
        strlen(req_format) + strlen(method) + strlen(path) + 6  // 6 for maximum 6 digit content-length :)
        + strlen(req_text) + 1
    );
    if (req == NULL) {
        g_logger.error("[raise_http_request] error getting memory! seems like a resource crunch!\n");
        return -1;
    }
    sprintf(req, req_format, method, path, strlen(req_text), req_text);

    // sending info
    if (send(sockfd, req, strlen(req), 0) < 0) {
        g_logger.error("[raise_http_request] error sending request to host!");
        close(sockfd);
        free(req);
        return 8;
    }
    free(req);

    // reading socket (I'm feeling bit lazy now. so instead of respecting the content-length I'm always reading a 1 byte of data)
    int bytes_received = recv (sockfd, resp_buffer, max_recv_len, 0);
    if (bytes_received > 0) {
        resp_buffer[bytes_received] = '\0';
    }
    if (bytes_received < 0) {
        g_logger.error("[raise_http_request] receive failed!\n");
        close(sockfd);
        return 9;
    }
    close(sockfd);
    return 0;
}
