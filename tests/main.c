#include <stdio.h>

int test_server_constructor(void);
int test_listener(void);

int main() {
    int status;
    status = test_server_constructor();
    if (status) return status;

    printf("\n\n");
    status = test_listener();
    if (status) return status;

    return 0;
}
