
#ifndef MESSAGES_H
#define MESSAGES_H

// EDIT caution: do not terminate with newline
#define CERVE_RESPONSE_HEADERS "Server: Cerve"

#define RESPONSE_BODY_404 "<!doctype html>" \
"<html lang=\"en\">"    \
"    <head>"   \
"        <meta charset=\"UTF-8\" />"    \
"        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />" \
"        <title>404</title>" \
"        <style>"   \
"            body {"   \
"                width: 35em;  " \
"                margin: 0 auto; " \
"            }" \
"        </style>"  \
"    </head>"   \
"    <body>"    \
"        <h1>404 : Not Found</h1>"  \
"        <p>Oops! The webpage you're looking for is not here :)</p>"    \
"        <p>(Cerve)</p>"    \
"    </body>"   \
"</html>"   \

#define RESPONSE_BODY_500 "<!doctype html>" \
"<html lang=\"en\">"    \
"    <head>"   \
"        <meta charset=\"UTF-8\" />"    \
"        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />" \
"        <title>500</title>" \
"        <style>"   \
"            body {"   \
"                width: 35em;  " \
"                margin: 0 auto; " \
"            }" \
"        </style>"  \
"    </head>"   \
"    <body>"    \
"        <h1>500 : Internal Server Error</h1>"  \
"        <p>Oops! Something went terribly bad :/</p>"    \
"        <p>(Cerve)</p>"    \
"    </body>"   \
"</html>"   \


typedef struct {
    char *method;
    char *location;
    char *version;      // currently only concerned about these things
} Request;

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

/**
@brief write http response to the wire and return the status code
@param sock - socket object to write
@param request - http request struct
@param resp_headers - http response headers
@param serve_dir - serve directory
*/
int write_http_response(int sock, Request *request, char **resp_headers, const char *serve_dir);

#endif  /* MESSAGES_H */
