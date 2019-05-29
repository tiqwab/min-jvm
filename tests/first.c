#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[1] = {"First.class"};
    int retval = run(classes, 1);

    if (retval == 42) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 42, retval);
        return 1;
    }
}
