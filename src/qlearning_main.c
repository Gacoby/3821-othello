#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qlearning.h"

#define ALPHA 0.1
#define GAMMA 0.95
#define EPSILON 1


static void print_usage(char *program_name);

int main(int argc, char *argv[]) {
    if (argc != 8) {
        print_usage(argv[0]);
    }

    char *program_str = argv[0];
    char *type_str = argv[1];
    char *board_size_str = argv[2];
    char *num_episodes_str = argv[3];
    char *alpha_str = argv[4];
    char *gamma_str = argv[5];
    char *epsilon_min_str = argv[6];
    char *output_filename_str = argv[7];

    Type type;
    if (strcmp(type_str, "tabular") == 0) {
        type = TABULAR;
    } else if (strcmp(type_str, "linear") == 0) {
        type = LINEAR;
    } else if (strcmp(type_str, "aggregated") == 0) {
        type = AGGREGATED;
    } else {
        print_usage(program_str);
    }
    

    uint8_t board_size = atoi(board_size_str);

    uint64_t num_episodes = strtoull(num_episodes_str, NULL, 10);
    
    double alpha = strtod(alpha_str, NULL);

    double gamma = strtod(gamma_str, NULL);

    double epsilon_min = strtod(epsilon_min_str, NULL);

    QAgent qagent = qagent_init(type, board_size);

    qagent_training(qagent, board_size, num_episodes, alpha, gamma, epsilon_min);

    qagent_write(qagent, output_filename_str);

    qagent_free(qagent);
    return EXIT_SUCCESS;
}

static void print_usage(char *program_name) {
    fprintf(
        stderr, 
        "Usage: %s {tabular | linear | aggregated} <board_size> <number of episodes> "
        "<alpha> <gamma> <epsilon_min> <output_filename> \n", 
        program_name
    );
    fprintf(stderr, "%s tabular 4 tabular_4x4_100m 100\n", program_name);
    exit(EXIT_FAILURE);
}
