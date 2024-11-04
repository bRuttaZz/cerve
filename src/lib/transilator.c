#include "../../include/messages.h"
#include "../../include/utils.h"
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>


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

/**
@brief write headers to the socket
*/
int _write_headers(int sock, char **custom_header) {
    if (strlen(*custom_header)) {
        write(sock, *custom_header, strlen(*custom_header));
        write(sock, "\r\n", 2);
    }
    write(sock, CERVE_RESPONSE_HEADERS, strlen(CERVE_RESPONSE_HEADERS));
    write(sock, "\r\n", 2);
    return 0;
}

/**
@brief helper fun to transform string to lower
*/
void _convert_to_lower(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

/**
@brief get mime type of a file
*/
const char* _get_mime_type(const char* filename) {
    char* ext = strrchr(filename, '.');
    if (!ext) {
        return "application/octet-stream"; // default
    }
    _convert_to_lower(ext);

    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".json") == 0) return "application/json";
    // TODO: adding more mimetypes

    return "application/octet-stream"; // default again
}


/**
@brief get a file from the path
returns
    0: if succeed
    1: if not able to resolve the filepath
    2: if a directory
*/
int _get_file(const char* filename, FILE** file) {
    struct stat file_info;

    if (stat(filename, &file_info) == 0) {
        if (S_ISREG(file_info.st_mode)) {
            *file = fopen(filename, "r");
            if (!*file) {
                return 1;
            }
            return 0;
        }
        else if (S_ISDIR(file_info.st_mode))
            return 2;
    }
    return 1;
}

/**
@brief check for file/html pattern in server
@param filename - name of file pattern
@param file - file object
@returns
    0: on successfully found a file
    1: on not finding any file
    2: on unexpected search errors
*/
int _check_for_file_pattern_in_path(char** file_name, FILE** file) {
    int resp_status;
    int last_try = 0;

    while (1) {
        resp_status = _get_file(*file_name, file);
        g_logger.debug("search result : %d : %s", resp_status, *file_name);
        if (resp_status==0) {
            return 0;
        } else if (last_try) {
            break;
        } else if (resp_status==1) {        // if no path check for html file
            strcat(*file_name, ".html");
            last_try ++;
        } else if (resp_status==2) {        // if dir check for index file
            if ((*file_name)[strlen(*file_name)-1] != '/')
                strcat(*file_name, "/");
            strcat(*file_name, "index.html");
            last_try ++;
        } else {
            g_logger.error("Error searching for file paths %d!", resp_status);
            return 2;
        }
    }
    return 1;
}


/**
@brief write http response to the wire and return the status code
@param sock - socket object to write
@param request - http request struct
@param resp_headers - http response headers
@param serve_dir - serve directory
*/
int write_http_response(int sock, Request *request, char **resp_headers, const char *serve_dir) {
    long int file_size;
    char _msg[1024];
    char *file_name;
    FILE *file = NULL;

    // TODO: catch write errors
    write(sock, request->version, strlen(request->version));
    //                                                                  100 for adding serching purposes
    file_name = malloc((strlen(request->location) + strlen(serve_dir) + 100 + 1) * sizeof(char));
    if (!file_name) {
        sprintf(_msg, " 500 Internal Server Error\r\n");
        write(sock, _msg, strlen(_msg));
        _write_headers(sock, resp_headers);

        // the content
        sprintf(_msg, "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %ld\r\n\r\n", strlen(RESPONSE_BODY_500));
        write(sock, _msg, strlen(_msg));
        write(sock, RESPONSE_BODY_500, strlen(RESPONSE_BODY_500));

        g_logger.error("[http write error] error allocating mem");
        return 500;
    }
    strcat(file_name, serve_dir);
    strcat(file_name, request->location);


    if (!_check_for_file_pattern_in_path(&file_name, &file)){
        // found file
        sprintf(_msg, " 200 Ok\r\n");
        write(sock, _msg, strlen(_msg));
        _write_headers(sock, resp_headers);

        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        sprintf(_msg, "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n\r\n", _get_mime_type(file_name), file_size);
        write(sock, _msg, strlen(_msg));

        while (1) {
            file_size = fread(_msg, sizeof(char), 1024, file);
            if (file_size <= 0)
                break;
            write(sock, _msg, file_size);
        }

        free(file_name);
        fclose(file);
        return 200;
    };

    // no file
    sprintf(_msg, " 404 Not Found\r\n");
    write(sock, _msg, strlen(_msg));
    _write_headers(sock, resp_headers);

    sprintf(_msg, "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: %ld\r\n\r\n", strlen(RESPONSE_BODY_404));
    write(sock, _msg, strlen(_msg));
    write(sock, RESPONSE_BODY_404, strlen(RESPONSE_BODY_404));

    g_logger.debug("path not found %s (%s)", request->location, file_name);

    free(file_name);
    return 404;
}
