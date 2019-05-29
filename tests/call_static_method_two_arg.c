#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[1] = {"CallStaticMethodTwoArg.class"};
    int retval = run(classes, 1);

    if (retval == 44) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 44, retval);
        return 1;
    }
}
