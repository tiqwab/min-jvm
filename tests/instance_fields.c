#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[1] = {"InstanceFields.class"};
    int retval = run(classes, 1);

    if (retval == 50) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 50, retval);
        return 1;
    }
}


