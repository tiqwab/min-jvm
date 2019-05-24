#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// 4.4 The Constant Pool
struct cp_info {
    u_int8_t tag;
    void *info; // the structure depends on tag (as defined in 4.4)
};

// Constant pool tags (from Table 4.4-A)
#define CONSTANT_CLASS 7
#define CONSTANT_FIELDREF 9
#define CONSTANT_METHODREF 10
#define CONSTANT_INTERFACE_METHODREF 11
#define CONSTANT_STRING 8
#define CONSTANT_INTEGER 3
#define CONSTANT_FLOAT 4
#define CONSTANT_LONG 5
#define CONSTANT_DOUBLE 6
#define CONSTANT_NAME_AND_TYPE 12
#define CONSTANT_UTF8 1
#define CONSTANT_METHOD_HANDLE 15
#define CONSTANT_METHOD_TYPE 16
#define CONSTANT_INVOKE_DYNAMIC 18

// 4.4.1 The CONSTANT_Class_info Structure (cp_info)
struct constant_class_info {
    u_int8_t tag;
    u_int16_t name_index;
};

// 4.4.2 The CONSTANT_Methodref_info Structure (cp_info)
struct constant_methodref_info {
    u_int8_t tag;
    u_int16_t class_index;
    u_int16_t name_and_type_index;
};

// 4.4.6 The CONSTANT_NameAndType_info Structure (cp_info)
struct constant_name_and_type_info {
    u_int8_t tag;
    u_int16_t name_index;
    u_int16_t descriptor_index;
};

// 4.4.7 The CONSTANT_Utf8_info Structure (cp_info)
struct constant_utf8_info {
    u_int8_t tag;
    u_int16_t length;
    u_int8_t *bytes;
};

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
    return i-1;
}

// 4.5 Fields
struct field_info {

};

// 4.6 Methods
struct method_info {
    u_int16_t access_flags;
    u_int16_t name_index;
    u_int16_t descriptor_index;
    u_int16_t attributes_count;
    struct attribute_info **attributes;
};

// Table 4.6-A: Method access and property flags
#define ACC_PUBLIC 0x0001
#define ACC_PRIVATE 0x0002
#define ACC_PROTECTED 0x0004
#define ACC_STATIC 0x0008
#define ACC_FINAL 0x0010
#define ACC_SYNCHRONIZED 0x0020
#define ACC_BRIDGE 0x0040
#define ACC_VARARGS 0x0080
#define ACC_NATIVE 0x0100
#define ACC_ABSTRACT 0x0400
#define ACC_STRICT 0x0800
#define ACC_SYNTHETIC 0x1000

// 4.7 Attributes
struct attribute_info {
    u_int16_t attribute_name_index;
    u_int32_t attribute_length;
    void *info; // structure depends on attributes
};

// Predefined class file attributes (from Table 4.7-A, B, and C)
#define ATTR_CODE "Code"
#define ATTR_SOURCE_FILE "SourceFile"
#define ATTR_LINE_NUMBER_TABLE "LineNumberTable"

// 4.7.3 The Code Attribute (attribute_info)
struct exception_table_entry {
    u_int16_t start_pc;
    u_int16_t end_pc;
    u_int16_t handler_pc;
    u_int16_t catch_type;
};

struct code_attribute {
    u_int16_t attribute_name_index;
    u_int32_t attribute_length;
    u_int16_t max_stack;
    u_int16_t max_locals;
    u_int32_t code_length;
    u_int8_t *code;
    u_int16_t exception_table_length;
    struct exception_table_entry *exception_table;
    u_int16_t attributes_count;
    struct attribute_info **attributes;
};

#define ATTR_CODE_INFO(attr) ((struct code_attribute *) attr)
/*
#define ATTR_CODE_MAX_STACK(attr) (((struct code_attribute *) attr->info)->max_stack)
#define ATTR_CODE_MAX_LOCALS(attr) (((struct code_attribute *) attr->info)->max_locals)
#define ATTR_CODE_CODE_LENGTH(attr) (((struct code_attribute *) attr->info)->code_length)
#define ATTR_CODE_CODE(attr) (((struct code_attribute *) attr->info)->code)
#define ATTR_CODE_EXCEPTION_TABLE_LENGTH(attr) (((struct code_attribute *) attr->info)->exception_table_length)
#define ATTR_CODE_EXCEPTION_TABLE(attr) (((struct code_attribute *) attr->info)->exception_table)
#define ATTR_CODE_ATTRIBUTES_COUNT(attr) (((struct code_attribute *) attr->info)->attributes_count)
#define ATTR_CODE_ATTRIBUTES(attr) (((struct code_attribute *) attr->info)->attributes)
*/

// 4.7.10 The SourceFile Attribute (attribute_info)
struct source_file_attribute {
    u_int16_t attribute_name_index;
    u_int32_t attribute_length;
    u_int16_t sourcefile_index;
};

// 4.7.12 The LineNumberTable Attribute (attribute_info)
struct line_number_table_entry {
    u_int16_t start_pc;
    u_int16_t line_number;
};

struct line_number_table_attribute {
    u_int16_t attribute_name_index;
    u_int32_t attribute_length;
    u_int16_t line_number_table_length;
    struct line_number_table_entry *line_number_table;
};

#define ATTR_LINE_NUMBER_TABLE_INFO(attr) ((struct line_number_table_attribute *) attr)

// 4.1 The ClassFile Structure
struct class_file {
    u_int8_t magic[4];
    u_int16_t minor_version;
    u_int16_t major_version;
    u_int16_t constant_pool_count;
    struct cp_info **constant_pool;
    u_int16_t access_flags;
    u_int16_t this_class;
    u_int16_t super_class;
    u_int16_t interfaces_count;
    u_int16_t *interfaces;
    u_int16_t fields_count;
    struct field_info **fields;
    u_int16_t methods_count;
    struct method_info **methods;
    u_int16_t attributes_count;
    struct attribute_info **attributes;
};

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

        ((struct source_file_attribute *) (*attr)->info)->sourcefile_index = read16(main_file);
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

    return 0;
}

int main(int argc, char *argv[]) {
    FILE *main_file;
    struct class_file main_class;

    if (argc != 2) {
        fprintf(stderr, "usage: min_jvm <main_class>");
        exit(1);
    }

    main_file = fopen(argv[1], "r");
    if (main_file == NULL) {
        perror("fopen");
        exit(1);
    }

    parse_class(&main_class, main_file);

    if (fclose(main_file) != 0) {
        perror("fclose");
        exit(1);
    }

    return 0;
}