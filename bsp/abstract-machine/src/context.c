#include <am.h>
#include <klib.h>
#include <rtthread.h>
// typedef struct Context Context;
// Context *kcontext(Area kstack, void (*entry)(void *), void *arg);

typedef struct {
    void (*tentry)(void *);   // tentry 函数指针
    void *parameter;          // 参数
    void (*texit)(void);      // texit 函数指针
} context_params_t;

static Context* ev_handler(Event e, Context *c) {
  rt_thread_t current = rt_thread_self();
  Context** from = NULL;
  switch (e.event) {
    case EVENT_YIELD:
      from = (Context**)(((rt_ubase_t*)(current->user_data))[1]);
      Context** to = (Context**)(((rt_ubase_t*)(current->user_data))[0]);
      if(from != 0){
        *from = c;
      }
      c = *to;
      break; 
    case EVENT_IRQ_TIMER: 
    break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

// rt_hw_context_switch_to()用于切换到to指向的上下文指针变量所指向的上下文
void rt_hw_context_switch_to(rt_ubase_t to) {
  // assert(0);
  rt_thread_t current = rt_thread_self();
  rt_ubase_t from_to[2] = {to, 0};
  rt_ubase_t user_data_bnk = current->user_data;
  current->user_data = (rt_ubase_t)from_to;
  yield();
  current->user_data = user_data_bnk;
}                                                                   

// rt_hw_context_switch()还需要额外将当前上下文的指针写入from指向的上下文指针变量中
// rt_ubase_t类型其实是unsigned long, to和from都是指向上下文指针变量的指针(二级指针)
void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  // assert(0);
  rt_thread_t current = rt_thread_self();
  rt_ubase_t from_to[2] = {to, from};
  rt_ubase_t user_data_bnk = current->user_data;
  current->user_data = (rt_ubase_t)from_to;
  yield();
  current->user_data = user_data_bnk;
}

// 目前RT-Thread的运行过程不会调用它, 因此目前可以忽略它
void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

void wrapper_func(void *args){
  // assert(0);
  void (*tentry)(void *);   // tentry 函数指针
  // void *parameter;          // 参数
  void (*texit)(void);      // texit 函数指针

  uintptr_t *args_addr = ((uintptr_t *)args) - 3;
  tentry = (void(*)(void *))((uintptr_t)(args_addr[2]));
  texit = (void(*)(void))((uintptr_t)(args_addr[0]));
  tentry((void *)((uintptr_t)(args_addr[1])));
  texit();
  // (void(*)(void *))(args_addr[0])(args_addr[1]);
  // (void(*)(void))(args_addr[2])();
  // uintptr_t *stack_end = rt_thread_self()->sp;
  // // uintptr_t stack_end = (uintptr_t)((Context *)stack_end - 1);
  // printf("stack_end = 0x%08x\n", *(rt_uint32_t *)stack_end);
  // *((rt_uint32_t *)--stack_end) = (uintptr_t)tentry;
  // printf("stack_end = 0x%08x\n", *(rt_uint32_t *)stack_end);
  // *((rt_uint32_t *)--stack_end) = (uintptr_t)parameter;
  // printf("stack_end = 0x%08x\n", *(rt_uint32_t *)stack_end);
  // *((rt_uint32_t *)--stack_end) = (uintptr_t)texit;
  // printf("stack_end = 0x%08x\n", *(rt_uint32_t *)stack_end);
  // texit();
}

// 功能是以stack_addr为栈底创建一个入口为tentry 参数为parameter的上下文
rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  // assert(0);
  uintptr_t stack_end = RT_ALIGN((uintptr_t)stack_addr,sizeof(uintptr_t));
  uintptr_t stack_start = (uintptr_t)((char*)stack_end - FINSH_THREAD_STACK_SIZE);
  // printf("stack_start = 0x%08x\n",stack_start);
  // printf("stack_end = 0x%08x\n",stack_end);

  Area area = { (void *)stack_start, (void *)stack_end};
  Context* con = kcontext(area, wrapper_func, (void *)((Context *)stack_end - 1));
  
  uintptr_t *args_addr = ((uintptr_t *)((Context *)stack_end - 1)) - 3;
  args_addr[0] = ((uintptr_t)texit);
  args_addr[1] = ((uintptr_t)parameter);
  args_addr[2] = ((uintptr_t)tentry);

  // for(int i = 0; i < 4; i ++){
  //   printf("stack[%d] = 0x%08x\n", i,args_addr[i]);
  // }
  return (rt_uint8_t *)con;
}
