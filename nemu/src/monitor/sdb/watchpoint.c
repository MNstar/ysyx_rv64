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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expression[100];
  int value;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }
  head = NULL; 
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */


WP *new_wp() {
  if (free_ == NULL) {
    printf("Watchpoint Pool is full!!!\n");
    assert(0);
  }

  //头插法
  WP *p = free_;
  free_ = free_ -> next; //p -> free_
  p -> next = head; // p -> head
  head = p; // 

  //尾插法 TODO()...
  return p;
}



void free_wp(WP *wp) {
  if(wp == head) {
    head = head->next;
  }
  else {
    WP *pos = head;
    while(pos && pos->next != wp) {
      pos++;
    }
    if (!pos) {
      printf("Input is not in Watchpoint Pool!!!\n");
      assert(0);
    }
    pos -> next = wp -> next;  
  }
  wp -> next = free_;
  free_ = wp;
}


void creat(char *args, int32_t res)
{
  WP* wp = new_wp();
  strcpy(wp->expression, args);
  wp->value=res;
  printf("Watchpoint %d: %s\n", wp->NO, wp->expression);
}
 
 
void removing(int no)
{
  if(no<0 || no>=NR_WP)
  {
    printf("N is not in right\n");
    assert(0);
  }
  WP* wp = &wp_pool[no];
  free_wp(wp);
  printf("Delete watchpoint %d: %s\n", wp->NO, wp->expression);
}
 


void info_watchpoint()
{
  WP *pos=head;
  if(!pos){
    printf("NO watchpoints\n");
    return;
  }
  printf("%s %s\n", "No", "Expression");
  while (pos) {
    printf("%d %s\n", pos->NO, pos->expression);
    pos = pos->next;
  }
}


void diff()
{
  WP* pos = head;
  while (pos) {
    bool success=true;
    int new = expr(pos->expression, &success);
    // printf("old_value\n", )
    if (pos->value != new) {
      Log("Watchpoint %d: %s\n"
        "Old value = %x\n"
        "New value = %x\n"
        , pos->NO, pos->expression, pos->value, new);
      // printf("%ld\n", pos->value);
      pos->value = new;
      nemu_state.state=NEMU_STOP;
      // assert(0);
    } else {
      Log("Watchpoint %d: %s\n"
            "Value = %x"
            , pos->NO, pos->expression, pos->value);
    }
    pos = pos->next;
  }
}






