#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <unistd.h>
#include "main.h"

static int read_utf8(char *str, struct constant_utf8_info *cp);
static char *get_method_name(struct method_info *method, struct class_file *class);

static struct constant_fieldref_info *find_cp_fieldref(int index, struct class_file *class);
static struct constant_methodref_info *find_cp_methodref(int index, struct class_file *class);
static struct constant_class_info *find_cp_class(int index, struct class_file *class);
static struct constant_name_and_type_info *find_cp_name_and_type(int index, struct class_file *class);
static struct constant_utf8_info *find_cp_utf8(int index, struct class_file *class);

static char *get_field_name(struct field_info *field, struct class_file *class);
static char *get_field_descriptor(struct field_info *field, struct class_file *class);
static struct field_info *find_field(char *name, struct class_file *class);
static struct method_info *find_method(char *target_name, struct class_file *class);

static struct constant_utf8_info *get_this_class(struct class_file *class);

struct class_loader;
struct native_loader;

static int exec_method(struct method_info *current_method, struct code_attribute *current_code,
                       struct frame *prev_frame, struct class_file *current_class, struct class_loader *loader,
                       struct native_loader *native_loader);


//
// Class Loader
//

struct class_loader {
    int class_num;
    struct class_file *classes;
};

static int initialize_class(struct class_file *class, struct class_loader *loader, struct native_loader *native_loader) {
    // find <clinit> method
    struct method_info *method = find_method("<clinit>", class);
    if (method == NULL) {
        return 0;
    }

    struct code_attribute *code = get_code(method, class);
    if (code == NULL) {
        fprintf(stderr, "not found code attr in <clinit>\n");
        return -1;
    }

    struct frame *frame = initialize_frame(code->max_stack, code->max_locals);

    // exec <clinit>
    return exec_method(method, code, frame, class, loader, native_loader);
}

static int initialize_class_loader(struct class_loader *loader, char *class_names[], int len, struct native_loader *native_loader) {
    FILE *f;
    int i;
    struct class_file *class_files;

    class_files = calloc(sizeof(struct class_file), len);
    if (class_files == NULL) {
        return -1;
    }

    loader->class_num = len;
    loader->classes = class_files;

    for (i = 0; i < len; i++) {
        f = fopen(class_names[i], "r");
        if (f == NULL) {
            perror("fopen");
            return -1;
        }

        // Parse class
        // After parsing, initialize class by executing `<clinit>`
        parse_class(&class_files[i], f);

        if (initialize_class(&class_files[i], loader, native_loader) != 0) {
            return -1;
        }

        if (fclose(f) != 0) {
            perror("fclose");
            return -1;
        }
    }

    return 0;
}

/**
 * Return class having the same name as specified one.
 * Return NULL if not found.
 */
struct class_file *get_class(struct class_loader *loader, char *name) {
    int i;
    struct class_file *class;
    struct constant_utf8_info *utf8;
    char buf[1024];

    for (i = 0; i < loader->class_num; i++) {
        class = &loader->classes[i];
        utf8 = get_this_class(class);
        if (utf8 != NULL) {
            read_utf8(buf, utf8);
            if (strcmp(buf, name) == 0) {
                return class;
            }
        }
    }

    return NULL;
}

int tear_down_class_loader(struct class_loader *loader) {
    return 0;
}

//
// Native Loader
//

#define LIB_JAVA "libjava.so"

struct native_loader {
    void *handler;
};

static int exec_native_method(struct method_info *pInfo, struct frame *pFrame, struct class_file *class, struct native_loader *loader);

static int initialize_native_loader(struct native_loader *loader) {
    void *handler;

    handler = dlopen(LIB_JAVA, RTLD_LAZY);
    if (handler == NULL) {
        fprintf(stderr, "failed to load %s\n", LIB_JAVA);
        return -1;
    }

    loader->handler = handler;
    return 0;
}

//
// Instance Creation
//

#define FIELD_DESCRIPTOR_INT 'I'
#define FIELD_DESCRIPTOR_OBJECT 'L'

#define REFERENCE_NULL -1

struct class_instance_field {
    char *name;
    char *descriptor;
    void *data;
};

// There is no spec about structure of class instances.
// a naive implementation would be like Map[String, Any]?
struct class_instance {
    int field_num;
    struct class_file *class;
    struct class_instance_field **fields;
};

static int create_instance_field(struct class_instance_field **field, char *name, char *descriptor) {
    char *field_name;
    int name_len;

    (*field) = malloc(sizeof(struct class_instance_field));

    name_len = strlen(name);
    field_name = malloc(name_len + 1);
    strcpy(field_name, name);
    (*field)->name = field_name;

    switch (descriptor[0]) {
        case FIELD_DESCRIPTOR_INT:
            (*field)->descriptor = descriptor;
            (*field)->data = malloc(sizeof(u_int32_t));
            *((u_int32_t *) (*field)->data) = 0;
            break;
        case FIELD_DESCRIPTOR_OBJECT:
            (*field)->descriptor = descriptor;
            (*field)->data = malloc(sizeof(int));
            *((int *) (*field)->data) = REFERENCE_NULL;
            break;
        default:
            fprintf(stderr, "not yet implemented for %s in create_instance_field\n", descriptor);
            return -1;
    }

    return 0;
}

// FIXME: Remove limit for number of objects
#define MAX_INSTANCES 1024
static struct class_instance *instances[MAX_INSTANCES];
static int instance_count = 0;

/**
 * Create a new instance of the specified class.
 * Return index to reference the created object.
 * Return -1 if failed to create.
 */
static int create_instance(
        struct constant_class_info *cp_class, struct class_file *class, struct class_loader *loader) {
    int field_i, instance_i;

    if (instance_count >= MAX_INSTANCES) {
        return -1;
    }

    instance_i = instance_count;
    instances[instance_i] = calloc(1, sizeof(struct class_instance));
    instances[instance_i]->class = class;
    instances[instance_i]->field_num = class->fields_count;
    instances[instance_i]->fields = calloc(class->fields_count, sizeof(struct class_instance_field *));

    // initialize fields
    for (field_i = 0; field_i < class->fields_count; field_i++) {
        char *field_name = get_field_name(class->fields[field_i], class);
        char *field_descriptor = get_field_descriptor(class->fields[field_i], class);
        if (create_instance_field(&instances[instance_i]->fields[field_i], field_name, field_descriptor) != 0) {
            fprintf(stderr, "failed to initialize field\n");
            return -1;
        }
    }

    instance_count++;
    return instance_i;
}

static struct class_instance *get_instance(int index) {
    if (index >= instance_count || index < 0) {
        return NULL;
    }
    return instances[index];
}

static int get_instance_field(struct class_instance *instance, const char *name, void *value) {
    int i;
    struct class_instance_field *field;

    for (i = 0; i < instance->field_num; i++) {
        field = instance->fields[i];
        if (strcmp(field->name, name) == 0) {
            switch (field->descriptor[0]) {
                case FIELD_DESCRIPTOR_INT:
                    *((int *) value) = *((int *) field->data);
                    return 0;
                case FIELD_DESCRIPTOR_OBJECT:
                    fprintf(stderr, "not yet implemented for FIELD_DESCRIPTOR_OBJECT\n");
                    return -1;
                default:
                    return -1;
            }
        }
    }

    return -1;
}

static int put_instance_field(struct class_instance *instance, const char *name, void *value) {
    int i;
    struct class_instance_field *field;

    for (i = 0; i < instance->field_num; i++) {
        field = instance->fields[i];
        if (strcmp(field->name, name) == 0) {
            switch (field->descriptor[0]) {
                case FIELD_DESCRIPTOR_INT:
                    *((int *) field->data) = *((int *) value);
                    return 0;
                case FIELD_DESCRIPTOR_OBJECT:
                    fprintf(stderr, "not yet implemented for FIELD_DESCRIPTOR_OBJECT\n");
                    return -1;
                default:
                    return -1;
            }
        }
    }

    return -1;
}

/**
 * Return length of str.
 * The format is described in 4.4.7 The CONSTANT_Utf8_info Structure
 */
static int read_utf8(char *str, struct constant_utf8_info *cp) {
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
            *cp_info = (struct cp_info *) malloc(sizeof(struct constant_fieldref_info));
            if (*cp_info == NULL) {
                fprintf(stderr, "failed to prepare CONSTANT_FIELDREF cp_info\n");
                return -1;
            }
            ((struct constant_fieldref_info *) *cp_info)->tag = tag;
            ((struct constant_fieldref_info *) *cp_info)->class_index = read16(main_file);
            ((struct constant_fieldref_info *) *cp_info)->name_and_type_index = read16(main_file);
            return 0;
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

int parse_field(struct field_info **field, struct class_file *main_class, FILE *main_file) {
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

    // 4 u_int16_t fields, attributes pointers, and data
    *field = (struct field_info *) malloc(sizeof(u_int16_t) * 4 + sizeof(void **) + sizeof(int *));
    (*field)->access_flags = access_flags;
    (*field)->name_index = name_index;
    (*field)->descriptor_index = descriptor_index;
    (*field)->attributes_count = attributes_count;
    (*field)->attributes = attributes;
    (*field)->data = calloc(1, sizeof(int *));

    return 0;
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

    *method = (struct method_info *) malloc(sizeof(u_int16_t) * 4 + sizeof(void **));
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
    main_class->fields = calloc(main_class->fields_count, sizeof(void *));
    if (main_class->fields == NULL) {
        fprintf(stderr, "failed to prepare fields\n");
        return -1;
    }
    for (i = 0; i < main_class->fields_count; i++) {
        if (parse_field(&main_class->fields[i], main_class, main_file) != 0) {
            return -1;
        }
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

static char *get_field_name(struct field_info *field, struct class_file *class) {
    // TODO: not thread-safe
    // TODO: should not limit the length of field name
    static char name[1024];
    u_int16_t name_index;
    struct constant_utf8_info *utf8_info;

    name_index = field->name_index;
    utf8_info = find_cp_utf8(name_index, class);
    if (utf8_info == NULL) {
        return NULL;
    }

    read_utf8(name, utf8_info);
    return name;
}

static char *get_field_descriptor(struct field_info *field, struct class_file *class) {
    // TODO: not thread-safe
    static char descriptor[2];
    u_int16_t descriptor_index;
    struct constant_utf8_info *utf8_info;

    descriptor_index = field->descriptor_index;
    utf8_info = find_cp_utf8(descriptor_index, class);
    if (utf8_info == NULL) {
        return NULL;
    }

    read_utf8(descriptor, utf8_info);
    return descriptor;
}

/**
 * Find field_info from fields.
 * Return NULL if not found.
 */
static struct field_info *find_field(char *target_name, struct class_file *class) {
    // TODO: Check field descriptor as well as name
    int i;
    int target_name_len;
    char *name;
    int name_len;

    target_name_len = strlen(target_name);
    for (i = 0; i < class->fields_count; i++) {
        name = get_field_name(class->fields[i], class);
        if (name != NULL) {
            name_len = strlen(name);
            if (target_name_len == name_len && strncmp(name, target_name, name_len) == 0) {
                return (struct field_info *) class->fields[i];
            }
        }
    }

    return NULL;
}

/**
 * Find method_info from methods.
 * Return NULL if not found.
 */
static struct method_info *find_method(char *target_name, struct class_file *class) {
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

struct method_descriptor {
    int num;
};

/**
 * Parse method descriptor and store it to descriptor.
 * The format of method descriptor is defined in 4.3.3.
 * Return 0 if success, return -1 otherwise.
 */
static int get_method_descriptor(struct method_descriptor *descriptor, struct method_info *current_method, struct class_file *class) {
    char desc[1024], *p;
    struct constant_utf8_info *utf8 = find_cp_utf8(current_method->descriptor_index, class);
    read_utf8(desc, utf8);

    descriptor->num = 0;

    p = desc;
    if (*p != '(') {
        return -1;
    }
    p++;
    while (*p != ')') {
        switch(*p) {
            case 'I':
                descriptor->num++;
                break;
            case 'B':
                fprintf(stderr, "not yet implemented for %c\n", *p);
                return -1;
            case 'C':
                fprintf(stderr, "not yet implemented for %c\n", *p);
                return -1;
            case 'D':
                fprintf(stderr, "not yet implemented for %c\n", *p);
                return -1;
            case 'F':
                fprintf(stderr, "not yet implemented for %c\n", *p);
                return -1;
            case 'J':
                fprintf(stderr, "not yet implemented for %c\n", *p);
                return -1;
            case 'L':
                descriptor->num++;
                while (*p != ';') {
                    // skip for now
                    p++;
                }
                break;
            case 'S':
                fprintf(stderr, "not yet implemented for %c\n", *p);
                return -1;
            case 'Z':
                fprintf(stderr, "not yet implemented for %c\n", *p);
                return -1;
            case '[':
                // ignore for now
                break;
            default:
                fprintf(stderr, "unexpected character appears in method descriptor\n");
                return -1;
        }
        p++;
    }

    // ignore ReturnDescriptor for now

    return 0;
}

#define ACC_PUBLIC       0x0001
#define ACC_PRIVATE      0x0002
#define ACC_PROTECTED    0x0004
#define ACC_STATIC       0x0008
#define ACC_FINAL        0x0010
#define ACC_SYNCHRONIZED 0x0020
#define ACC_BRIDGE       0x0040
#define ACC_VARARGS      0x0080
#define ACC_NATIVE       0x0100
#define ACC_ABSTRACT     0x0400
#define ACC_STRICT       0x0800
#define ACC_SYNTHETIC    0x1000

static bool is_static_method(struct method_info *method) {
    return ((method->access_flags & ACC_STATIC) != 0);
}

static bool is_native_method(struct method_info *method) {
    return ((method->access_flags & ACC_NATIVE) != 0);
}

/**
 * Return this class name representing by constant_utf8_info.
 * This should not return NULL.
 */
static struct constant_utf8_info *get_this_class(struct class_file *class) {
    u_int16_t this_class_index = class->this_class;
    struct constant_class_info *cp_class = find_cp_class(this_class_index, class);
    if (cp_class == NULL) {
        return NULL;
    }
    return find_cp_utf8(cp_class->name_index, class);
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
 * Return constant_fieldref_info at the specified index of constant pool.
 * Return NULL if the item is not Fieldred.
 */
static struct constant_fieldref_info *find_cp_fieldref(int index, struct class_file *class) {
    struct cp_info *cp_info;

    if (index > class->constant_pool_count) {
        return NULL;
    }

    cp_info = class->constant_pool[index-1];
    if (cp_info->tag != CONSTANT_FIELDREF) {
        return NULL;
    }

    return (struct constant_fieldref_info *) cp_info;
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
    f->stack = (int32_t *) calloc(max_stack, sizeof(u_int32_t));
    if (max_stack > 0 && f->stack == NULL) {
        free(f);
        return NULL;
    }
    f->max_locals = max_locals;
    f->locals = (int32_t *) calloc(max_locals, sizeof(u_int32_t));
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

#define DESC_INT ('I')

/**
 * Return how many units are necessary for descriptor c
 */
static int get_operand_stack_units(char c) {
    switch (c) {
        case DESC_INT:
            return 1;
        default:
            fprintf(stderr, "not yet implemented for %c in get_operand_stack_units\n", c);
            return -1;
    }
}

// TODO: support other types than int
int push_operand_stack(int32_t item, struct frame *frame) {
    if (frame->stack_i >= frame->max_stack) {
        return -1;
    }
    frame->stack[frame->stack_i] = item;
    frame->stack_i++;
    return 0;
}

// TODO: support other types than int
int pop_operand_stack(int32_t *item, struct frame *frame) {
    if (frame->stack_i == 0) {
        return -1;
    }
    frame->stack_i--;
    *item = frame->stack[frame->stack_i];
    return 0;
}

static int status = 0;

static int exec_method(struct method_info *current_method, struct code_attribute *current_code,
        struct frame *prev_frame, struct class_file *current_class, struct class_loader *loader,
                struct native_loader *native_loader) {
    u_int8_t *p = current_code->code;
    u_int16_t current_code_len = current_code->code_length;
    struct method_descriptor current_descriptor;

    int i, j;
    int cp_index;
    int opcode, operand1, operand2, stack_unit;
    struct constant_fieldref_info *cp_fieldref;
    struct constant_methodref_info *cp_methodref;
    struct constant_class_info *cp_class;
    struct constant_name_and_type_info *cp_name_and_type;
    struct constant_utf8_info *cp_utf8;
    char buf[1024];
    struct field_info *field;
    struct method_info *method2;
    struct code_attribute *code2;
    struct class_file *class2;
    int instance_index;
    struct class_instance *instance;

    // prepare frame
    struct frame *current_frame = initialize_frame(current_code->max_stack, current_code->max_locals);
    if (current_frame == NULL) {
        fprintf(stderr, "failed to prepare frame\n");
        return 1;
    }

    if (get_method_descriptor(&current_descriptor, current_method, current_class) != 0) {
        fprintf(stderr, "failed to get descriptor\n");
        return 1;
    }

    i = 0;
    // for 'this' reference
    if (!is_static_method(current_method)) {
        i++;
    }
    for (j = current_descriptor.num; j > 0; j--) {
        pop_operand_stack(&current_frame->locals[j - 1 + i], prev_frame);
    }
    if (!is_static_method(current_method)) {
        pop_operand_stack(&current_frame->locals[0], prev_frame);
    }

    // interpret code
    while (p < current_code->code + current_code_len && status == 0) {
        if (*p == 0x02) {
            // iconst_m1
            p++;
            printf("iconst_m1\n");
            push_operand_stack(-1, current_frame);
        } else if (*p == 0x03) {
            // iconst_0
            p++;
            printf("iconst_0\n");
            push_operand_stack(0, current_frame);
        } else if (*p == 0x04) {
            // iconst_1
            p++;
            printf("iconst_1\n");
            push_operand_stack(1, current_frame);
        } else if (*p == 0x10) {
            // bipush
            p++;
            printf("bipush %d\n", (int32_t) *p);
            push_operand_stack((int32_t) *p, current_frame);
            p++;
        } else if (*p == 0x1a) {
            // iload_0
            p++;
            printf("iload_0\n");
            push_operand_stack((int32_t) current_frame->locals[0], current_frame);
        } else if (*p == 0x1b) {
            // iload_1
            // push value to stack from local 1
            p++;
            printf("iload_1\n");
            push_operand_stack((int32_t) current_frame->locals[1], current_frame);
        } else if (*p == 0x2a) {
            // aload_0
            p++;
            printf("aload_0\n");

            push_operand_stack((int32_t) current_frame->locals[0], current_frame);
        } else if (*p == 0x2b) {
            // aload_1
            p++;
            printf("aload_1\n");

            push_operand_stack((int32_t) current_frame->locals[1], current_frame);
        } else if (*p == 0x3c) {
            // istore_1
            // pop value from stack and store it in local 1
            p++;
            printf("istore_1\n");
            pop_operand_stack((int32_t *) &current_frame->locals[1], current_frame);
        } else if (*p == 0x4c) {
            // astore_1
            p++;
            printf("astore_1\n");

            pop_operand_stack((int32_t *) &current_frame->locals[1], current_frame);
        } else if (*p == 0x59) {
            // dup
            p++;
            printf("dup\n");

            pop_operand_stack(&operand1, current_frame);
            push_operand_stack(operand1, current_frame);
            push_operand_stack(operand1, current_frame);
        } else if (*p == 0x60) {
            // iadd
            p++;
            pop_operand_stack(&operand2, current_frame);
            pop_operand_stack(&operand1, current_frame);
            printf("iadd: %d + %d\n", operand1, operand2);
            push_operand_stack((int32_t) (operand1 + operand2), current_frame);
        } else if (*p == 0x64) {
            // isub
            p++;
            pop_operand_stack(&operand2, current_frame);
            pop_operand_stack(&operand1, current_frame);
            printf("isub: %d - %d\n", operand1, operand2);
            push_operand_stack((int32_t) (operand1 - operand2), current_frame);
        } else if (*p == 0xac) {
            // ireturn
            // pop value from the current frame and push to the invoker frame
            p++;
            pop_operand_stack((int32_t *) &operand1, current_frame);
            printf("ireturn %d\n", operand1);
            push_operand_stack((int32_t) operand1, prev_frame);
            break;
        } else if (*p == 0xb1) {
            // return
            p++;
            printf("return\n");
            break;
        } else if (*p == 0xb2 || *p == 0xb3) {
            // 0xb2: getstatic
            // 0xb3: putstatic
            opcode = *p;
            p++;
            cp_index = *p;
            p++;
            cp_index = (cp_index << 8) | *p;
            p++;

            if (opcode == 0xb2) {
                printf("getstatic %d\n", cp_index);
            } else {
                printf("putstatic %d\n", cp_index);
            }

            cp_fieldref = find_cp_fieldref(cp_index, current_class);
            if (cp_fieldref == NULL) {
                fprintf(stderr, "Fieldref is not found in constant pool\n");
                status = 1;
            }

            cp_class = find_cp_class(cp_fieldref->class_index, current_class);
            if (cp_class == NULL) {
                fprintf(stderr, "Class is not found in constant pool\n");
                status = 1;
            }

            // check class having field
            cp_utf8 = find_cp_utf8(cp_class->name_index, current_class);
            if (cp_utf8 == NULL) {
                fprintf(stderr, "Utf8 is not found in constant pool\n");
                status = 1;
            }
            read_utf8(buf, cp_utf8);
            class2 = get_class(loader, buf);
            if (class2 == NULL) {
                fprintf(stderr, "class not found: %s\n", buf);
                status = 1;
            }

            cp_name_and_type = find_cp_name_and_type(cp_fieldref->name_and_type_index, current_class);
            if (cp_name_and_type == NULL) {
                fprintf(stderr, "NameAndType is not found in constant pool\n");
                status = 1;
            }

            cp_utf8 = find_cp_utf8(cp_name_and_type->name_index, current_class);
            if (cp_utf8 == NULL) {
                fprintf(stderr, "Utf8 is not found in constant pool\n");
                status = 1;
            }
            read_utf8(buf, cp_utf8);

            field = find_field(buf, class2);
            if (field == NULL) {
                fprintf(stderr, "field %s is not found.\n", buf);
                status = 1;
            }

            if (opcode == 0xb2) {
                // getstatic
                push_operand_stack(*(field->data), current_frame);
            } else {
                // putstatic
                pop_operand_stack(&operand1, current_frame);
                *(field->data) = operand1;
            }
        } else if (*p == 0xb4 || *p == 0xb5) {
            // getfield (0xb4) or putfield (0xb5)
            opcode = *p;
            p++;
            cp_index = *p;
            p++;
            cp_index = (cp_index << 8) | *p;
            p++;
            if (opcode == 0xb4) {
                printf("getfield %d\n", cp_index);
            } else {
                printf("putfield %d\n", cp_index);
            }

            // field is expected to belong to current_class (6.5 putfield)
            cp_fieldref = find_cp_fieldref(cp_index, current_class);
            cp_name_and_type = find_cp_name_and_type(cp_fieldref->name_and_type_index, current_class);
            read_utf8(buf, find_cp_utf8(cp_name_and_type->descriptor_index, current_class));
            stack_unit = get_operand_stack_units(buf[0]);
            read_utf8(buf, find_cp_utf8(cp_name_and_type->name_index, current_class));

            // TODO: handle types other than int
            if (stack_unit != 1) {
                fprintf(stderr, "not implemented for stack_unit other than 1\n");
                status = 1;
            }

            if (opcode == 0xb4) {
                // getfield
                pop_operand_stack(&operand1, current_frame); // objectref

                instance = get_instance(operand1);
                if (instance == NULL) {
                    fprintf(stderr, "failed to get_instance\n");
                    status = 1;
                }

                if (get_instance_field(instance, buf, &operand2) < 0) {
                    fprintf(stderr, "failed to get_instance_field\n");
                    status = 1;
                }
                push_operand_stack(operand2, current_frame);
            } else {
                // putfield
                pop_operand_stack(&operand1, current_frame); // value
                pop_operand_stack(&operand2, current_frame); // objectref

                instance = get_instance(operand2);
                if (instance == NULL) {
                    fprintf(stderr, "failed to get_instance\n");
                    status = 1;
                }

                if (put_instance_field(instance, buf, &operand1) < 0) {
                    fprintf(stderr, "failed to put_instance_field\n");
                    status = 1;
                }
            }
        } else if (*p == 0xb6 || *p == 0xb7) {
            // invokevirtual (0xb6) or invokespecial (0xb7)
            // TODO: follow spec (what should be checked respectively?)
            opcode = *p;
            p++;
            cp_index = *p;
            p++;
            cp_index = (cp_index << 8) | *p;
            p++;
            if (opcode == 0xb6) {
                printf("invokevirtual %d\n", cp_index);
            } else {
                printf("invokespecial %d\n", cp_index);
            }

            cp_methodref = find_cp_methodref(cp_index, current_class);
            if (cp_methodref == NULL) {
                fprintf(stderr, "Methodref is not found in constant pool\n");
                status = 1;
            }

            cp_class = find_cp_class(cp_methodref->class_index, current_class);
            if (cp_class == NULL) {
                fprintf(stderr, "Class is not found in constant pool\n");
                status = 1;
            }

            // check class having method
            cp_utf8 = find_cp_utf8(cp_class->name_index, current_class);
            if (cp_utf8 == NULL) {
                fprintf(stderr, "Utf8 is not found in constant pool\n");
                status = 1;
            }
            read_utf8(buf, cp_utf8);
            class2 = get_class(loader, buf);
            if (class2 == NULL) {
                fprintf(stderr, "class not found: %s\n", buf);
                status = 1;
            }

            cp_name_and_type = find_cp_name_and_type(cp_methodref->name_and_type_index, current_class);
            if (cp_name_and_type == NULL) {
                fprintf(stderr, "NameAndType is not found in constant pool\n");
                status = 1;
            }

            cp_utf8 = find_cp_utf8(cp_name_and_type->name_index, current_class);
            if (cp_utf8 == NULL) {
                fprintf(stderr, "Utf8 is not found in constant pool\n");
                status = 1;
            }
            read_utf8(buf, cp_utf8);

            method2 = find_method(buf, class2);
            if (method2 == NULL) {
                fprintf(stderr, "not found method: %s\n", buf);
                status = 1;
            }

            code2 = get_code(method2, class2);
            if (code2 == NULL) {
                fprintf(stderr, "not found code\n");
                status = 1;
            }

            status = exec_method(method2, code2, current_frame, class2, loader, native_loader);
        } else if (*p == 0xb8) {
            // invokestatic
            p++;
            cp_index = *p;
            p++;
            cp_index = (cp_index << 8) | *p;
            p++;
            printf("invokestatic %d\n", cp_index);

            cp_methodref = find_cp_methodref(cp_index, current_class);
            if (cp_methodref == NULL) {
                fprintf(stderr, "Methodref is not found in constant pool\n");
                status = 1;
            }

            cp_class = find_cp_class(cp_methodref->class_index, current_class);
            if (cp_class == NULL) {
                fprintf(stderr, "Class is not found in constant pool\n");
                status = 1;
            }

            // check class having method
            cp_utf8 = find_cp_utf8(cp_class->name_index, current_class);
            if (cp_utf8 == NULL) {
                fprintf(stderr, "Utf8 is not found in constant pool\n");
                status = 1;
            }
            read_utf8(buf, cp_utf8);
            class2 = get_class(loader, buf);
            if (class2 == NULL) {
                fprintf(stderr, "class not found: %s\n", buf);
                status = 1;
            }

            cp_name_and_type = find_cp_name_and_type(cp_methodref->name_and_type_index, current_class);
            if (cp_name_and_type == NULL) {
                fprintf(stderr, "NameAndType is not found in constant pool\n");
                status = 1;
            }

            cp_utf8 = find_cp_utf8(cp_name_and_type->name_index, current_class);
            if (cp_utf8 == NULL) {
                fprintf(stderr, "Utf8 is not found in constant pool\n");
                status = 1;
            }
            read_utf8(buf, cp_utf8);

            method2 = find_method(buf, class2);
            if (method2 == NULL) {
                fprintf(stderr, "not found method: %s\n", buf);
                status = 1;
            }

            if (!is_static_method(method2)) {
                fprintf(stderr, "this is not static method\n");
                status = 1;
            }

            if (is_native_method(method2)) {
                status = exec_native_method(method2, current_frame, class2, native_loader);
            } else {
                code2 = get_code(method2, class2);
                if (code2 == NULL) {
                    fprintf(stderr, "not found code\n");
                    status = 1;
                }
                status = exec_method(method2, code2, current_frame, class2, loader, native_loader);
            }
        } else if (*p == 0xbb) {
            // new
            p++;
            cp_index = *p;
            p++;
            cp_index = (cp_index << 8) | *p;
            p++;
            printf("new %d\n", cp_index);

            // get class from constant pool
            cp_class = find_cp_class(cp_index, current_class);
            if (cp_class == NULL) {
                fprintf(stderr, "Class is not found in constant pool\n");
                status = 1;
            }
            cp_utf8 = find_cp_utf8(cp_class->name_index, current_class);
            if (cp_utf8 == NULL) {
                fprintf(stderr, "Utf8 is not found in constant pool\n");
                status = 1;
            }
            read_utf8(buf, cp_utf8);
            class2 = get_class(loader, buf);
            if (class2 == NULL) {
                fprintf(stderr, "class not found: %s\n", buf);
                status = 1;
            }

            // create instance
            instance_index = create_instance(cp_class, class2, loader);
            if (instance_index < 0) {
                fprintf(stderr, "failed to create instance\n");
                status = 1;
            }

            // push reference to operand stack
            push_operand_stack(instance_index, current_frame);
        } else {
            fprintf(stderr, "unknown inst\n");
            status = 1;
        }
    }

    free_frame(current_frame);
    return status;
}

/**
 * Replace char a in str to char b.
 */
static void strreplace(char *str, size_t len, char a, char b) {
    int i;

    for (i = 0; i < len; i++) {
        if (str[i] == a) {
            str[i] = b;
        }
    }
}

static void generate_native_method_name(char *native_method_name, char *class_name, char *method_name) {
    char *replaced_class_name;
    int class_name_len = strlen(class_name);

    replaced_class_name = malloc(class_name_len + 1);
    strncpy(replaced_class_name, class_name, class_name_len);

    strreplace(replaced_class_name, class_name_len, '/', '_');
    sprintf(native_method_name, "Java_%s_%s", replaced_class_name, method_name);

    free(replaced_class_name);
}

static int exec_native_method(struct method_info *method, struct frame *frame, struct class_file *class, struct native_loader *loader) {
    typedef void (*Func) (void *, void *, int);
    Func f;
    struct constant_utf8_info *cp_utf8;
    struct constant_class_info *cp_class;
    char method_name[1024], class_name[1024], native_method_name[1024];
    int operand;

    cp_utf8 = find_cp_utf8(method->name_index, class);
    read_utf8(method_name, cp_utf8);

    cp_class = find_cp_class(class->this_class, class);
    cp_utf8 = find_cp_utf8(cp_class->name_index, class);
    read_utf8(class_name, cp_utf8);

    generate_native_method_name(native_method_name, class_name, method_name);

    printf("%s\n", native_method_name);

    f = (Func) dlsym(loader->handler, native_method_name);

    pop_operand_stack(&operand, frame);

    if (f != NULL) {
        f(NULL, NULL, operand);
    } else {
        printf("not found native method: %s\n", native_method_name);
        status = 1;
    }

    return status;
}

int run(char *user_class_name[], int user_class_len) {
    char *main_class_name;
    struct class_file *main_class;
    struct class_loader loader;
    struct native_loader native_loader;
    struct method_info *method;
    struct code_attribute *code;
    struct frame *frame;
    int i, retval;
    char *c;

    if (initialize_native_loader(&native_loader) < 0) {
        fprintf(stderr, "failed to initialize native loader\n");
        return 1;
    }

    static char *stdlib_class_name[] = {"java/lang/Object.class", "java/lang/System.class"};
    static int stdlib_class_len = (sizeof(stdlib_class_name) / sizeof (stdlib_class_name[0]));

    int class_len = user_class_len + stdlib_class_len;
    char **class_name = calloc(class_len, sizeof(char *));
    for (i = 0; i < class_len; i++) {
        if (i < stdlib_class_len) {
            class_name[i] = stdlib_class_name[i];
        } else {
            class_name[i] = user_class_name[i - stdlib_class_len];
        }
    }

    if (initialize_class_loader(&loader, class_name, class_len, &native_loader) < 0) {
        fprintf(stderr, "failed to initiazlie class loader\n");
        return 1;
    }

    // Derive main class name without extension
    // e.g. First.class -> First
    main_class_name = malloc(strlen(user_class_name[0]) + 1);
    if (main_class_name == NULL) {
        fprintf(stderr, "failed to malloc\n");
        return 1;
    }
    strcpy(main_class_name, user_class_name[0]);
    c = strchr(main_class_name, '.');
    if (c == NULL) {
        fprintf(stderr, "failed to get main class name\n");
        return 1;
    }
    *c = '\0';

    main_class = get_class(&loader, main_class_name);
    if (main_class == NULL) {
        fprintf(stderr, "not found class: %s\n", main_class_name);
        return 1;
    }

    method = find_method("main", main_class);
    if (method == NULL) {
        fprintf(stderr, "not found method: %s\n", "main");
        return 1;
    }

    code = get_code(method, main_class);
    if (code == NULL) {
        fprintf(stderr, "not found code of method: %s\n", "main");
        return 1;
    }

    frame = initialize_frame(1, 0);
    if ((status = exec_method(method, code, frame, main_class, &loader, &native_loader)) != 0) {
        retval = status;
    } else {
        pop_operand_stack((int32_t *) &retval, frame);
    }

    tear_down_class_loader(&loader);

    return retval;
}

void request_shutdown(int s) {
    printf("set status as %d\n", s);
    status = s;
}
