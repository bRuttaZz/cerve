
#ifndef MESSAGES_H
#define MESSAGES_H

#define CERVE_RESPONSE_HEADERS "Server: Cerve"

typedef struct {
    char *method;
    char *location;
    char *version;      // currently only concerned about these things
} Request;

typedef struct {

} Response;

/**
@brief read http request-line

Only read the first 8000 octets from the wire, as per the HTTP/1.1 standard
refer: https://httpwg.org/specs/rfc9112.html

Returns
-------
<0 : error reading data from the socket
0 : request successfully transilated
1 : fail reading request-line (malformatted request-line)
2 : invalid http method
*/
int read_http_request(int sock, Request *request);

/**
Free a request object
*/
void free_request(Request *req);

#endif  /* MESSAGES_H */
