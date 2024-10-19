#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <arpa/inet.h>

#ifndef VERSION
#define VERSION "0.0.0" // meant to be overwrite
#endif // VERSION

#ifndef DBUG_MODE
#define DBUG_MODE 0
#endif // DBUG_MODE

#define PROG_NAME "cerve"
#define HELP_MSG  \
"usage: cerve [OPTIONS] \n\n" \
\
"Options \n" \
"   -h --help       print help and exit \n"\
"   --version       print version and exit \n" \
"   -v --verbose    an integer between 1 and 4, inclusive of those values representing \n" \
"                   different log levels. higher values for detailed logs. 1 for minum logging \n" \
"                   default to 4 \n" \
"   -w --workers    number of workers to be used \n" \
"                   defaults to 4 \n" \
"   -p  --port      system port at which the server to listen for connections. \n"\
"                   defaults to 8000 \n"\
"   --disable-socket-reuse      if the flag is provided the SO_REUSEADDR will not be set. \n"\


extern int g_server_port;
extern int g_worker_count;

extern int g_enable_socket_reuse;   //  if set to 0 (default to 1), SO_REUSEADDR will not be enabled

/**
@brief configure values based on CLI arguments
@param argc - number of arguments
@param argv - string array of arguments
@return return 0 if success otherwise the error code
*/
int set_config_from_args(int argc, char** argv);

#endif /* SERVER_CONFIG_H */
