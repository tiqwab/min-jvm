#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[1] = {"CallStaticMethodOneArg.class"};
    int retval = run(classes, 1);

    if (retval == 43) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 43, retval);
        return 1;
    }
}
