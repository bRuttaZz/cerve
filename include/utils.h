
#ifndef UTILS_H
#define UTILS_H

#ifndef LOGGER_PREFIX_LEN
#define LOGGER_PREFIX_LEN 4     // can be shorten to avoid logging prefix for lower order log levels
#endif

#ifndef DEFAULT_LOGLEVEL
#define DEFAULT_LOGLEVEL 99
#endif

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
    void (*debug) (const char* message, ...);
    void (*info) (const char* message, ...);
    void (*warning) (const char* message, ...);
    void (*error) (const char* message, ...);
};

extern struct Logger g_logger;


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
int raise_http_request(char *hostname, char *port, char *path, char *req_text, char *method, char *resp_buffer, int max_recv_len);


/**
@brief get mime type of a file
@param filename - filename to be used
@return mimetype string
*/
const char* get_mime_type(const char* filename);

#endif /* UTILS_H */
