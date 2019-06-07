#include "../main.h"

int main(int argc, char *argv[]) {
    char *classes[1] = {"StaticReferenceField.class"};
    int retval = run(classes, 1);

    if (retval == 51) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 51, retval);
        return 1;
    }
}


