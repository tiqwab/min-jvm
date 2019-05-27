#include "../main.h"

int main(int argc, char *argv[]) {
    int retval = run("CallStaticMethodTwoArg.class");

    if (retval == 44) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 44, retval);
        return 1;
    }
}
