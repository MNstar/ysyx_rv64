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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/paddr.h>


enum {
  TK_NOTYPE = 256, 
  TK_EQ,
  NUM,
  LEFT,
  RIGHT,
  REG,
  HEX,
  TK_NOEQ,
  TK_AND,
  POINT,
  NEGTIVE,
  STRING

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"!=", TK_NOEQ},
  {"&&", TK_AND},

  //My code...
  {"\\-", '-'},
  {"\\*", '*'},
  {"\\/", '/'},
  {"\\b[0-9]+\\b", NUM},
  {"\\(", '('},
  {"\\)", ')'},
  {"\\$(\\$0|ra|[sgt]p|t[0-6]|a[0-7]|s([0-9]|1[0-1])|pc)", REG},
  {"0[xX][0-9a-fA-F]+", HEX},
  {"[A-Za-z]+", STRING}
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32000];
} Token;

static Token tokens[32000] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

//292+6/(4*8)/1*3
/*My code*/
int check(int p, int q) {
  int count = 0; //括号的数量
  if (tokens[p].type == '(' && tokens[q].type == ')') {
    for (int i = p + 1; i < q; i++) { //在表达式中遍历
      if (tokens[i].type == '(') { // 如果是左括号
        if (tokens[i+1].type == ')') return 0; // 如果下一个就是右括号说明括号内无值 返回0
        count++; //左括号的数量加1
      }
      else if (tokens[i].type == ')') {
        if (count == 0) return 0;//如果是右括号但是左括号的数量为0 则匹配错误返回0
        else count--;//括号配成一对 数量-1
      }
      if (i == q - 1) { //如果遍历到尽头
        if (count == 0) return 1; //括号数量匹配无误 返回1
        else return 0;
      }
    }
  }
  return 0;
}

/*My code*/
word_t eval(int p, int q)
{
    if (p > q) {
        printf("ERROR!!!!!!\n");
        assert(0);
    }
    else if (p == q) {
      if (tokens[p].type == NUM) {
        word_t num;
        num = atoi(tokens[p].str);
        return num;
      }
      else if (tokens[p].type == HEX) {
        word_t num;
        num = strtol(tokens[p].str, NULL, 16);
        return num;
      }
      else if (tokens[p].type == REG) {
        word_t num;
        bool success = true;
        num = isa_reg_str2val(tokens[p].str, &success);
        return num;
      }
    }
    else if (check(p, q) == 1) {
        return eval(p+1, q-1);
    }
    else if (p + 1 == q) {
      if (tokens[p].type == NEGTIVE) {
        return -eval(q, q);
      }
      else if (tokens[p].type == POINT) {
        return paddr_read(eval(q, q), 4);
      }
    }
    //292+6/(4*8)/1*3
    else {
        int op = -1;
        int count = 0; //括号数量
        int lock = 0; //操作符锁
        int lock2 = 0;
        for (int i = q; i >= p; i--) {
            // printf("%d\n", tokens[i].type);
            if (tokens[i].type == ')') count++;
            else if (tokens[i].type == '(') count--;  
            else if (count == 0) {
                if (tokens[i].type == TK_AND || tokens[i].type == TK_EQ || tokens[i].type == TK_NOEQ) {
                  op = i;
                  break;
                }
                //292+6/(4*8)/1*3
                else if (tokens[i].type == '+' || tokens[i].type == '-') {
                    if (lock2== 0) {
                      op = i;
                      lock2 = 1;
                      lock = 1;
                    } // 由于+和-的优先级最低所以为主操作符 遇到就直接break  
                }
                else if (tokens[i].type == '*' || tokens[i].type == '/') {// 如果为*或/
                    if (lock == 0) { 
                        op = i;
                        lock = 1; //从右到左优先级递增 所以右边的op为主操作符 所以赋值之后要上锁
                    }     
                }
            }
            // if (count == 1 && op != -1) break; 
        }
        
        word_t val1 = 0;
        word_t val2 = 0;
        if (op == -1) {
          if (tokens[p].type == POINT) {
            return paddr_read(eval(p+1, q), 4);
          }
          else if (tokens[p].type == NEGTIVE) {
            return -eval(p+1, q);
          }
        }
        else {
            val1 = eval(p, op-1);
            if (tokens[op+1].type == NEGTIVE) {
              val2 = eval(op+2, q);
              val2 = -val2;
            }
            else if (tokens[op+1].type == POINT) {
              val2 = eval(op+2, q);
              val2 = paddr_read(val2, 4);
            }
            else {
              val2 = eval(op+1, q);
            }
        }   
        // printf("%d\n", val1);
        // printf("%d\n", val2);

        switch (tokens[op].type) {
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/': return val1 / val2;
            case TK_EQ: return val1 == val2;
            case TK_NOEQ: return val1 != val2;
            case TK_AND: return val1 && val2;
            default: assert(0);
        }
    }
    printf("Expr error!\n");
    assert(0);
    return 0;
}


static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;
  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so==0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);
        // printf("Match found at offset %d to %d: ", pmatch.rm_so, pmatch.rm_eo-1);  
        // printf("%.*s\n", (int)(pmatch.rm_eo - pmatch.rm_so), e +position+ pmatch.rm_so);
        position += substr_len;
        

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        Token my_token;
        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;    
          default: 
            strncpy(my_token.str, substr_start, substr_len);
            my_token.str[substr_len] = '\0';
            my_token.type = rules[i].token_type;
            tokens[nr_token++] = my_token;
            break;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  return true;
}


word_t expr(char *e, bool *success) {
  // printf("%s\n", e);
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '*' && (i == 0 || (tokens[i-1].type != NUM && tokens[i-1].type != ')') )) {
      tokens[i].type = POINT;
    }
  }
  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '-' && (i == 0 || (tokens[i-1].type != NUM && tokens[i-1].type != ')')) ) {
      tokens[i].type = NEGTIVE;
    }
  }
  

  // for (int i = 0; i < nr_token; i++) {
  //   printf("%s\n", tokens[i].str);
  // }


  /* TODO: Insert codes to evaluate the expression. */
  // TODO();

  return eval(0, nr_token-1);
}
