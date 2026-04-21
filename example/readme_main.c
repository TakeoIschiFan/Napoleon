#define NAPOLEON_IMPLEMENTATION
#include "../napoleon.h"

extern void register_tests(void);

int main(int argc, char *argv[]){
    register_tests();
    int result = nap_run();
    (void)argc;
    (void)argv;
    return result;
}