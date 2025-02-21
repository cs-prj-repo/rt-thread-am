#include <am.h>
#include <klib.h>
#include <rtthread.h>

typedef struct {
  uintptr_t entry, parameter, exit;
} ContextArgs;

typedef struct {
  rt_ubase_t from, to;
  rt_ubase_t backup;
} SwitchParameter;
 
static Context* ev_handler(Event e, Context *c) {
  rt_thread_t current = rt_thread_self();
  SwitchParameter *sparam = NULL;
  Context **from = NULL;
  Context **to = NULL;

  switch (e.event) {
    case EVENT_YIELD:
      sparam = (SwitchParameter *)current->user_data;
      from = (Context **)sparam->from;
      to = (Context **)sparam->to;

      if (from)
        *from = c;
      c = *to;
      current->user_data = sparam->backup;
      break;
    case EVENT_IRQ_TIMER:
    case EVENT_IRQ_IODEV:
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

static void rt_exec_wrapper(void *arg) {
  ContextArgs *args = arg;
  void (*entry)(void *) = (void *)args->entry;
  void (*exit)() = (void *)args->exit;

  entry((void *)args->parameter);
  exit();
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  rt_thread_t current = rt_thread_self();
  SwitchParameter sparam = (SwitchParameter){ .from = from, .to = to, .backup = current->user_data };
  current->user_data = (rt_base_t)&sparam;
  yield();
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  rt_hw_context_switch((rt_ubase_t)NULL, to);
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  uintptr_t align_addr = ((uintptr_t)stack_addr / sizeof(uintptr_t)) * sizeof(uintptr_t);
  uintptr_t context_addr = align_addr - sizeof(Context);
  uintptr_t exec_args_addr = context_addr - sizeof(ContextArgs);

  ContextArgs *args = (ContextArgs *)exec_args_addr;
  args->exit = (uintptr_t)texit;
  args->entry = (uintptr_t)tentry;
  args->parameter = (uintptr_t)parameter;

  Context *ct = kcontext((Area) { NULL, (void *)align_addr }, rt_exec_wrapper, args);

  return (rt_uint8_t *)ct;
}
