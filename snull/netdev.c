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
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/spinlock.h>

#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/rtnetlink.h> /* rtnl_lock() */
#include <linux/if_arp.h>  /* ARPHRD_IEEE80211_RADIOTAP */

MODULE_AUTHOR("Yajun Fu");
MODULE_LICENSE("Dual BSD/GPL");

/* netdevice for mon */
static struct net_device *netdev_mon;

static netdev_tx_t netdev_mon_xmit(struct sk_buff *skb,
                                  struct net_device *dev)
{
  dev_kfree_skb(skb);
  return NETDEV_TX_OK;
}

static const struct net_device_ops netdev_ops = {
  .ndo_start_xmit = netdev_mon_xmit,
  .ndo_change_mtu = eth_change_mtu,
  .ndo_set_mac_address = eth_mac_addr,
  .ndo_validate_addr = eth_validate_addr,
};

  static void netdev_mon_setup(struct net_device *dev)
  {
    printk(KERN_DEBUG "setup netdev...\n");
    dev->netdev_ops = &netdev_ops;
    dev->destructor = free_netdev;
    ether_setup(dev);
    dev->tx_queue_len = 0;
    dev->type = ARPHRD_IEEE80211_RADIOTAP;
    memset(dev->dev_addr, 0, ETH_ALEN);
    dev->dev_addr[0] = 0x12;
  }


static int __init module_enter(void)
{
  int ret = -ENOMEM;

  netdev_mon = alloc_netdev(0, "test%d", NET_NAME_UNKNOWN, netdev_mon_setup);
  if (netdev_mon == NULL) {
    ret = -ENOMEM;
    goto out;
  }

  rtnl_lock();
  ret = register_netdevice(netdev_mon);
  if (ret < 0) {
    rtnl_lock();
    goto out_free_netdev;
  }
  rtnl_unlock();

  return 0;

 out_free_netdev:
  free_netdev(netdev_mon);
 out:
  printk("module init failed\n");
  return ret;
}

static void __exit module_leave(void)
{
  printk(KERN_DEBUG "netdev: unregister virtual netdevice");
  unregister_netdev(netdev_mon);
  return;
}

module_init(module_enter);
module_exit(module_leave);
