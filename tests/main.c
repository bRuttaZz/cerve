#include <stdio.h>

void test_server_constructor(void);
void test_listener(void);

int main() {
    test_server_constructor();
    printf("\n\n");
    test_listener();

    return 0;
}
