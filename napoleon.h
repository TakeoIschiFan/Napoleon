#pragma once

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Public API */
// nap_add()
typedef struct nap_add_optionals {
    const char* suite;
    const char* skip_reason;
    int timeout;
    bool should_fail;
} nap_add_optionals;
#define nap_add(test_fn, ...)                                                                                          \
    do {                                                                                                               \
        static const nap_add_optionals _nap_tp = {__VA_ARGS__};                                                        \
        _nap_add((test_fn), _nap_str_cat(test_fn), _nap_str_cat(__FILE__), _nap_tp);                                       \
    } while (0)
void _nap_add(void (*test)(void), const char* func_name, const char* file_name, nap_add_optionals params);

// nap_run()
typedef struct nap_run_optionals {
    int default_timeout;
    bool quiet;
    bool dont_capture_output;
    const char* suite;
} nap_run_optionals;
#define nap_run(...) _nap_run((nap_run_optionals){__VA_ARGS__})
int _nap_run(nap_run_optionals options);

/* asserts */
#define nap_assert(condition) _nap_assert((condition), #condition, __FILE__, __LINE__)
void _nap_assert(bool condition, const char* expr, const char* file, int line);

#define nap_assert_str(s1, s2) _nap_assert_str((s1), (s2), __FILE__, __LINE__)
void _nap_assert_str(const char* s1, const char* s2, const char* file, int line);

typedef struct nap_assert_num_optionals {
    double tolerance; /* Default: 0.0 */
} nap_assert_num_optionals;
#define nap_assert_num(n1, n2, ...)                                                                                    \
    do {                                                                                                               \
        static const nap_assert_num_optionals _nap_tp = {__VA_ARGS__};                                                 \
        _nap_assert_num((n1), (n2), _nap_tp, __FILE__, __LINE__);                                                      \
    } while (0)
void _nap_assert_num(double n1, double n2, nap_assert_num_optionals params, const char* file, int line);

#define nap_assert_mem(p1, p2, size) _nap_assert_mem((p1), (p2), (size), __FILE__, __LINE__)
void _nap_assert_mem(const void* p1, const void* p2, size_t size, const char* file, int line);

#define _nap_str(x) #x
#define _nap_str_cat(x) _nap_str(x)

/* Implementation */
#ifdef NAPOLEON_IMPLEMENTATION

#undef NAPOLEON_IMPLEMENTATION

/* ── Colors ── */
#define NAP_COLOR_RESET "\x1b[0m"
#define NAP_COLOR_RED "\x1b[31m"
#define NAP_COLOR_GREEN "\x1b[32m"
#define NAP_COLOR_YELLOW "\x1b[33m"
#define NAP_COLOR_BLUE "\x1b[34m"
#define NAP_COLOR_CYAN "\x1b[36m"
#define NAP_COLOR_BOLD "\x1b[1m"

/* ── Utils ── */
#define NAP_FILE_BASENAME(file) (strrchr((file), '/') ? strrchr((file), '/') + 1 : (file))

static const char* _nap_basename_strip_ext(const char* file_name) {
    static char buf[256];
    const char* base = NAP_FILE_BASENAME(file_name);
    int len = (int)strlen(base);
    if (len > 0 && base[len - 1] == '"') {
        len--;
    }
    if (len > 0 && base[0] == '"') {
        base++;
        len--;
    }
    snprintf(buf, sizeof(buf), "%.*s", len, base);
    char* ext = strrchr(buf, '.');
    if (ext) {
        *ext = '\0';
    }
    return buf;
}

static long long _nap_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* ── Types ── */
typedef enum {
    NAP_RESULT_PASS = 0,
    NAP_RESULT_FAIL,
    NAP_RESULT_FAIL_ASSERT,
    NAP_RESULT_FAIL_STRING,
    NAP_RESULT_FAIL_NUMBER,
    NAP_RESULT_FAIL_MEMORY,
    NAP_RESULT_FAIL_TIMEOUT,
    NAP_RESULT_ERROR,
    NAP_RESULT_SKIP
} _nap_result;

typedef struct nap_test {
    void (*fn)(void);
    const char* suite;
    const char* skip_reason;
    const char* func_name;
    int timeout;
    bool should_fail;
} _nap_test;

typedef struct nap_test_result {
    _nap_result result;
    int exit_code;
    int signal;
    long long duration_ms;
    char captured_output[1024];
    char captured_stderr[1024];
    char failure_expr[256];
    char failure_expected[256];
    char failure_got[256];
    char failure_file[256];
    int failure_line;
    int timeout;
} _nap_test_result;

static _nap_test* _nap_tests = NULL;
static int _nap_test_count = 0;
static int _nap_test_capacity = 0;
static bool _nap_should_capture_output = true;

static int _nap_failure_pipe[2] = {-1, -1};

/* ── Pipe functions ── */
static void _nap_write_failure_to_pipe(_nap_result type, const char* expr, const char* expected, const char* got,
                                       const char* file, int line) {
    if (_nap_failure_pipe[1] < 0) {
        _exit(1);
        return;
    }

    char buf[4096];
    int offset;
    switch (type) {
        case NAP_RESULT_FAIL_ASSERT:
            offset = snprintf(buf, sizeof(buf), "%d|%s|%s|%s|%s|%d|", (int)type, file, expr ? expr : "", "-", "", line);
            break;
        case NAP_RESULT_FAIL_STRING:
        case NAP_RESULT_FAIL_NUMBER:
            offset = snprintf(buf, sizeof(buf), "%d|%s|%s|%s|%s|%d|", (int)type, file, expr ? expr : "-",
                              expected ? expected : "", got ? got : "", line);
            break;
        case NAP_RESULT_FAIL_MEMORY:
            offset = snprintf(buf, sizeof(buf), "%d|memory compare failed|%s|%d|", (int)type, file, line);
            break;
        case NAP_RESULT_FAIL_TIMEOUT:
            offset = snprintf(buf, sizeof(buf), "%d|timeout exceeded|%s|%d|", (int)type, file, line);
            break;
        default:
            return;
    }

    ssize_t written = write(_nap_failure_pipe[1], buf, offset);
    (void)written;
    _exit(1);
}

static bool _nap_read_failure_from_pipe(_nap_test_result* result) {
    if (_nap_failure_pipe[0] < 0)
        return false;

    fd_set fds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    FD_ZERO(&fds);
    FD_SET(_nap_failure_pipe[0], &fds);

    int ret = select(_nap_failure_pipe[0] + 1, &fds, NULL, NULL, &tv);
    if (ret <= 0)
        return false;

    char buf[4096];
    ssize_t n = read(_nap_failure_pipe[0], buf, sizeof(buf) - 1);
    if (n <= 0)
        return false;
    buf[n] = '\0';

    char* saveptr;
    char* type_str = strtok_r(buf, "|", &saveptr);
    char* file_str = strtok_r(NULL, "|", &saveptr);
    char* expr = strtok_r(NULL, "|", &saveptr);
    char* expected = strtok_r(NULL, "|", &saveptr);
    char* line_str = strtok_r(NULL, "|", &saveptr);
    char* got = strtok_r(NULL, "|", &saveptr);

    if (!type_str || !file_str)
        return false;

    result->result = (_nap_result)atoi(type_str);
    snprintf(result->failure_file, sizeof(result->failure_file), "%s", file_str ? file_str : "");
    if (line_str) {
        result->failure_line = atoi(line_str);
    }

    switch (result->result) {
        case NAP_RESULT_FAIL_ASSERT:
            snprintf(result->failure_expr, sizeof(result->failure_expr), "%s", expr ? expr : "");
            break;
        case NAP_RESULT_FAIL_STRING:
        case NAP_RESULT_FAIL_NUMBER:
            snprintf(result->failure_expr, sizeof(result->failure_expr), "%s", expr ? expr : "");
            snprintf(result->failure_expected, sizeof(result->failure_expected), "%s", expected ? expected : "");
            snprintf(result->failure_got, sizeof(result->failure_got), "%s", got ? got : "");
            break;
        case NAP_RESULT_FAIL_MEMORY:
            snprintf(result->failure_expr, sizeof(result->failure_expr), "memory compare failed");
            break;
        case NAP_RESULT_FAIL_TIMEOUT:
            snprintf(result->failure_expr, sizeof(result->failure_expr), "timeout");
            break;
        default:
            return true;
    }

    return true;
}

static void _nap_capture_output(int pipe_fd, char* buf, size_t buf_size, int timeout_sec) {
    if (pipe_fd < 0)
        return;

    if (timeout_sec > 0) {
        struct timeval tv;
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;
        setsockopt(pipe_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    ssize_t n = read(pipe_fd, buf, buf_size - 1);
    if (n > 0) {
        buf[n] = '\0';
    } else {
        buf[0] = '\0';
    }

    close(pipe_fd);
}

/* ── Assert functions ── */
void _nap_assert(bool condition, const char* expr, const char* file, int line) {
    if (condition)
        return;
    _nap_write_failure_to_pipe(NAP_RESULT_FAIL_ASSERT, expr, NULL, NULL, file, line);
    _exit(1);
}

void _nap_assert_str(const char* s1, const char* s2, const char* file, int line) {
    if (s1 == s2)
        return;
    if (s1 && s2 && strcmp(s1, s2) == 0)
        return;

    _nap_write_failure_to_pipe(NAP_RESULT_FAIL_STRING, NULL, s1 ? s1 : "(null)", s2 ? s2 : "(null)", file, line);
    _exit(1);
}

void _nap_assert_num(double n1, double n2, nap_assert_num_optionals params, const char* file, int line) {
    double tolerance = params.tolerance;
    double diff = n1 - n2;
    if (diff < 0)
        diff = -diff;

    if (diff <= tolerance)
        return;

    char buf1[64], buf2[64];
    snprintf(buf1, sizeof(buf1), "%.6g", n1);
    snprintf(buf2, sizeof(buf2), "%.6g", n2);

    char tol_buf[64];
    snprintf(tol_buf, sizeof(tol_buf), "%.6g", tolerance);

    char expr[256];
    snprintf(expr, sizeof(expr), "%s ~= %s (tolerance: %s)", buf1, buf2, tol_buf);

    _nap_write_failure_to_pipe(NAP_RESULT_FAIL_NUMBER, expr, buf2, buf1, file, line);
    _exit(1);
}

void _nap_assert_mem(const void* p1, const void* p2, size_t size, const char* file, int line) {
    if (p1 == p2)
        return;
    if (memcmp(p1, p2, size) == 0)
        return;

    _nap_write_failure_to_pipe(NAP_RESULT_FAIL_MEMORY, NULL, NULL, NULL, file, line);
    _exit(1);
}

/* ── Print functions ── */
static void _nap_print_result_detail(_nap_test_result* result) {
    if (result->result > NAP_RESULT_FAIL_TIMEOUT)
        return;

    if (result->failure_file[0]) {
        fprintf(stdout, "    %s:%d\n", result->failure_file, result->failure_line);
    }

    switch (result->result) {
        case NAP_RESULT_FAIL:
        case NAP_RESULT_FAIL_ASSERT:
            if (result->failure_expr[0]) {
                fprintf(stdout, "    %s\n", result->failure_expr);
            }
            break;
        case NAP_RESULT_FAIL_STRING:
            if (result->failure_expected[0] || result->failure_got[0]) {
                fprintf(stdout, "    got:      \"%s\"\n", result->failure_got);
                fprintf(stdout, "    expected: \"%s\"\n", result->failure_expected);
            }
            break;
        case NAP_RESULT_FAIL_NUMBER:
            if (result->failure_expected[0] || result->failure_got[0]) {
                fprintf(stdout, "    %s\n", result->failure_expr);
                fprintf(stdout, "    got:      %s\n", result->failure_got);
                fprintf(stdout, "    expected: %s\n", result->failure_expected);
            }
            break;
        case NAP_RESULT_FAIL_MEMORY:
            fputs("    memory regions differ\n", stdout);
            break;
        case NAP_RESULT_FAIL_TIMEOUT:
            fprintf(stdout, "    test exceeded timeout (%d s)\n", result->timeout);
            break;
        default:
            return;
    }

    if (result->captured_output[0]) {
        fputs("    stdout:\n", stdout);
        const char* start = result->captured_output;
        while (*start) {
            const char* newline = strchr(start, '\n');
            if (!newline) {
                fprintf(stdout, "      %s\n", start);
                break;
            }
            fprintf(stdout, "      %.*s\n", (int)(newline - start), start);
            start = newline + 1;
        }
    }

    if (result->captured_stderr[0]) {
        fputs("    stderr:\n", stdout);
        const char* start = result->captured_stderr;
        while (*start) {
            const char* newline = strchr(start, '\n');
            if (!newline) {
                fprintf(stdout, "      %s\n", start);
                break;
            }
            fprintf(stdout, "      %.*s\n", (int)(newline - start), start);
            start = newline + 1;
        }
    }
}

static void _nap_print_summary(int passed, int failed, int skipped, int errors, long long total_duration_ms) {
    fputs(NAP_COLOR_BOLD "\n[summary]" NAP_COLOR_RESET, stdout);

    if (failed > 0)
        fputs(NAP_COLOR_RED, stdout);
    fprintf(stdout, "  failed:  %d\n", failed);
    if (failed > 0)
        fputs(NAP_COLOR_RESET, stdout);

    if (passed > 0)
        fputs(NAP_COLOR_GREEN, stdout);
    fprintf(stdout, "  passed:  %d\n", passed);
    if (passed > 0)
        fputs(NAP_COLOR_RESET, stdout);

    if (errors > 0) {
        fputs(NAP_COLOR_RED, stdout);
    }
    fprintf(stdout, "  errors:  %d\n", errors);
    if (errors > 0)
        fputs(NAP_COLOR_RESET, stdout);

    if (skipped > 0) {
        fputs(NAP_COLOR_YELLOW, stdout);
    }
    fprintf(stdout, "  skipped: %d\n", skipped);
    if (skipped > 0)
        fputs(NAP_COLOR_RESET, stdout);

    fprintf(stdout, "  total:   %d\n", passed + failed + skipped + errors);
    fprintf(stdout, "  duration: %lld.%03dms\n", (long long)(total_duration_ms / 1000),
            (int)(total_duration_ms % 1000));
    fputs(NAP_COLOR_RESET, stdout);
}

/* ── Test execution ── */
static void _nap_run_in_child(_nap_test* test, int pipe_stdout[2], int pipe_stderr[2], bool capture_output) {
    if (capture_output) {
        dup2(pipe_stdout[1], STDOUT_FILENO);
        dup2(pipe_stderr[1], STDERR_FILENO);
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);
    } else {
        close(pipe_stdout[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[0]);
        close(pipe_stderr[1]);
    }

    close(_nap_failure_pipe[0]);

    test->fn();

    _exit(0);
}

static _nap_test_result _nap_run_test(_nap_test* test, int global_timeout) {
    _nap_test_result result = {0};
    result.result = NAP_RESULT_PASS;

    bool capture_output = _nap_should_capture_output;
    int timeout = test->timeout > 0 ? test->timeout : global_timeout;

    int pipe_stdout[2] = {-1, -1};
    int pipe_stderr[2] = {-1, -1};

    if (capture_output) {
        if (pipe(pipe_stdout) != 0 || pipe(pipe_stderr) != 0) {
            result.result = NAP_RESULT_ERROR;
            snprintf(result.failure_expr, sizeof(result.failure_expr), "Failed to create pipe");
            return result;
        }
    }

    if (pipe(_nap_failure_pipe) != 0) {
        result.result = NAP_RESULT_ERROR;
        snprintf(result.failure_expr, sizeof(result.failure_expr), "Failed to create failure pipe");
        return result;
    }

    long long start_ms = _nap_now_ms();

    pid_t pid = fork();
    if (pid < 0) {
        result.result = NAP_RESULT_ERROR;
        snprintf(result.failure_expr, sizeof(result.failure_expr), "Failed to fork: %s", strerror(errno));
        return result;
    }

    if (pid == 0) {
        _nap_run_in_child(test, pipe_stdout, pipe_stderr, capture_output);
    }

    if (pipe_stdout[1] >= 0)
        close(pipe_stdout[1]);
    if (pipe_stderr[1] >= 0)
        close(pipe_stderr[1]);

    int status = 0;
    int wait_count = 0;
    while (1) {
        pid_t ret = waitpid(pid, &status, WNOHANG);
        if (ret != 0)
            break;
        wait_count++;
        if (timeout > 0 && wait_count >= timeout * 100) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            status = 0;
            break;
        }
        usleep(10000);
    }

    int timeout_val = test->timeout > 0 ? test->timeout : global_timeout;
    result.timeout = timeout_val;

    if (timeout > 0 && wait_count >= timeout_val * 100) {
        result.result = NAP_RESULT_FAIL_TIMEOUT;
    }

    long long end_ms = _nap_now_ms();
    result.duration_ms = end_ms - start_ms;

    _nap_read_failure_from_pipe(&result);

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        result.signal = sig;
        result.result = NAP_RESULT_FAIL;
        snprintf(result.failure_expr, sizeof(result.failure_expr), "Test killed by signal %d", sig);
    }

    if (result.result == NAP_RESULT_PASS && WIFEXITED(status) && result.exit_code != 0) {
        result.result = NAP_RESULT_FAIL;
    }

    close(_nap_failure_pipe[0]);
    _nap_failure_pipe[0] = -1;
    _nap_failure_pipe[1] = -1;

    if (capture_output) {
        _nap_capture_output(pipe_stdout[0], result.captured_output, sizeof(result.captured_output), timeout);
        _nap_capture_output(pipe_stderr[0], result.captured_stderr, sizeof(result.captured_stderr), timeout);
    }

    return result;
}

/* ── API functions ── */
void _nap_add(void (*test)(void), const char* func_name, const char* file_name, nap_add_optionals params) {
    if (_nap_test_capacity == 0 || _nap_test_count >= _nap_test_capacity) {
        int new_capacity = _nap_test_capacity == 0 ? 64 : _nap_test_capacity + 64;
        _nap_test* new_tests = (_nap_test*)realloc(_nap_tests, new_capacity * sizeof(_nap_test));
        if (!new_tests) {
            fprintf(stderr, "Napoleon: failed to allocate memory for tests\n");
            return;
        }
        _nap_tests = new_tests;
        _nap_test_capacity = new_capacity;
    }

    _nap_test* t = &_nap_tests[_nap_test_count];

    t->fn = test;
    t->suite = params.suite;
    t->skip_reason = params.skip_reason;
    t->func_name = func_name;
    t->timeout = params.timeout;
    t->should_fail = params.should_fail;

    if (!t->suite) {
        t->suite = strdup(_nap_basename_strip_ext(file_name));
    }

    _nap_test_count++;
}

int _nap_run(nap_run_optionals options) {
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    int errors = 0;
    int default_timeout = options.default_timeout;
    _nap_should_capture_output = !options.dont_capture_output;
    long long total_start = _nap_now_ms();

    fputs("\n[tests]\n", stdout);

    const char* filter_suite = options.suite;

    for (int i = 0; i < _nap_test_count; i++) {
        _nap_test* t = &_nap_tests[i];

        if (!t->func_name) {
            t->func_name = "unknown";
        }
    }

    const char* current_suite = NULL;

    for (int i = 0; i < _nap_test_count; i++) {
        _nap_test* t = &_nap_tests[i];

        if (filter_suite && strcmp(t->suite, filter_suite) != 0) {
            continue;
        }

        if (!current_suite || strcmp(t->suite, current_suite) != 0) {
            current_suite = t->suite;
            fprintf(stdout, "\n" NAP_COLOR_BOLD "[%s]" NAP_COLOR_RESET "\n", current_suite);
        }

        if (t->skip_reason) {
            skipped++;
            fputs("  " NAP_COLOR_YELLOW "SKIP" NAP_COLOR_RESET, stdout);
            fprintf(stdout, " %s %s\n", t->func_name, t->skip_reason);
            continue;
        }

        _nap_test_result result = _nap_run_test(t, default_timeout);

        if (result.result == NAP_RESULT_PASS) {
            if (t->should_fail) {
                failed++;
                fputs("  " NAP_COLOR_RED "FAIL" NAP_COLOR_RESET, stdout);
                fprintf(stdout, " %s (expected to fail but passed)\n", t->func_name);
            } else {
                passed++;
                if (!options.quiet) {
                    fputs("  " NAP_COLOR_GREEN "PASS" NAP_COLOR_RESET, stdout);
                    fprintf(stdout, " %s\n", t->func_name);
                }
            }
        } else if (result.result > NAP_RESULT_PASS) {
            if (t->should_fail) {
                passed++;
                if (!options.quiet) {
                    fputs("  " NAP_COLOR_GREEN "PASS" NAP_COLOR_RESET, stdout);
                    fprintf(stdout, " %s (expected to fail)\n", t->func_name);
                }
            } else {
                failed++;
                fputs("  " NAP_COLOR_RED "FAIL" NAP_COLOR_RESET, stdout);
                fprintf(stdout, " %s (%.3fs)\n", t->func_name, result.duration_ms / 1000.0);
                _nap_print_result_detail(&result);
            }
        } else if (result.result == NAP_RESULT_ERROR) {
            errors++;
            fputs("  " NAP_COLOR_RED "ERROR" NAP_COLOR_RESET, stdout);
            fprintf(stdout, " %s", t->func_name);
            if (result.failure_expr[0]) {
                fprintf(stdout, ": %s", result.failure_expr);
            }
            fprintf(stdout, "\n");
        }
    }

    long long total_duration = _nap_now_ms() - total_start;

    _nap_print_summary(passed, failed, skipped, errors, total_duration);

    free(_nap_tests);
    _nap_tests = NULL;
    _nap_test_count = 0;
    _nap_test_capacity = 0;

    if (failed > 0)
        return 1;
    if (errors > 0)
        return 2;
    return 0;
}

#endif /* NAPOLEON_IMPLEMENTATION */
