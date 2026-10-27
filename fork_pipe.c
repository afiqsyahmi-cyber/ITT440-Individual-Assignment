#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define NUM_CHILDREN 3

/* Runs when the user presses Ctrl+C (SIGINT) */
static void on_sigint(int sig)
{
    (void)sig;
    const char msg[] = "\n[INTERRUPT] Ctrl+C detected. Shutting down.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main(void)
{
    int pipes[NUM_CHILDREN][2];
    char message[256];

    signal(SIGINT, on_sigint);

    /* 1. Create one pipe per child */
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(pipes[i]) == -1) { perror("pipe"); exit(1); }
    }

    /* 2. Fork the children */
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }

        if (pid == 0) {                       /* CHILD */
            signal(SIGINT, SIG_IGN);          /* only the parent reports Ctrl+C */
            for (int j = 0; j < NUM_CHILDREN; j++) {
                close(pipes[j][1]);           /* child never writes */
                if (j != i) close(pipes[j][0]);
            }
            char buf[256];
            ssize_t n = read(pipes[i][0], buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                printf("Child %d (PID %d) received: %s\n", i + 1, getpid(), buf);
            }
            close(pipes[i][0]);
            exit(0);
        }
    }

    /* 3. PARENT: get message from keyboard */
    printf("Parent: enter a message: ");
    if (fgets(message, sizeof(message), stdin) == NULL) return 1;
    message[strcspn(message, "\n")] = '\0';

    /* 4. Send to every child through its pipe */
    for (int i = 0; i < NUM_CHILDREN; i++) {
        close(pipes[i][0]);
        write(pipes[i][1], message, strlen(message) + 1);
        close(pipes[i][1]);
    }

    for (int i = 0; i < NUM_CHILDREN; i++) wait(NULL);
    printf("Parent: message delivered to all children.\n");
    return 0;
}
