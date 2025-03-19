#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static struct termios term_orig;

void term_deinit() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_orig);
}

void term_init() {
    static struct termios term_new;
    tcgetattr(STDIN_FILENO, &term_orig);
    term_new = term_orig;
    term_new.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    term_new.c_oflag &= ~(OPOST);
    term_new.c_cflag |= (CS8);
    term_new.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    term_new.c_cc[VMIN] = 0;
    term_new.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_new);
}

void run() {
    char c;
    while (1) {
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == 'q') {
                break;
            } else {
                printf("You pressed %c\n\r", c);
            }
        }
    }
}

void deinit() {
    term_deinit();
}

void init() {
    term_init();
}

int main() {
    init();
    run();
    deinit();
    return 0;
}