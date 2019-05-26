#include <stdlib.h>
#include "../main.h"

int main(int argc, char *argv[]) {
    FILE *main_file;
    struct class_file main_class;
    struct method_info *method;
    struct code_attribute *code;
    int retval;

    main_file = fopen("CallStaticMethodNoArg.class", "r");
    if (main_file == NULL) {
        perror("fopen");
        exit(1);
    }

    parse_class(&main_class, main_file);

    // TODO should exec <init> here?

    method = find_method("main", &main_class);
    if (method == NULL) {
        fprintf(stderr, "not found method: %s\n", "main");
        return 1;
    }

    code = get_code(method, &main_class);
    if (code == NULL) {
        fprintf(stderr, "not found code of method: %s\n", "main");
        return 1;
    }

    retval = exec_method(method, code, &main_class);

    if (fclose(main_file) != 0) {
        perror("fclose");
        exit(1);
    }

    if (retval == 42) {
        return 0;
    } else {
        fprintf(stderr, "expect %d but actual %d\n", 42, retval);
        return 1;
    }
}
