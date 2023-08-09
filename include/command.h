#ifndef _COMMAND_H_
#define _COMMAND_H_

typedef int (*cmd_func_t)(int argc, char **argv);

struct cmd_entry {
  char *name;
  cmd_func_t func;
  int arg_count;
  char *usage;
};

int command_main(int argc, char **argv);

int command_add(struct cmd_entry *cmd);

#endif
