#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qlearning.h"

#define NUM_EPISODES 100000000
#define ALPHA 0.1
#define GAMMA 0.95
#define EPSILON 1

static void print_usage(char *program_name);

int main(int argc, char *argv[]) {
    Type type;
    const char *filename = argv[3];

    if (argc != 5) {
        print_usage(argv[0]);
    }


    
    if (strcmp(argv[1], "tabular") == 0) {
        type = TABULAR;
    } else if (strcmp(argv[1], "linear") == 0) {
        type = LINEAR;
    } else if (strcmp(argv[1], "aggregated") == 0) {
        type = AGGREGATED;
    } else {
        print_usage(argv[0]);
    }
    

    uint8_t board_size = atoi(argv[2]);

    uint64_t num_episodes = strtoull(argv[4], NULL, 10) * 1000000;
    

    QAgent qagent = qagent_init(type, board_size);

    qagent_training(qagent, board_size, num_episodes, ALPHA, GAMMA, EPSILON);

    qagent_write(qagent, filename);

    qagent_free(qagent);
    return EXIT_SUCCESS;
}

static void print_usage(char *program_name) {
    fprintf(stderr, "Usage: %s {tabular | linear | aggregated} <board_size> <output_filename> <number of episodes in millions> \n", program_name);
    fprintf(stderr, "%s tabular 4 tabular_4x4_100m 100\n", program_name);
    exit(EXIT_FAILURE);
}
