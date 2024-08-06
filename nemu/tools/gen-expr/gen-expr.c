

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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
// #include <sdb/sdb.h>

// word_t expr(char *e, bool *success);

// this should be enough
static char buf[20000] = {};
char code_buf[20000 + 128] = {}; // a little larger than `buf`
char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int choose(int n) {
  return rand()%n;
}
int i = 0;

void gen_num() {
  int num = choose(9)+1;
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%d", num);
  if (strlen(buf) + strlen(buffer) < sizeof(buf)) {
    strcat(buf, buffer);
  }
}

void gen_char(char a) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%c", a);
  if (strlen(buf) + strlen(buffer) < sizeof(buf)) {
    strcat(buf, buffer);
  }
}

void gen_rand_op() {
  char ops[] = {'+', '-', '*', '/'};
  int op_index = choose(4);
  char buffer[2] = {ops[op_index], '\0'};
  if (strlen(buf) + strlen(buffer) < sizeof(buf)) {
    strcat(buf, buffer);
  }
}

int find_op(char *buf) {
  char a[5] = {'+', '-', '*', '/'};
  for (int i = 0; i < 4; i++) {
    if (buf[strlen(buf)-1] == a[i]) return 1;
  }
  return 0;
}


static void gen_rand_expr() {
  switch(choose(3)) {
    case 0: 
      if (buf[strlen(buf)-1] != ')') {
        gen_num();
      }
      else {
        gen_rand_expr();
      }
      break;
    case 1: 
      if (buf[0] != '\0' && find_op(buf)){
        gen_char('(');
        gen_rand_expr();
        gen_char(')');
      }
      else {
        gen_rand_expr();
      }
      break;
    default: gen_rand_expr();gen_rand_op();gen_rand_expr();break;
  }
}


static int zero() {
  char *p = buf;
  while(*p) {
    if (*p == '/' && (*(p + 1) == '0' || *(p + 1) == '(')) return 1;
    p++;
  }
  return 0;
}




int main(int argc, char *argv[]) {

  gen_rand_expr();

  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();
    if (zero()) {
      memset(buf, '\0', sizeof(buf));
      i--;
      // printf("%s\n", "waring!!!!!");
      continue;
    }
    // printf("%s\n", buf);


    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%d %s\n", result, buf);
    memset(buf, '\0', sizeof(buf));
    
  }
  return 0;
}


// 11+(2*(4+(7*9*4-4/3/9+2)-64-5))+4
// 9-9/6/3+2+(2*(1*1+7-4+(7*9*4-(8+4-4/5-8)/3/9+2)-8*8-5))+4
