#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

static struct termios term_orig;
static struct winsize term_size;
static char input_buffer[BUFFER_SIZE];
static char output_buffer[BUFFER_SIZE];

static uint32_t origin_x = UINT32_MAX / 2;
static uint32_t origin_y = UINT32_MAX / 2;
static uint32_t camera_x = UINT32_MAX / 2 - 3;
static uint32_t camera_y = UINT32_MAX / 2 - 1;

static int game_exit = 0;

void term_update() {
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &term_size);
}

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

void input_update() {
    int bytes_read = read(STDIN_FILENO, input_buffer, BUFFER_SIZE);
    input_buffer[bytes_read] = '\0';
    for (char* itr = input_buffer; *itr; ++itr) {
        if (*itr == 'q') {
            game_exit = 1;
        } else if (*itr == 'w') {
            camera_y += 1;
        } else if (*itr == 'a') {
            camera_x -= 1;
        } else if (*itr == 's') {
            camera_y -= 1;
        } else if (*itr == 'd') {
            camera_x += 1;
        }
    }
}

void output_update() {
    output_buffer[0] = '\0';
    strcat(output_buffer, "\033[2J\033[1;1H");
    for (uint32_t y = 0; y < term_size.ws_row; ++y) {
        for (uint32_t x = 0; x < term_size.ws_col; ++x) {
            uint32_t world_x = camera_x + x - term_size.ws_col / 2;
            uint32_t world_y = camera_y - y + term_size.ws_row / 2;
            if (world_x == origin_x && world_y == origin_y) {
                strcat(output_buffer, "O");
            } else if (world_x == camera_x && world_y == camera_y) {
                strcat(output_buffer, "+");
            } else {
                strcat(output_buffer, ".");
            }
        }
    }
    write(STDOUT_FILENO, output_buffer, strlen(output_buffer));
}

void run() {
    while (!game_exit) {
        term_update();
        input_update();
        output_update();
        usleep(1000 * 1000 / 60);
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