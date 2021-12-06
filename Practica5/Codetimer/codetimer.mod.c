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
	{ 0x97934ecf, "del_timer_sync" },
	{ 0xbf1aa5c8, "remove_proc_entry" },
	{ 0xdb760f52, "__kfifo_free" },
	{ 0xdf9208c0, "alloc_workqueue" },
	{ 0x24d273d1, "add_timer" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xa4bf3758, "proc_create" },
	{ 0x139f2189, "__kfifo_alloc" },
	{ 0x999e8297, "vfree" },
	{ 0xe1537255, "__list_del_entry_valid" },
	{ 0x9166fada, "strncpy" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x56470118, "__warn_printk" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x9b77e97c, "try_module_get" },
	{ 0x4ae19672, "module_put" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x7a2af7b4, "cpu_number" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xf23fcb99, "__kfifo_in" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xd36dc10c, "get_random_u32" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xc5850110, "printk" },
	{ 0x13d0adf7, "__kfifo_out" },
	{ 0xa916b694, "strnlen" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");

