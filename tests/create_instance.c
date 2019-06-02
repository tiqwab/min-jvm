#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[1] = {"CreateInstance.class"};
    int retval = run(classes, 1);

    if (retval == 49) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 49, retval);
        return 1;
    }
}

