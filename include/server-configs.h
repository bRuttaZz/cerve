#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <arpa/inet.h>

#ifndef VERSION
#define VERSION "0.0.0" // meant to be overwrite
#endif // VERSION

#ifndef DBUG_MODE
#define DBUG_MODE 0
#endif // DBUG_MODE

#define SERVER_BACKLOG 100

#define DEFAULT_WORKERS 4
#define DEFAULT_PORT 8000

#define PROG_NAME "cerve"
#define HELP_MSG  \
"usage: cerve [OPTIONS] \n\n" \
\
"Options \n" \
"   -h --help       print help and exit \n"\
"   --version       print version and exit \n" \
"   -v --verbose    an integer between 1 and 4, inclusive of those values representing \n" \
"                   different log levels. higher values for detailed logs. 1 for minimum logging \n" \
"                   default to 4 \n" \
"   -w --workers    number of workers to be used \n" \
"                   defaults to 4 \n" \
"   -p  --port      system port at which the server to listen for connections. \n"\
"                   defaults to 8000 \n"\
"   --disable-socket-reuse      (flag) if provided the SO_REUSEADDR will not be set. \n"\
"   --res-headers   add custom response headers. Defaults to none.\n" \
"                   expects a newline separated utf-8 text files with colon separated key values.\n" \
"                   syntax reference: https://httpwg.org/specs/rfc9112.html#rfc.section.2.1 \n" \
"   --serve-dir     specify custom serve directory.\n" \
"                   defaults to the current working directory\n" \


extern int g_server_port;
extern int g_worker_count;

extern int g_enable_socket_reuse;   //  if set to 0 (default to 1), SO_REUSEADDR will not be enabled

// custom config files
extern char * g_custom_resp_header_file_path;   // header file to be served along wth each response
extern char g_custom_serve_directory[];         // directory to be served over http

/**
@brief configure values based on CLI arguments
@param argc - number of arguments
@param argv - string array of arguments
@return return 0 if success otherwise the error code
*/
int set_config_from_args(int argc, char** argv);

#endif /* SERVER_CONFIG_H */
