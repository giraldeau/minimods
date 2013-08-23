#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/stacktrace.h>
#include <linux/uaccess.h>
#include <linux/pagemap.h>
#include <linux/sched.h>

#define PROCFS_NAME "minimod"
#define MAX_ENTRIES 10
#define MAX_ADDRESSES 1000

void (*save_func)(struct stack_trace *trace);

static struct proc_dir_entry *pent;

static unsigned long buf[MAX_ENTRIES];

static void guess_stack_trace(void)
{
	const struct pt_regs *regs = task_pt_regs(current);
	const void __user *sp = (const void __user *)regs->sp;
	const void __user *top = (const void __user *)current->mm->start_stack;
	unsigned long addr;
	unsigned long i;
	unsigned long data;
	struct vm_area_struct *vma;

	printk("minimod: ip=0x%lx\n", regs->ip);
	printk("minimod: sp=0x%lx top=0x%lx size=%lu\n",
			(unsigned long)sp, (unsigned long)top, ((unsigned long)top - (unsigned long)sp));
	printk("minimod: current->mm->stack_vm=0x%lx 0x%lu\n", current->mm->stack_vm, current->mm->stack_vm << 12);

	if (sp < (top - (current->mm->stack_vm << 12)) || sp > top) {
		printk("minimod: stack pointer outside mm\n");
		return;
	}

	printk("%-18s %-18s\n", "Address", "Content");
	for (i = 0; i < MAX_ADDRESSES; i++) {
		int is_exec = 0;
		addr = (unsigned long) sp + i * sizeof(unsigned long);
		if (__copy_from_user_inatomic(&data, (void *)addr, sizeof(data)))
			break;
		vma = find_vma(current->mm, data);
		if (vma && (vma->vm_flags & VM_EXEC) &&
				(vma->vm_start <= data) &&
				(data <= vma->vm_end))
			is_exec = 1;
		//if (is_exec)
		printk("0x%016lx 0x%016lx %d\n", addr, data, is_exec);
	}
}

//ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
ssize_t write_callback(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	int i;
	struct stack_trace trace;
	trace.nr_entries = 0;
	trace.max_entries = MAX_ENTRIES;
	trace.entries = buf;
	save_func(&trace);
	for (i = 0; i < trace.nr_entries; i++)
		printk("0x%lx ", trace.entries[i]);
	printk("\n");
	guess_stack_trace();
	return count;
}

static const struct file_operations fops = {
		.owner = THIS_MODULE,
		.write = write_callback,
};

static int __init minimod_init(void)
{
	save_func = (void *)kallsyms_lookup_name("save_stack_trace_user");
	if (!save_func)
		return -ENOMEM;

	pent = proc_create(PROCFS_NAME, 0666, NULL, &fops);
	if (!pent)
		return -ENOMEM;

	return 0;
}

static void __exit minimod_exit(void)
{
	// proc_remove(pent); // not compatible with 3.8
	remove_proc_entry(PROCFS_NAME, NULL);
	return;
}

module_init(minimod_init);
module_exit(minimod_exit);

MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Francis Giraldeau <francis.giraldeau@gmail.com>");
MODULE_DESCRIPTION("debug");
