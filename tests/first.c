#include "../main.h"

int main(int argc, char *argv[]) {
    int retval = run("First.class");

    if (retval == 42) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 42, retval);
        return 1;
    }
}
