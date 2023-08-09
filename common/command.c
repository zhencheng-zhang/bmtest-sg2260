#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system_common.h"
#include "command.h"
#include "uart.h"
#if defined(BUILD_TEST_CASE_audio)
#include "audio.h"
#endif

#define BM_SHELL_MAX_CMDS	128

int do_help(int argc, char **argv);
int do_rm(int argc, char **argv);
int do_wm(int argc, char **argv);

static struct cmd_entry cmd_list[BM_SHELL_MAX_CMDS] = {
  {"help", do_help, 0, ""},
  {"rm", do_rm, 1, "address"},
  {"wm", do_wm, 2, "address value"},
//#if defined(BUILD_TEST_CASE_audio)
 // {"au", do_audio, 5, "audio/i2c/i2s usage"},
//#endif
  {NULL, NULL, 0, NULL}
};

int do_help(int argc, char **argv)
{
  struct cmd_entry *ptr = cmd_list;

  printf("Command List:\n");
  for (; ptr->name != NULL; ptr++) {
    printf("  %s %s\n", ptr->name, ptr->usage);
  }
  return 0;
}

int do_rm(int argc, char **argv)
{
  void *addr;
  unsigned int val;
  //printf("%s %s\n", argv[0], argv[1]);
  addr = (void *)strtoul(argv[1], NULL, 16);
  printf("read addr = 0x%p, ", addr);
  val = __raw_readl(addr);
  printf("value = 0x%x\n", val);
  return 0;
}

int do_wm(int argc, char **argv)
{
  void *addr;
  unsigned int val;
  //printf("%s %s %s\n", argv[0], argv[1], argv[2]);
  addr =(void *)strtoul(argv[1], NULL, 16);
  val = strtoul(argv[2], NULL, 16);
  printf("write addr = 0x%p, value = 0x%x\n", addr, val);
  __raw_writel(addr, val);
  return 0;
}

static struct cmd_entry * find_cmd(char *cmd)
{
  struct cmd_entry *ptr = cmd_list;
  for (; ptr->name != NULL; ptr++) {
    if (strcmp(cmd, ptr->name) == 0)
      return ptr;
  }
  return NULL;
}

int command_main(int argc, char **argv)
{
  struct cmd_entry *ptr = find_cmd(argv[0]);
  if (ptr == NULL) {
     printf("unknown command %s\n", argv[0]);
     return -1;
  }
#if 0  // Support the number of command parameters is variable
  if (argc != (ptr->arg_count + 1)) {
    printf("get cmd argc=%d\n", argc);
    for(int i=0; i<argc; i++){
      printf("%s ", argv[i]);
    }
    printf("\n");
#if defined(BUILD_TEST_CASE_audio)
  if (strcmp(argv[0], "au")== 0)
  {
    debug("Execute audio command\n");
    return ptr->func(argc, argv);
  }
#endif
    printf("%s with wrong argument, usage:\n", ptr->name);
    printf("  %s %s\n", ptr->name, ptr->usage);
    return -1;
  }
#endif
  return ptr->func(argc, argv);
}

int command_add(struct cmd_entry *cmd)
{
    struct cmd_entry *cptr;
    int i;

    if( ( !cmd ) || ( !cmd->name ) || ( !cmd->func ) )
        return -1;

    for (cptr = cmd_list, i = 0; i < BM_SHELL_MAX_CMDS; cptr++, i++)
    {
      if (!cptr->name)
      {
          memcpy((char*)cptr, (char*)cmd, sizeof(struct cmd_entry));
          return 0;
      }
    }

    return -1;
}

