#include <stdlib.h>
#include <string.h>
#include "main.h"

static char *get_method_name(struct method_info *method, struct class_file *class);

static struct constant_methodref_info *find_cp_methodref(int index, struct class_file *class);
static struct constant_class_info *find_cp_class(int index, struct class_file *class);
static struct constant_name_and_type_info *find_cp_name_and_type(int index, struct class_file *class);
static struct constant_utf8_info *find_cp_utf8(int index, struct class_file *class);

/**
 * Return length of str.
 * The format is described in 4.4.7 The CONSTANT_Utf8_info Structure
 */
int read_utf8(char *str, struct constant_utf8_info *cp) {
    // TODO: current implementation cannot handle unicode higher than \u007F
    int i;

    for (i = 0; i < cp->length; i++) {
        if (cp->bytes[i] > 0x007f) {
            fprintf(stderr, "read_utf8 not yet implemented other than ASCII characters\n");
            return -1;
        }
        str[i] = cp->bytes[i];
    }
    str[i] = '\0';
    return i;
}

//
// read data from class file in big endian
//

static u_int8_t read8(FILE *main_file) {
    u_int8_t x;
    fread(&x, 1, 1, main_file);
    return x;
}

static u_int16_t read16(FILE *main_file) {
    u_int16_t x;
    fread(&x, 2, 1, main_file);
    return ((x & 0x00ff) << 8 | (x >> 8));
}

static u_int32_t read32(FILE *main_file) {
    u_int32_t x;
    fread(&x, 4, 1, main_file);
    return (((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24));
}

static size_t readn(void *dst, int n, FILE *main_file) {
    return fread(dst, 1, n, main_file);
}

int parse_cp_info(struct cp_info **cp_info, FILE *main_file) {
    u_int8_t tag;
    u_int16_t len;

    tag = read8(main_file);
    switch (tag) {
        case CONSTANT_CLASS:
            *cp_info = (struct cp_info *) malloc(sizeof(struct constant_class_info));
            if (*cp_info == NULL) {
                fprintf(stderr, "failed to prepare CONSTANT_CLASS cp_info\n");
                return -1;
            }
            ((struct constant_class_info *) *cp_info)->tag = tag;
            ((struct constant_class_info *) *cp_info)->name_index = read16(main_file);
            return 0;
        case CONSTANT_FIELDREF:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_FIELDREF\n");
            return -1;
        case CONSTANT_METHODREF:
            *cp_info = (struct cp_info *) malloc(sizeof(struct constant_methodref_info));
            if (*cp_info == NULL) {
                fprintf(stderr, "failed to prepare CONSTANT_METHODREF cp_info\n");
                return -1;
            }
            ((struct constant_methodref_info *) *cp_info)->tag = tag;
            ((struct constant_methodref_info *) *cp_info)->class_index = read16(main_file);
            ((struct constant_methodref_info *) *cp_info)->name_and_type_index = read16(main_file);
            return 0;
        case CONSTANT_INTERFACE_METHODREF:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_INTERFACE\n");
            return -1;
        case CONSTANT_STRING:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_STRING\n");
            return -1;
        case CONSTANT_INTEGER:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_INTEGER\n");
            return -1;
        case CONSTANT_FLOAT:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_FLOAT\n");
            return -1;
        case CONSTANT_LONG:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_LONG\n");
            return -1;
        case CONSTANT_DOUBLE:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_DOUBLE\n");
            return -1;
        case CONSTANT_NAME_AND_TYPE:
            *cp_info = (struct cp_info *) malloc(sizeof(struct constant_name_and_type_info));
            if (*cp_info == NULL) {
                fprintf(stderr, "failed to prepare CONSTANT_NAME_AND_TYPE cp_info\n");
                return -1;
            }
            ((struct constant_name_and_type_info *) *cp_info)->tag = tag;
            ((struct constant_name_and_type_info *) *cp_info)->name_index = read16(main_file);
            ((struct constant_name_and_type_info *) *cp_info)->descriptor_index = read16(main_file);
            return 0;
        case CONSTANT_UTF8:
            *cp_info = (struct cp_info *) malloc(sizeof(struct constant_utf8_info));
            if (*cp_info == NULL) {
                fprintf(stderr, "failed to prepare CONSTANT_UTF8 cp_info\n");
                return -1;
            }
            ((struct constant_utf8_info *) *cp_info)->tag = tag;
            len = read16(main_file);
            ((struct constant_utf8_info *) *cp_info)->length = len;
            ((struct constant_utf8_info *) *cp_info)->bytes = (u_int8_t *) malloc(len);
            readn(((struct constant_utf8_info *) *cp_info)->bytes, len, main_file);
            return 0;
        case CONSTANT_METHOD_HANDLE:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_METHOD_HANDLE\n");
            return -1;
        case CONSTANT_METHOD_TYPE:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_METHOD_TYPE\n");
            return -1;
        case CONSTANT_INVOKE_DYNAMIC:
            fprintf(stderr, "not yet implemented cp_info: CONSTANT_INVOKE_DYNAMIC\n");
            return -1;
        default:
            fprintf(stderr, "unexpected tag: %d\n", tag);
            return -1;
    }
}

int parse_attribute(struct attribute_info **attr, struct class_file *main_class, FILE *main_file) {
    struct constant_utf8_info *cp;
    u_int16_t attr_name_index;
    u_int32_t attr_length;
    char attr_name[1024]; // FIXME: there is no limit in spec?
    int attr_len;
    int i;

    attr_name_index = read16(main_file);
    attr_length = read32(main_file);

    cp = (struct constant_utf8_info *) main_class->constant_pool[attr_name_index-1];
    attr_len = read_utf8(attr_name, cp);
    if (attr_len < 0) {
        return -1;
    }
    if (strncmp(attr_name, ATTR_CODE, attr_len) == 0) {
        *attr = (struct attribute_info *) malloc(sizeof(struct code_attribute));
        (*attr)->attribute_name_index = attr_name_index;
        (*attr)->attribute_length = attr_length;

        ATTR_CODE_INFO((*attr))->max_stack = read16(main_file);
        ATTR_CODE_INFO((*attr))->max_locals = read16(main_file);

        // code
        ATTR_CODE_INFO((*attr))->code_length = read32(main_file);
        if (ATTR_CODE_INFO((*attr))->code_length > 0) {
            ATTR_CODE_INFO((*attr))->code = (u_int8_t *) malloc(ATTR_CODE_INFO((*attr))->code_length);
        }
        readn((ATTR_CODE_INFO((*attr)))->code, ATTR_CODE_INFO((*attr))->code_length, main_file);

        // exception_table
        ATTR_CODE_INFO((*attr))->exception_table_length = read16(main_file);
        if (ATTR_CODE_INFO((*attr))->exception_table_length > 0) {
            ATTR_CODE_INFO((*attr))->exception_table = calloc(
                    ATTR_CODE_INFO((*attr))->exception_table_length,
                    sizeof(struct exception_table_entry));
        }
        for (i = 0; i < ATTR_CODE_INFO((*attr))->exception_table_length; i++) {
            ATTR_CODE_INFO((*attr))->exception_table[i].start_pc = read16(main_file);
            ATTR_CODE_INFO((*attr))->exception_table[i].end_pc = read16(main_file);
            ATTR_CODE_INFO((*attr))->exception_table[i].handler_pc = read16(main_file);
            ATTR_CODE_INFO((*attr))->exception_table[i].catch_type = read16(main_file);
        }

        // attributes
        ATTR_CODE_INFO((*attr))->attributes_count = read16(main_file);
        if (ATTR_CODE_INFO((*attr))->attributes_count > 0) {
            /*
            ATTR_CODE_INFO((*attr))->attributes = malloc(
                    ATTR_CODE_INFO((*attr))->attributes_count * sizeof(void *));
             */
            ATTR_CODE_INFO((*attr))->attributes = (struct attribute_info **) calloc(
                    ATTR_CODE_INFO((*attr))->attributes_count,
                    sizeof(void *));
        }
        for (i = 0; i < ATTR_CODE_INFO((*attr))->attributes_count; i++) {
            if (parse_attribute(&ATTR_CODE_INFO((*attr))->attributes[i], main_class, main_file) != 0) {
                return -1;
            }
        }

        return 0;
    } else if (strncmp(attr_name, ATTR_SOURCE_FILE, attr_len) == 0) {
        *attr = (struct attribute_info *) malloc(sizeof(struct source_file_attribute));
        (*attr)->attribute_name_index = attr_name_index;
        (*attr)->attribute_length = attr_length;

        ((struct source_file_attribute *) (*attr))->sourcefile_index = read16(main_file);
        return 0;
    } else if (strncmp(attr_name, ATTR_LINE_NUMBER_TABLE, attr_len) == 0) {
        *attr = (struct attribute_info *) malloc(sizeof(struct line_number_table_attribute));
        (*attr)->attribute_name_index = attr_name_index;
        (*attr)->attribute_length = attr_length;

        ATTR_LINE_NUMBER_TABLE_INFO((*attr))->line_number_table_length = read16(main_file);
        ATTR_LINE_NUMBER_TABLE_INFO((*attr))->line_number_table = calloc(
                ATTR_LINE_NUMBER_TABLE_INFO((*attr))->line_number_table_length,
                sizeof(struct line_number_table_entry));
        for (i = 0; i < ATTR_LINE_NUMBER_TABLE_INFO((*attr))->line_number_table_length; i++) {
            ATTR_LINE_NUMBER_TABLE_INFO((*attr))->line_number_table[i].start_pc = read16(main_file);
            ATTR_LINE_NUMBER_TABLE_INFO((*attr))->line_number_table[i].line_number = read16(main_file);
        }
        return 0;
    } else {
        fprintf(stderr, "not yet implemented for attr_name: %s\n", attr_name);
        return -1;
    }
}

int parse_method(struct method_info **method, struct class_file *main_class, FILE *main_file) {
    u_int16_t access_flags, name_index, descriptor_index, attributes_count;
    int i;
    struct attribute_info **attributes;

    access_flags = read16(main_file);
    name_index = read16(main_file);
    descriptor_index = read16(main_file);

    attributes_count = read16(main_file);
    attributes = (struct attribute_info **) calloc(attributes_count, sizeof(void *));
    for (i = 0; i < attributes_count; i++) {
        if (parse_attribute(&attributes[i], main_class, main_file) != 0) {
            return -1;
        }
    }

    *method = (struct method_info *) malloc(sizeof(u_int16_t) * 4 + sizeof(void *) * attributes_count);
    (*method)->access_flags = access_flags;
    (*method)->name_index = name_index;
    (*method)->descriptor_index = descriptor_index;
    (*method)->attributes_count = attributes_count;
    (*method)->attributes = attributes;

    return 0;
}

int parse_class(struct class_file *main_class, FILE *main_file) {
    unsigned char buf[256];
    int i;
    static unsigned char magic[4] = {0xca, 0xfe, 0xba, 0xbe};

    // parse magic
    fread(main_class->magic, 1, 4, main_file);
    printf("%d\n", main_class->magic[0]);
    printf("%d\n", main_class->magic[1]);
    printf("%d\n", main_class->magic[2]);
    printf("%d\n", main_class->magic[3]);
    if (memcmp(main_class->magic, magic, 4) != 0) {
        fprintf(stderr, "magic is illegal\n");
        return -1;
    }

    // parse minor_version and major_version
    main_class->minor_version = read16(main_file);
    main_class->major_version = read16(main_file);
    printf("version: %d.%d\n", main_class->major_version, main_class->minor_version);

    // parse constant_pool_count
    main_class->constant_pool_count = read16(main_file);
    main_class->constant_pool = (struct cp_info **) calloc(main_class->constant_pool_count, sizeof(void *));
    if (main_class->constant_pool == NULL) {
        fprintf(stderr, "failed to prepare constant_pool\n");
        return -1;
    }

    // parse constant_pool
    for (i = 0; i < main_class->constant_pool_count - 1; i++) {
        if (parse_cp_info(&main_class->constant_pool[i], main_file) != 0) {
            return -1;
        }
        if (main_class->constant_pool[i] != NULL) {
            printf("tag: %d\n", ((struct constant_class_info *) main_class->constant_pool[i])->tag);
        }
    }

    // parse access_flags
    main_class->access_flags = read16(main_file);
    printf("access_flags: %d\n", main_class->access_flags);

    // parse this_class
    main_class->this_class = read16(main_file);
    printf("this_class: %d\n", main_class->this_class);

    // parse super_class
    main_class->super_class = read16(main_file);
    printf("super_class: %d\n", main_class->super_class);

    // parse interfaces_count
    main_class->interfaces_count = read16(main_file);
    printf("interfaces_count: %d\n", main_class->interfaces_count);

    // parse interfaces
    // TODO
    if (main_class->interfaces_count > 0) {
        fprintf(stderr, "not yet implemented to parse interfaces\n");
        return -1;
    }

    // parse fields_count
    main_class->fields_count = read16(main_file);
    printf("fields_count: %d\n", main_class->fields_count);

    // parse fields
    // TODO
    if (main_class->fields_count > 0) {
        fprintf(stderr, "not yet implemented to parse interfaces\n");
        return -1;
    }

    // parse methods_count
    main_class->methods_count = read16(main_file);
    printf("methods_count: %d\n", main_class->methods_count);

    // parse methods
    main_class->methods = calloc(main_class->methods_count, sizeof(void *));
    if (main_class->methods == NULL) {
        fprintf(stderr, "failed to prepare methods\n");
        return -1;
    }
    for (i = 0; i < main_class->methods_count; i++) {
        if (parse_method(&main_class->methods[i], main_class, main_file) != 0) {
            return -1;
        }
    }

    // parse attributes_count
    main_class->attributes_count = read16(main_file);
    printf("attributes_count: %d\n", main_class->attributes_count);

    // parse attributes
    main_class->attributes = calloc(main_class->attributes_count, sizeof(void *));
    if (main_class->attributes == NULL) {
        fprintf(stderr, "failed to prepare methods\n");
        return -1;
    }
    for (i = 0; i < main_class->attributes_count; i++) {
        if (parse_attribute(&main_class->attributes[i], main_class, main_file) != 0) {
            return -1;
        }
    }

    return 0;
}

/**
 * Find method_info from methods.
 * Return NULL if not found.
 */
struct method_info *find_method(char *target_name, struct class_file *class) {
    // TODO: Check method signature as well as name
    int i;
    int target_name_len;
    char *name;
    int name_len;

    target_name_len = strlen(target_name);
    for (i = 0; i < class->methods_count; i++) {
        name = get_method_name(class->methods[i], class);
        if (name != NULL) {
            name_len = strlen(name);
            if (target_name_len == name_len && strncmp(name, target_name, name_len) == 0) {
                return (struct method_info *) class->methods[i];
            }
        }
    }

    return NULL;
}

/**
 * Return code_attribute of the passed method_info.
 * Return NULL if not found.
 */
struct code_attribute *get_code(struct method_info *method, struct class_file *class) {
    int j;
    int attr_name_index;
    char name[1024];

    for (j = 0; j < method->attributes_count; j++) {
        attr_name_index = method->attributes[j]->attribute_name_index;
        read_utf8(name, (struct constant_utf8_info *) class->constant_pool[attr_name_index-1]);
        if (strncmp(name, ATTR_CODE, 4) == 0) {
            return (struct code_attribute *) method->attributes[j];
        }
    }

    return NULL;
}

/**
 * Return name of the method.
 */
static char *get_method_name(struct method_info *method, struct class_file *class) {
    // TODO: not thread-safe
    // TODO: should not limit the length of methods
    static char name[1024];
    u_int16_t name_index;
    struct cp_info *cp_info;
    struct constant_utf8_info *utf8_info;

    name_index = method->name_index;
    if (name_index > class->constant_pool_count) {
        return NULL;
    }

    cp_info = class->constant_pool[name_index - 1];
    if (cp_info->tag != CONSTANT_UTF8) {
        return NULL;
    }
    utf8_info = (struct constant_utf8_info *) cp_info;

    read_utf8(name, utf8_info);

    return name;
}

/**
 * Return constant_class_info at the specified index of constant pool.
 * Return NULL if the item is not Class.
 */
static struct constant_class_info *find_cp_class(int index, struct class_file *class) {
    struct cp_info *cp_info;

    if (index > class->constant_pool_count) {
        return NULL;
    }

    cp_info = class->constant_pool[index-1];
    if (cp_info->tag != CONSTANT_CLASS) {
        return NULL;
    }

    return (struct constant_class_info *) cp_info;
}

/**
 * Return constant_methodref_info at the specified index of constant pool.
 * Return NULL if the item is not Methodref.
 */
static struct constant_methodref_info *find_cp_methodref(int index, struct class_file *class) {
    struct cp_info *cp_info;

    if (index > class->constant_pool_count) {
        return NULL;
    }

    cp_info = class->constant_pool[index-1];
    if (cp_info->tag != CONSTANT_METHODREF) {
        return NULL;
    }

    return (struct constant_methodref_info *) cp_info;
}

/**
 * Return constant_name_and_type_info at the specified index of constant pool.
 * Return NULL if the item is not NameAndType
 */
static struct constant_name_and_type_info *find_cp_name_and_type(int index, struct class_file *class) {
    struct cp_info *cp_info;

    if (index > class->constant_pool_count) {
        return NULL;
    }

    cp_info = class->constant_pool[index-1];
    if (cp_info->tag != CONSTANT_NAME_AND_TYPE) {
        return NULL;
    }

    return (struct constant_name_and_type_info *) cp_info;
}

/**
 * Return constant_utf8_info at the specified index of constant pool.
 * Return NULL if the item is not Utf8.
 */
static struct constant_utf8_info *find_cp_utf8(int index, struct class_file *class) {
    struct cp_info *cp_info;

    if (index > class->constant_pool_count) {
        return NULL;
    }

    cp_info = class->constant_pool[index-1];
    if (cp_info->tag != CONSTANT_UTF8) {
        return NULL;
    }

    return (struct constant_utf8_info *) cp_info;
}

/**
 * Allocate and initialize frame.
 * Return NULL if failed to initialize.
 */
struct frame *initialize_frame(int max_stack, int max_locals) {
    struct frame *f = (struct frame *) malloc(sizeof(struct frame));
    if (f == NULL) {
        return NULL;
    }
    f->max_stack = max_stack;
    f->stack_i = 0;
    f->stack = (u_int32_t *) calloc(max_stack, sizeof(u_int32_t));
    if (max_stack > 0 && f->stack == NULL) {
        free(f);
        return NULL;
    }
    f->max_locals = max_locals;
    f->locals = (u_int32_t *) calloc(max_locals, sizeof(u_int32_t));
    if (max_locals > 0 && f->locals == NULL) {
        free(f);
        return NULL;
    }
    return f;
}

/**
 * Free allocated frame.
 */
void free_frame(struct frame *frame) {
    free(frame->stack);
    free(frame->locals);
    free(frame);
}

int push_item_frame(int32_t item, struct frame *frame) {
    if (frame->stack_i >= frame->max_stack) {
        return -1;
    }
    frame->stack[frame->stack_i] = item;
    frame->stack_i++;
    return 0;
}

int pop_item_frame(int32_t *item, struct frame *frame) {
    if (frame->stack_i == 0) {
        return -1;
    }
    frame->stack_i--;
    *item = frame->stack[frame->stack_i];
    return 0;
}

int exec_method(struct method_info *method, struct code_attribute *code, struct frame *prev_frame, struct class_file *class) {
    u_int8_t *p = code->code;
    u_int16_t code_len = code->code_length;
    int operand;
    struct constant_methodref_info *cp_methodref;
    struct constant_class_info *cp_class;
    struct constant_name_and_type_info *cp_name_and_type;
    struct constant_utf8_info *cp_utf8;
    char buf[1024];
    struct method_info *method2;
    struct code_attribute *code2;

    struct frame *current_frame = initialize_frame(code->max_stack, code->max_locals);
    if (current_frame == NULL) {
        fprintf(stderr, "failed to prepare frame\n");
        return -1;
    }

    while (p < code->code + code_len) {
        if (*p == 0x10) {
            // bipush
            p++;
            printf("bipush %d\n", (int32_t) *p);
            push_item_frame((int32_t) *p, current_frame);
            p++;
        } else if (*p == 0x1b) {
            // iload_1
            // push value to stack from local 1
            p++;
            printf("iload_1\n");
            push_item_frame((int32_t) current_frame->locals[1], current_frame);
        } else if (*p == 0x3c) {
            // istore_1
            // pop value from stack and store it in local 1
            p++;
            printf("istore_1\n");
            pop_item_frame((int32_t *) &current_frame->locals[1], current_frame);
        } else if (*p == 0xac) {
            // ireturn
            // pop value from the current frame and push to the invoker frame
            p++;
            pop_item_frame((int32_t *) &operand, current_frame);
            printf("ireturn %d\n", operand);
            push_item_frame((int32_t) operand, prev_frame);
            return 0;
        } else if (*p == 0xb8) {
            // invokestatic
            p++;
            operand = *p; p++;
            operand = (operand << 8) | *p; p++;
            printf("invokestatic %d\n", operand);

            cp_methodref = find_cp_methodref(operand, class);
            if (cp_methodref == NULL) {
                fprintf(stderr, "Methodref is not found in constant pool\n");
                return -1;
            }

            cp_class = find_cp_class(cp_methodref->class_index, class);
            if (cp_class == NULL) {
                fprintf(stderr, "Class is not found in constant pool\n");
                return -1;
            }

            cp_name_and_type = find_cp_name_and_type(cp_methodref->name_and_type_index, class);
            if (cp_name_and_type == NULL) {
                fprintf(stderr, "NameAndType is not found in constant pool\n");
                return -1;
            }

            cp_utf8 = find_cp_utf8(cp_name_and_type->name_index, class);
            if (cp_utf8 == NULL) {
                fprintf(stderr, "Utf8 is not found in constant pool\n");
                return -1;
            }

            read_utf8(buf, cp_utf8);

            method2 = find_method(buf, class);
            if (method2 == NULL) {
                fprintf(stderr, "not found method\n");
                return -1;
            }

            code2 = get_code(method2, class);
            if (code2 == NULL) {
                fprintf(stderr, "not found code\n");
                return -1;
            }

            exec_method(method2, code2, current_frame, class);
        } else {
            fprintf(stderr, "unknown inst\n");
            return -1;
        }
    }

    free_frame(current_frame);
    return 0;
}
