#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "get_arg.h"

static bool isalldigit(char *str, unsigned int len)
{
    int i;

    if (!str)
        return 0;

    for (i = 0; i < len; i++)
    {
        if (!isdigit (str[i]))
            return false;
    }

    return true;
}

char *get_arg_value_string(int argc, char **argv, char *name)
{
    int i = 0;

    for(i = 0; i < argc; i++)
    {
        if(!strcmp(argv[i], name))
        {
            break;
        }
    }

    if ((i == argc)||(i == (argc-1)))
        return NULL;

    return (char *)argv[i + 1];
}

int get_arg_value_uint(int argc, char **argv, char *name, unsigned int *value)
{
    char *v;

    v = get_arg_value_string(argc, argv, name);
    if (v == NULL)
        return -1;

    if(isalldigit(v, strlen(v)))
    {
        *value = (unsigned int)atoi(v);
    }
    else
        return -2;

    return 0;
}

int check_arg(int argc, char **argv, char *name)
{
    int i = 0;
    for(i = 0; i < argc; i++)
    {
        if(!strcmp(argv[i], name))
        {
            return 1;
        }
    }
    
    return 0;
}

