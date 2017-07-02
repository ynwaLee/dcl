#ifndef __GET_ARG_H__
#define __GET_ARG_H__

char *get_arg_value_string(int argc, char **argv, char *str);
int get_arg_value_uint(int argc, char **argv, char *name, unsigned int *value);
int check_arg(int argc, char **argv, char *name);

#endif
