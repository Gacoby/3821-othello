#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qlearning.h"

#define NUM_EPISODES 100000000
#define ALPHA 0.1
#define GAMMA 0.95
#define EPSILON 1

int main(int argc, char *argv[]) {
    Type type = TABULAR;
    const char *filename = "tabular4x4";

    if (argc >= 2) {
        if (strcmp(argv[1], "tabular") == 0) {
            type = TABULAR;
            filename = "tabular4x4";
        } else if (strcmp(argv[1], "linear") == 0) {
            type = LINEAR;
            filename = "linear4x4";
        } else if (strcmp(argv[1], "aggregated") == 0) {
            type = AGGREGATED;
            filename = "aggregated4x4";
        } else {
            fprintf(stderr, "Usage: %s [tabular|linear] [output file in data/]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (argc == 3) {
        filename = argv[2];
    }

    QAgent qagent = qagent_init(type);

    qagent_training(qagent, 4, NUM_EPISODES, ALPHA, GAMMA, EPSILON);

    qagent_write(qagent, filename);

    qagent_free(qagent);
    return EXIT_SUCCESS;
}
