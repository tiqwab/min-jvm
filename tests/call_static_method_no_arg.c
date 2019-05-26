#include <stdlib.h>
#include "../main.h"

int main(int argc, char *argv[]) {
    FILE *main_file;
    struct class_file main_class;
    struct method_info *method;
    struct code_attribute *code;
    struct frame *frame;
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

    frame = initialize_frame(1, 0);
    if (exec_method(method, code, frame, &main_class) != 0) {
        fprintf(stderr, "failed to exec main\n");
        return 1;
    }
    pop_item_frame((int32_t *) &retval, frame);

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
