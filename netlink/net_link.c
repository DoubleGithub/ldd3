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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <net/netlink.h>

#define NETLINK_TEST 31

struct sock *nl_sk = NULL;
EXPORT_SYMBOL_GPL(nl_sk);

void nl_data_ready (struct sk_buff *__skb)
{
  struct sk_buff *skb;
  struct nlmsghdr *nlh;
  u32 pid;
  int rc;
  int len = NLMSG_SPACE(1200);
  char str[100];

  printk("net_link: data is ready to read.\n");
  skb = skb_get(__skb);

  if (skb->len >= NLMSG_SPACE(0)) {
    nlh = nlmsg_hdr(skb);
    printk("net_link: recv %s.\n", (char *)NLMSG_DATA(nlh));
    memcpy(str,NLMSG_DATA(nlh), sizeof(str));
    pid = nlh->nlmsg_pid; /*pid of sending process */
    printk("net_link: pid is %d\n", pid);
    kfree_skb(skb);

    skb = alloc_skb(len, GFP_ATOMIC);
    if (!skb){
      printk(KERN_ERR "net_link: allocate failed.\n");
      return;
    }
    nlh = nlmsg_put(skb,0,0,0,1200,0);
    NETLINK_CB(skb).portid = 0; /* from kernel */

    memcpy(NLMSG_DATA(nlh), str, sizeof(str));
    printk("net_link: going to send.\n");
    rc = netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);
    if (rc < 0) {
      printk(KERN_ERR "net_link: can not unicast skb (%d)\n", rc);
    }
    printk("net_link: send is ok.\n");
  }
  
}
 
 static struct netlink_kernel_cfg cfg = {
  .input	=  nl_data_ready, 
    };


 static int test_netlink(void) {
  nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST, &cfg );

  if (!nl_sk) {
  printk(KERN_ERR "net_link: Cannot create netlink socket.\n");
  return -EIO;
}
  printk("net_link: create socket ok.\n");
  return 0;
}

 int init_module()
 {
  test_netlink();
  return 0;
}
 void cleanup_module( )
 {
  if (nl_sk != NULL){
  sock_release(nl_sk->sk_socket);
}
  printk("net_link: remove ok.\n");
}
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("kidoln");
