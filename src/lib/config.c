#include "../../include/server-configs.h"
#include "../../include/utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

char *g_custom_resp_header_file_path = "";
char g_custom_serve_directory[PATH_MAX+1] = ".";

int _check_if_argument_available(int index, int argc, char *arg) {
    if (index++ >= argc -1 ) {
        fprintf(stderr, "value not provided for argument %s\n", arg);
        return 126;  // Required key not available
    }
    return 0;
}

/**
@brief configure values based on CLI arguments
@param argc - number of arguments
@param argv - string array of arguments
@return return 0 if success otherwise the error code (negative for success exits)
*/
int set_config_from_args(int argc, char** argv) {
    int i=1;
    while (i<argc) {
        if (strcmp(argv[i], "-h")==0 || strcmp(argv[i], "--help") == 0) {
            printf("%s v%s\n\n", PROG_NAME, VERSION);
            printf(HELP_MSG);
            return -1;

        }else if (strcmp(argv[i], "--version") == 0){
            printf("%s v%s\n", PROG_NAME, VERSION);
            return -1;

        }else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (_check_if_argument_available(i, argc, argv[i])) return 126;
            i++;
            int _port = atoi(argv[i]);
            if (!_port) {
                fprintf(stderr, "invalid port number provided\n");
                return 22;   // Invalid argument
            }
            g_server_port = _port;

        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--workers") == 0) {
            if (_check_if_argument_available(i, argc, argv[i])) return 126;
            i++;
            int _worker = atoi(argv[i]);
            if (!_worker) {
                fprintf(stderr, "invalid value for workers. Expected a positive integer\n");
                return 22;   // Invalid argument
            }
            g_worker_count = _worker;

        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            if (_check_if_argument_available(i, argc, argv[i])) return 126;
            i++;
            int _log_level = atoi(argv[i]);
            if (!_log_level || _log_level > 4 || _log_level < 1) {
                fprintf(stderr, "invalid value for log level. Expected an integer from 1 to 4\n");
                return 22;   // Invalid argument
            }
            g_logger.level = _log_level - 1;

        } else if (strcmp(argv[i], "--disable-socket-reuse") == 0) {
            g_enable_socket_reuse = 0;

        } else if (strcmp(argv[i], "--res-headers") == 0) {
            if (_check_if_argument_available(i, argc, argv[i])) return 126;
            i++;
            g_custom_resp_header_file_path = argv[i];
            if (access(g_custom_resp_header_file_path, R_OK) != 0 ) {
                fprintf(stderr, "cannot access provided header file ! : %s\n", g_custom_resp_header_file_path);
                return 2;
            }

        } else if (strcmp(argv[i], "--serve-dir") == 0) {
            if (_check_if_argument_available(i, argc, argv[i])) return 126;
            i++;
            if (argv[i][strlen(argv[i])-1] == '/') {
                argv[i][strlen(argv[i])-1] = '\0';
            }
            struct stat st;
            if (stat(argv[i], &st) != 0 || !S_ISDIR(st.st_mode)) {
                fprintf(stderr, "cannot access provided serve directory! : %s \n", argv[i]);
                return 2;
            }
            char *ptr;
            ptr = realpath(argv[i], g_custom_serve_directory);
            if (!ptr) {
                fprintf(stderr, "error resolving provided serve directory! : %s\n", argv[i]);
            }

        } else {
            // not supported
            fprintf(stderr, "invalid argument : %s\n\n", argv[i]);
            printf(HELP_MSG);
            return 22;
        }
        i++;
    }
    return 0;
}
