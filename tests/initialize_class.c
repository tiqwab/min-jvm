#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[1] = {"InitializeClass.class"};
    int retval = run(classes, 1);

    if (retval == 48) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 48, retval);
        return 1;
    }
}

