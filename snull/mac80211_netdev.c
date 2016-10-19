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

struct mac80211_netdev_data {
  struct ieee80211_hw *hw;
  struct device *dev;
};

static const struct ieee80211_ops mac80211_netdev_ops = {
  .tx = netdev_tx,
  .start = netdev_start,
  .stop = netdev_stop,
  .config = netdev_config,
  .add_interface = netdev_add_interface,
  .remove_interface = netdev_remove_interface,
  .configure_filter = netdev_configure_filter,
  .sta_state = netdev_sta_state,
  .sta_add = netdev_sta_add,
  .sta_remove = netdev_sta_remove,
};

static int __init init_mac80211_netdev(void)
{
  int err;
  u8 addr[ETH_ALEN];
  struct mac80211_netdev_data *data;
  struct ieee80211_hw *hw;
  const struct ieee80211_ops *ops = &mac80211_netdev_ops;

  hw = ieee80211_alloc_hw_nm(sizeof(*data), ops, NULL/*use the default name*/);
  if (!hw) {
    printk(KERN_DEBUG "mac80211_netdev: ieee80211_alloc_hw failed\n");
    err = -ENOMEM;
    goto failed;
  }
  data = hw->priv;
  data->hw = hw;

  /***************hw设置*******************/

  /****************************************/
  err = ieee80211_register_hw(hw);/* 在此函数中，通过调用ieee80211_if_add创建了无线网络设备接口*/
  if (err < 0) {
    printk(KERN_DEBUG "mac80211_hwsim: ieee80211_register_hw failed (%d)\n",err);
    goto failed_hw;
  }
  
  return 0;

failed_drvdata:
	ieee80211_free_hw(hw);
failed:
	return err;
}
module_init(init_mac80211_netdev);

static void __exit exit_mac80211_netdev(void)
{
  
}
module_exit(exit_mac80211_netdev);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yajun Fu<fuyajun1983cn@163.com");
