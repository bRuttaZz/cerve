#include "../include/server.h"
#include "../include/server-configs.h"

int main(int argc, char **argv) {
    int _state = set_config_from_args(argc, argv);
    if (_state>0) {
        return _state;
    } else if (_state<0) {
        return 0;
    }
	return start_listener();
}
