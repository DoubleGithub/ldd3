/**
 * Copyright (c)  2017     Yajun Fu (fuyajun1983cn@163.com)
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 *  proc_create_data(...)接口使用实例
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/sched.h> //for current
#include <linux/vmalloc.h>

/* dummy test structure */
struct priv_data {
  int data;
};

//Entries for /proc/gdl and /proc/gdl/memory
static struct proc_dir_entry *mm_proc_mem;
static struct proc_dir_entry *mm_proc_dir;

//only output some info from kernel

static int procfs_test4_show(struct seq_file *m, void *v)
{
  struct priv_data *data = (struct priv_data*)m->private;
  if (data != NULL) {
    seq_printf(m, "Get Private Data: %d\n", data->data);
    vfree(data);
  } else
    seq_printf(m, "NO Private Data\n");
  return 0;
}

static int procfs_test4_open(struct inode *inode, struct file *file)
{
  
  return single_open(file, procfs_test4_show, PDE_DATA(inode));
}

static const struct file_operations procfs_test4_fops = {
  .open           = procfs_test4_open,
  .read           = seq_read,
  .llseek         = seq_lseek,
  .release        = seq_release,
};

static int __init procfs_test4_init(void)
{
  struct priv_data *data = (struct priv_data*)vmalloc(sizeof(*data));
  memset(data, 0, sizeof(struct priv_data));
  data->data = 1;
  mm_proc_dir = 0;
  mm_proc_mem = 0;

  //create a directory under /proc
  mm_proc_dir = proc_mkdir("gdl", 0);
  if (mm_proc_dir == 0) {
    printk(KERN_ERR "/proc/gdl/ creation failed\n");
    return -1;
  }

  //create /proc/gdl/memory file
  mm_proc_mem = proc_create_data("memory", S_IFREG|S_IRWXU|S_IRWXG|S_IRWXO, mm_proc_dir, &procfs_test4_fops, data);
  if (mm_proc_mem == 0) {
    printk(KERN_ERR "/proc/gdl/memory creation failed\n");
    proc_remove(mm_proc_dir);
    mm_proc_dir = 0;
    return -1;
  }

  return 0;
}

static void __exit procfs_test4_exit(void)
{
  if (mm_proc_dir != 0)
    {
      if (mm_proc_mem != 0)
        {
          proc_remove(mm_proc_mem);
          mm_proc_mem = 0;
        }

      proc_remove(mm_proc_dir);
      mm_proc_dir = 0;
    }
}
module_init(procfs_test4_init);
module_exit(procfs_test4_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yajun Fu<fuyajun1983cn@163.com");

