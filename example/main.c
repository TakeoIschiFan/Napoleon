#define NAPOLEON_IMPLEMENTATION
#include "../napoleon.h"

extern void nap_register_tests(void);

int main(void) {
    nap_register_tests();
    return nap_run();
}
