#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[1] = {"StaticField.class"};
    int retval = run(classes, 1);

    if (retval == 47) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 47, retval);
        return 1;
    }
}
