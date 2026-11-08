/* wasm: no processes */
#define fork() (-1)
#define waitpid(p, s, o) ((void)(p), (void)(s), -1)
#define WIFEXITED(s) 0
#define WEXITSTATUS(s) 1
#define _exit(c) __builtin_trap()
#define pipe(fd) (-1)
typedef int pid_t;
