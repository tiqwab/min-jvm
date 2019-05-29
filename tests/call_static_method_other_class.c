#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[2] = {"CallStaticMethodCaller.class", "CallStaticMethodCallee.class"};
    int retval = run(classes, 2);

    if (retval == 43) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 43, retval);
        return 1;
    }
}
