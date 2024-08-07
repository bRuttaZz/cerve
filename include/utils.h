
#ifndef UTILS_H
#define UTILS_H

/**
@brief a primitve function to raise an http request meant for testing purposes
@param hostname - hostname or ip address to be resolved
@param port - address port
@param path - request path
@param req_text - request string to be sent (note: the content type will always be text/plain)
@param method - http method to be used
@returns response buffer
*/
void raise_http_request(char *hostname, char *port, char *path, char *req_text, char *method, char *resp_buffer, int max_recv_len);


#endif /* UTILS_H */
