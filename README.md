# Napoleon

Napoleon is a minimal single-header-style testing library for C. There are a bunch of these, but this is mine.

## Usage

Write tests as regular c functions. Register tests with `nap_add()`.

```c
#include "napoleon.h"

void my_test(void){
    nap_assert(true);
}

void register_my_tests(){
    nap_add(my_test)
}
```

Then add an entrypoint and call `nap_run()` to run all tests.

```c
#define NAPOLEON_IMPLEMENTATION // define implementation flag once
#include "napoleon.h"

int main(void){
    register_my_tests();
    int result = nap_run();
    return result;
}
```

#### Assert functions

Napoleon has a few helper assert functions you can use

```c
void my_test_asserts(void){
    // assert strings
    nap_assert_str("my string should equal", "my string should equal");

    // assert numerics
    nap_assert_num(24, 24);
    // with optional tolerance (0 if not specified)
    nap_assert_num(1.0f / 3.0f, 0.333f, .tolerance = 0.01f);

    // assert memory blocks
    int a[4] = {1, 2, 3, 4};
    int b[4] = {1, 2, 3, 5};
    nap_assert_mem(a, b, sizeof(a));
}
```

#### Organizing tests into suites

By default, tests are organised into suites based on their filename. You can change this by passing the optional `suite` argument to `nap_add()`

```c
void my_test(void){
    nap_assert(true);
}

void register_my_tests(){
    nap_add(my_test, .suite = "secondary suite")
}
```

and specifying a suite to run in `nap_run()`

```c
int main(void){
    register_my_tests();
    int result = nap_run(.suite = "secondary suite");
    return result;
}
```

#### Skipping tests

Skip tests by passing the `skip_reason` argument to `nap_add()`

```c
void my_test(void){
    nap_assert(true);
}

void register_my_tests(){
    nap_add(my_test, .skip_reason = "not implemented yet")
}
```

#### Timeouts

By default, tests time out after ten seconds. You can change this by passing the optional `timeout` argument to `nap_run()`.

```c
#include <unistd>

void sleep_test(void){
    sleep(10)
    nap_assert(true);
}

void register_my_tests(){
    nap_add(sleep_test)
}

int main(void){
    register_my_tests();
    int result = nap_run(.timeout = 5); // will FAIL!
    return result;
}
```

## License

MIT.
