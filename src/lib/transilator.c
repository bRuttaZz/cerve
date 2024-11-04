#include "../../include/messages.h"
#include "../../include/utils.h"
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

const int g_request_line_buff_size = 8000;
const char *g_request_methods[] = {
    "GET",
    "POST",
    "PUT",
    "DELETE",
    "PATCH",
    "HEAD",
    "OPTIONS",
    "TRACE",
    "CONNECT",
};
const int g_request_methods_len = 9;

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
int read_http_request(int sock, Request *request) {
    int buff_readed;
    char buffer[g_request_line_buff_size + 1];
    char lookup_chars[] = "  \n";
    int lookup_char_index = 0;
    int lookup_cursor = 0;
    char *data_chunks ;
    int found_flag = 0;


    buff_readed = read(sock, buffer, g_request_line_buff_size);
    if (buff_readed < 0) {
        g_logger.debug("[req reader] Error reading data from socket. Connection skipped!");
        return buff_readed;
    }
    buffer[buff_readed + 1] = '\0';

    for (int i=0; i<buff_readed; i++) {
        if (buffer[i]==lookup_chars[lookup_char_index]) {
            data_chunks = (char *) malloc((i - lookup_cursor +1) * sizeof(char));
            strncpy(data_chunks, buffer + lookup_cursor, i-lookup_cursor);
            data_chunks[i-lookup_cursor] = '\0';

            switch (lookup_char_index) {
                case 0:         // the method
                    // also makking sure to capitlize
                    for (int j=0; data_chunks[j] != '\0'; j++) {
                        data_chunks[j] = toupper((unsigned char) data_chunks[j]);
                    }
                    for (int j=0; j<g_request_methods_len; j++) {
                        if (!strcmp(data_chunks, g_request_methods[j])) {
                            found_flag = 1;
                            break;
                        }
                    }
                    if (!found_flag) {
                        return 2;
                    }
                    request->method = data_chunks;
                    break;
                case 1:         // location
                    request->location = data_chunks;
                    break;
                case 2:         // and the version
                    if (data_chunks[i-lookup_cursor-1]=='\r')
                        data_chunks[i-lookup_cursor-1] = '\0';
                    request->version = data_chunks;
                    return 0;

                default:        // just in case
                    free(data_chunks);
                    return 0;
            }
            lookup_cursor = i+1;
            lookup_char_index ++;
        }
    }
    return 1;
}


/**
Free a request object
*/
void free_request(Request *req) {
    if (req->location!=NULL) free(req->location);
    if (req->method!=NULL) free(req->method);
    if (req->version!=NULL) free(req->version);
    req->location = NULL;
    req->version = NULL;
    req->method = NULL;
}


int write_http_response(int sock, Request *request) {
    return 0;
}
