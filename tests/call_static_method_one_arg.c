#include "../main.h"

int main(int argc, char *argv[]) {
    int retval = run("CallStaticMethodOneArg.class");

    if (retval == 43) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 43, retval);
        return 1;
    }
}
