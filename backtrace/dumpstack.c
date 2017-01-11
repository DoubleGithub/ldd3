#include <linux/module.h>
#include <linux/stacktrace.h> /* for dump_stack */
#include <linux/sched.h> /* for task_struct */
#include <linux/completion.h> /* for completion */
#include <linux/interrupt.h> /* for tasklet */

static void test_normal(void)
{
  pr_info("print a call stack from a process context: Program name: %s, PID: %d\n", current->comm, current->pid);
  dump_stack();
}

static DECLARE_COMPLETION(backtrace_work);

static void tasklet_irq_callback(unsigned long data)
{
  dump_stack();
  complete(&backtrace_work);
}

static DECLARE_TASKLET(backtrace_tasklet, &tasklet_irq_callback, 0);

static void test_irq(void)
{
  pr_info("print a call stack from an interrupt context: Program name: %s, PID: %d\n", current->comm, current->pid);
  init_completion(&backtrace_work);
  tasklet_schedule(&backtrace_tasklet);
  wait_for_completion(&backtrace_work);
}

static int dumpstack_init(void)
{
  test_normal();
  test_irq();
  return 0;
}

static void dumpstack_exit(void)
{
  
}

module_init(dumpstack_init);
module_exit(dumpstack_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fuyajun1983cn@163.com");
