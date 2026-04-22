#define NAPOLEON_IMPLEMENTATION
#include "../napoleon.h"
#include <string.h>

extern void register_tests(void);

int main(int argc, char *argv[]){
    register_tests();

    if (argc > 1 && strcmp(argv[1], "--no-pass") == 0) {
        nap_run(.quiet = true);
    } else if (argc > 1 && strcmp(argv[1], "--no-capture") == 0) {
        nap_run(.dont_capture_output = true);
    } else {
        nap_run();
    }

    (void)argc;
    (void)argv;
    return 0;
}