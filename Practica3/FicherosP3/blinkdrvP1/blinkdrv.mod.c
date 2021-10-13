#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

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
	{ 0x7a9aeab2, "module_layout" },
	{ 0xb9891fe4, "usb_deregister" },
	{ 0xb04d0ff8, "usb_register_driver" },
	{ 0xa9ff3e2b, "usb_deregister_dev" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0x56470118, "__warn_printk" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x85df9b6c, "strsep" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x9166fada, "strncpy" },
	{ 0x37a0cba, "kfree" },
	{ 0x2d6ec9ab, "usb_control_msg" },
	{ 0x5af3e222, "kmem_cache_alloc_trace" },
	{ 0x512ffe5c, "kmalloc_caches" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xc5850110, "printk" },
	{ 0x19075cf2, "usb_find_interface" },
	{ 0x296695f, "refcount_warn_saturate" },
	{ 0xecc693a1, "_dev_info" },
	{ 0xdb427ebd, "_dev_err" },
	{ 0x4081716a, "usb_register_dev" },
	{ 0x3faebdd0, "usb_get_dev" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0x999e8297, "vfree" },
	{ 0xc56d959f, "usb_put_dev" },
	{ 0xfb384d37, "kasprintf" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "usbcore");

MODULE_ALIAS("usb:v20A0p41E5d*dc*dsc*dp*ic*isc*ip*in*");
