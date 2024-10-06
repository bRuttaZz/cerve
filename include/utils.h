
#ifndef UTILS_H
#define UTILS_H

// logger
// log becomes detailed as increaing the level (0-> error)
enum LogLevel {
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
};

struct Logger {
    enum LogLevel level;
    void (*debug) (const char* message);
    void (*info) (const char* message);
    void (*warning) (const char* message);
    void (*error) (const char* message);
};

extern struct Logger g_logger;


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
