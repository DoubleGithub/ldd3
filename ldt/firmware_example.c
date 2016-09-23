/**
 * Copyright (c)  2016     Yajun Fu (fuyajun1983cn@163.com)
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/firmware.h>
#include <linux/slab.h>     /* kfree */

static void
sample_firmware_load(const u8* firmware, int size)
{
  u8 *buf = kmalloc(size + 1, GFP_KERNEL);
  memcpy(buf, firmware, size);
  buf[size] = '\0';
  printk(KERN_INFO "firmware_example: Firmware: %s\n", buf);
  kfree(buf);
}

static void
sample_probe(struct device *dev)
{
  /* use the default method to get the firmware */
  const struct firmware *fw_entry;
  printk(KERN_INFO "firmware_example: ghost device inserted\n");

  if (request_firmware(&fw_entry, "sample_firmware.bin", dev) != 0) {
    printk(KERN_INFO "firmware_example: Firmware not available\n");
    return;
  }

  sample_firmware_load(fw_entry->data, fw_entry->size);

  /* finish setting up the device */
}

static void
ghost_release(struct device *dev)
{
  printk(KERN_DEBUG "firmware_example: ghost_device released\n");
}

static struct device ghost_device = {
  .init_name = "ghost0",
  .release = ghost_release
};

static int __init sample_init(void)
{
  device_register(&ghost_device);
  sample_probe(&ghost_device);
  return 0;
}

static void __exit sample_exit(void)
{
  device_unregister(&ghost_device);
}

module_init(sample_init);
module_exit(sample_exit);

MODULE_LICENSE("GPL");
