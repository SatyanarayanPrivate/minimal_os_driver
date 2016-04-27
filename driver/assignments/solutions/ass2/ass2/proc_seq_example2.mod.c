#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x67730ce8, "module_layout" },
	{ 0x85fd75fb, "seq_release" },
	{ 0xb5685e93, "seq_read" },
	{ 0x2d06208, "seq_lseek" },
	{ 0x8353b73e, "remove_proc_entry" },
	{ 0xeacb8e84, "create_proc_entry" },
	{ 0x37a0cba, "kfree" },
	{ 0x789bdca3, "seq_printf" },
	{ 0x91715312, "sprintf" },
	{ 0x8afa47cb, "kmem_cache_alloc_trace" },
	{ 0x3458763f, "kmalloc_caches" },
	{ 0xdc4f651f, "seq_open" },
	{ 0xdc901d8a, "init_task" },
	{ 0x27e1a049, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "15F95C914E8678E432AD5D3");
