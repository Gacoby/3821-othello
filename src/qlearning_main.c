#include "qlearning.h"

#define NUM_EPISODES 100000000
#define ALPHA 0.1
#define GAMMA 0.95
#define EPSILON 1

int main(void) {
    QAgent qagent = qagent_init(TABULAR);

    qagent_training(qagent, 4, NUM_EPISODES, ALPHA, GAMMA, EPSILON);

    qagent_write(qagent, "tabular4x4");

    qagent_free(qagent);
}