//
// Created by nm on 5/25/19.
//

#ifndef MIN_JVM_MAIN_H
#define MIN_JVM_MAIN_H

#include <stdio.h>
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

// 4.4.2 The CONSTANT_Fieldref_info Structures (cp_info)
struct constant_fieldref_info {
    u_int8_t tag;
    u_int16_t class_index;
    u_int16_t name_and_type_index;
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

// 4.5 Fields
struct field_info {
    u_int16_t access_flags;
    u_int16_t name_index;
    u_int16_t descriptor_index;
    u_int16_t attributes_count;
    struct attribute_info **attributes;
    // TODO: this is not in the spec.
    // TODO: cannot handle only int
    int *data;
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

int parse_class(struct class_file *main_class, FILE *main_file);

struct code_attribute *get_code(struct method_info *method, struct class_file *class);

//
// Frame
//

struct frame {
    u_int16_t max_stack;
    int stack_i;
    int32_t *stack;
    u_int16_t max_locals;
    int32_t *locals;
};

struct frame *initialize_frame(int max_stack, int max_locals);

void free_frame(struct frame *frame);

int push_operand_stack(int32_t item, struct frame *frame);

int pop_operand_stack(int32_t *item, struct frame *frame);

//
// Run main class
//

/**
 * Run program by specifying class name which has a main method.
 * Return exit code.
 */
int run(char *class_name[], int len);

#endif //MIN_JVM_MAIN_H
