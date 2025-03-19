#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define WORLD_WIDTH 2048
#define CHUNK_WIDTH 16

enum blocktype {
    blocktype_air,
    blocktype_matter,
    blocktype_matter_ore,
    blocktype_inserter_north,
    blocktype_inserter_east,
    blocktype_inserter_south,
    blocktype_inserter_west,
};

struct block {
    enum blocktype type;
    uint32_t count;
};

struct block_table {
    enum blocktype type;
    enum blocktype rotate_next;
    enum blocktype rotate_prev;
    const char* name;
    const char* symbol;
};

struct chunk {
    struct block blocks[CHUNK_WIDTH][CHUNK_WIDTH];
};

static struct termios term_orig;
static struct winsize term_size;

static char input_buffer[BUFFER_SIZE];
static char output_buffer[BUFFER_SIZE];

static struct chunk world[WORLD_WIDTH / CHUNK_WIDTH][WORLD_WIDTH / CHUNK_WIDTH];
static struct block_task {
    struct block block;
    uint32_t x;
    uint32_t y;
} block_tasks[BUFFER_SIZE];
static uint32_t block_task_size = 0;

static int game_exit = 0;
static uint32_t origin_x = WORLD_WIDTH / 2;
static uint32_t origin_y = WORLD_WIDTH / 2;
static uint32_t camera_x = WORLD_WIDTH / 2 - 3;
static uint32_t camera_y = WORLD_WIDTH / 2 - 1;

struct block_table block_table[] = {
    [blocktype_air] = {
        .type = blocktype_air,
        .rotate_next = blocktype_air,
        .rotate_prev = blocktype_air,
        .name = "Air",
        .symbol = " ",
    },
    [blocktype_matter] = {
        .type = blocktype_matter,
        .rotate_next = blocktype_matter,
        .rotate_prev = blocktype_matter,
        .name = "Matter",
        .symbol = "M",
    },
    [blocktype_matter_ore] = {
        .type = blocktype_matter_ore,
        .rotate_next = blocktype_matter_ore,
        .rotate_prev = blocktype_matter_ore,
        .name = "Matter Ore",
        .symbol = "O",
    },
    [blocktype_inserter_north] = {
        .type = blocktype_inserter_north,
        .rotate_next = blocktype_inserter_east,
        .rotate_prev = blocktype_inserter_west,
        .name = "Inserter North",
        .symbol = "↑",
    },
    [blocktype_inserter_east] = {
        .type = blocktype_inserter_east,
        .rotate_next = blocktype_inserter_south,
        .rotate_prev = blocktype_inserter_north,
        .name = "Inserter East",
        .symbol = "→",
    },
    [blocktype_inserter_south] = {
        .type = blocktype_inserter_south,
        .rotate_next = blocktype_inserter_west,
        .rotate_prev = blocktype_inserter_east,
        .name = "Inserter South",
        .symbol = "↓",
    },
    [blocktype_inserter_west] = {
        .type = blocktype_inserter_west,
        .rotate_next = blocktype_inserter_north,
        .rotate_prev = blocktype_inserter_south,
        .name = "Inserter West",
        .symbol = "←",
    },
};

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

struct block block_make(enum blocktype type, uint32_t count) {
    return (struct block){
        .type = type,
        .count = count,
    };
}

struct block* world_get(uint32_t x, uint32_t y) {
    return &world[y / CHUNK_WIDTH][x / CHUNK_WIDTH].blocks[y % CHUNK_WIDTH][x % CHUNK_WIDTH];
}

void world_update_block(uint32_t x, uint32_t y) {
}

void world_update() {
    block_task_size = 0;
    for (uint32_t chunk_y = 0; chunk_y < WORLD_WIDTH / CHUNK_WIDTH; ++chunk_y) {
        for (uint32_t chunk_x = 0; chunk_x < WORLD_WIDTH / CHUNK_WIDTH; ++chunk_x) {
            for (uint32_t y = 0; y < CHUNK_WIDTH; ++y) {
                for (uint32_t x = 0; x < CHUNK_WIDTH; ++x) {
                    world_update_block(chunk_x * CHUNK_WIDTH + x, chunk_y * CHUNK_WIDTH + y);
                }
            }
        }
    }
    for (uint32_t i = 0; i < block_task_size; ++i) {
        struct block_task* block_task = &block_tasks[i];
        struct block* block = world_get(block_task->x, block_task->y);
        *block = block_task->block;
    }
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
        } else if (*itr == 'r') {
            struct block* block = world_get(camera_x, camera_y);
            *block = block_make(block_table[block->type].rotate_next, 1);
        } else if (*itr == '1') {
            struct block* block = world_get(camera_x, camera_y);
            *block = block_make(blocktype_matter, 1);
        } else if (*itr == '2') {
            struct block* block = world_get(camera_x, camera_y);
            *block = block_make(blocktype_matter_ore, 1);
        } else if (*itr == '3') {
            struct block* block = world_get(camera_x, camera_y);
            *block = block_make(blocktype_inserter_north, 1);
        }
    }
}

void output_update() {
    strcpy(output_buffer, "\033[2J\033[1;1H");

    for (uint32_t y = 0; y < term_size.ws_row; ++y) {
        for (uint32_t x = 0; x < term_size.ws_col; ++x) {
            uint32_t world_x = camera_x + x - term_size.ws_col / 2;
            uint32_t world_y = camera_y - y + term_size.ws_row / 2;
            struct block* block = world_get(world_x, world_y);
            if (world_x == camera_x && world_y == camera_y) {
                strcat(output_buffer, "+");
            } else {
                strcat(output_buffer, block_table[block->type].symbol);
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
        world_update();
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