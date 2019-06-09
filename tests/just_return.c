#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[1] = {"JustReturn.class"};
    int retval = run(classes, 1);

    if (retval == 52) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 52, retval);
        return 1;
    }
}


