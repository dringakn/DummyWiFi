#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x4cf819e6, "module_layout" },
	{ 0x7d85d921, "unregister_netdev" },
	{ 0x3c12dfe, "cancel_work_sync" },
	{ 0x37a0cba, "kfree" },
	{ 0x65c93078, "wiphy_free" },
	{ 0xadf19c68, "wiphy_unregister" },
	{ 0xc7a32e1a, "free_netdev" },
	{ 0xdc4f5dc9, "register_netdev" },
	{ 0x5cd3a374, "alloc_netdev_mqs" },
	{ 0xf046a1db, "ether_setup" },
	{ 0xd5817d62, "wiphy_register" },
	{ 0x8d7eccf2, "wiphy_new_nm" },
	{ 0x4f00afd3, "kmem_cache_alloc_trace" },
	{ 0x696f0c3, "kmalloc_caches" },
	{ 0xcef62e03, "cfg80211_connect_done" },
	{ 0xeea1e14f, "cfg80211_scan_done" },
	{ 0xf9a482f9, "msleep" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x36d40d13, "cfg80211_put_bss" },
	{ 0x36b39fec, "cfg80211_inform_bss_data" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x2d3385d3, "system_wq" },
	{ 0x346cedcc, "kfree_skb_reason" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xcf2a6966, "up" },
	{ 0x2efe112e, "cfg80211_disconnected" },
	{ 0x6bd0e573, "down_interruptible" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "cfg80211");


MODULE_INFO(srcversion, "798E3BB1548C34236807E34");
