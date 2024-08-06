/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "memory/paddr.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void info_watchpoint();
void removing(int no);
void creat(char *args, int32_t res);
void wp_difftest();
void test_expr();


/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_si(char *args) {
  int step = 0;
  if (args == NULL) step = 1;
  else {
    sscanf(args, "%d", &step);
  }
  cpu_exec(1);
  return 0;
}

static int cmd_info(char *args) {
  if (strcmp(args, "r") == 0) {
    isa_reg_display();
  }
  else if (strcmp(args, "w") == 0) {
    info_watchpoint();
  }
  return 0;
}

static int cmd_x(char *args) {
  // printf("%s\n", args);
  int n;
  uint64_t addr;
  char *first_c = strtok(args, " ");
  char *addr_c = strtok(NULL, " ");
  sscanf(first_c, "%d", &n);
  sscanf(addr_c, "%lx", &addr);
  for (int i = 0; i < n; i++) {
    printf("%#lx\n", paddr_read(addr, 4));
    addr += 4;
  }

  // printf("%d\n", n);
  // printf("%#x\n", addr);


  return 0;
}

static int cmd_p(char *args) {
  if (strcmp(args, "test") == 0) {
    test_expr();
  }
  else {
    bool sucess = true;
    int32_t res = expr(args, &sucess);
    if(!sucess){
      printf("NO\n");
    }
    else {
      printf("%d\n", res);
    }
  }
  return 0;
}

static int cmd_d(char *args) {
  char *arg = strtok(NULL, "");
  if (!arg) {
    printf("Usage: d N\n");
    return 0;
  }
  int no = strtol(arg, NULL, 10);
  removing(no);
  return 0;
}

static int cmd_w(char *args) {
  if (!args){
    printf("Please input: w EXPR\n");
    return 0;
  }
  bool success = true;
  int32_t res = expr(args, &success);
  if (!success) {
    printf("Error!\n");
  } 
  else {
    creat(args, res);
  }
  return 0;
}

static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si", "One Step to Exec", cmd_si},
  {"x", "Scan the Mem", cmd_x},
  {"info", "Print the Regs", cmd_info},
  {"p", "Calu the Expr", cmd_p},
  {"d", "Delete the Watchpoint", cmd_d},
  {"w", "Set the Watchpoint", cmd_w}


  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void test_expr() {
  FILE *fp = fopen("/home/zty-ysyx/ysyx/ysyx-workbench/nemu/tools/gen-expr/input", "r");

  char *e = NULL;
  word_t test_value;
  size_t len = 0;
  ssize_t read;
  bool success = true;

  while (true) {
    if(fscanf(fp, "%ld ", &test_value) == -1) break;
    // printf("%ld\n", test_value);
    read = getline(&e, &len, fp);
    
    e[read-1] = '\0';
    word_t res = expr(e, &success);
    
    assert(success);
    if (res != test_value) {
      printf("expected: %ld, now: %ld\n", test_value, res);
      assert(0);
    }
  }

  fclose(fp);
  if (e) free(e);
  Log("Sucess!!");
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();
  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
