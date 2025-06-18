#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
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
	{ 0x92997ed8, "_printk" },
	{ 0x9ed554b3, "unregister_keyboard_notifier" },
	{ 0x29d9f26e, "cancel_delayed_work_sync" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x7fffffff, "SSD1306_Write" },
	{ 0x99bb8806, "memmove" },
	{ 0x3e82f3e9, "get_random_bytes" },
	{ 0x49cd25ed, "alloc_workqueue" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xcf86cdac, "queue_delayed_work_on" },
	{ 0x96554810, "register_keyboard_notifier" },
	{ 0xc84d16dc, "module_layout" },
};

MODULE_INFO(depends, "I2CDriver");


MODULE_INFO(srcversion, "5BE64B3498BDAB9AC48C689");
