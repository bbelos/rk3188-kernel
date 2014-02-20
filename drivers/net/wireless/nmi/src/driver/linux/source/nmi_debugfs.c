/*
* NewportMedia WiFi chipset driver test tools - nmi-debug
* Copyright (c) 2012 NewportMedia Inc.
* Author: SSW <sswd@nmisemic.com> 
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/

#if defined(NMC_DEBUGFS)
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include "nmi_queue.h"
#include "nmi_wlan_if.h"


static struct dentry *nmc_dir;

/*
--------------------------------------------------------------------------------
*/

// [[ linux_wlan_common.h
#define DBG_REGION_ALL	(GENERIC_DBG | HOSTAPD_DBG | HOSTINF_DBG | CORECONFIG_DBG | CFG80211_DBG | INT_DBG | TX_DBG | RX_DBG | LOCK_DBG | INIT_DBG | BUS_DBG | MEM_DBG)

#define DBG_LEVEL_ALL	(DEBUG | INFO | WRN | ERR)

atomic_t REGION = ATOMIC_INIT(INIT_DBG | GENERIC_DBG | CFG80211_DBG);

// [[ debug level: DEBUG, INFO, WRN, added by tony 2012-01-09
atomic_t DEBUG_LEVEL = ATOMIC_INIT(DEBUG | ERR); // if DEBUG_LEVEL is zero, silent, if DEBUG_LEVEL is 0xFFFFFFFF, verbose
// ]]

// debug_kmsg
queue g_dumpqueue = {NULL, };
#define DUMP_QUEUE_MAX (8*1024)
DECLARE_WAIT_QUEUE_HEAD(nmc_msgdump_waitqueue);


/*
--------------------------------------------------------------------------------
*/
static int nmc_debug_level_open(struct inode *inode, struct file *file)
{
	/*DEBUG_LEVEL = *((unsigned int *)inode->i_private);*/
	printk("nmc_debug_level_open: current debug-level=%x\n", atomic_read(&DEBUG_LEVEL));
	return 0;
}

static int nmc_debug_region_open(struct inode *inode, struct file *file)
{
	/*REGION = *((unsigned int *)inode->i_private);*/
	printk("nmc_debug_region_open: current debug-region=%x\n", atomic_read(&REGION));
	return 0;
}

static int nmc_debug_dump_open(struct inode *inode, struct file *file)
{
	printk("nmc_debug_dump_open\n");
	return 0;
}

static ssize_t nmc_debug_level_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
	if(copy_to_user(buf, (const void*)(&atomic_read(&DEBUG_LEVEL)), sizeof(unsigned int))) {
		return -EFAULT;
	}

	return sizeof(unsigned int);
}

static ssize_t nmc_debug_level_write(struct file *filp, const char *buf, size_t count, loff_t *ppos)
{
	unsigned int flag = 0;

	if(copy_from_user(&flag, buf, sizeof(unsigned int))) {
		return -EFAULT;
	}
	
	if(flag > DBG_LEVEL_ALL) {
		printk("%s, value (0x%08x) is out of range, stay previous flag (0x%08x)\n", __func__, flag, atomic_read(&DEBUG_LEVEL));
		return -EFAULT;
	}

	atomic_set(&DEBUG_LEVEL, (int)flag);
	printk("new debug-level is %x\n", atomic_read(&DEBUG_LEVEL));
	
	return sizeof(unsigned int);
}

static ssize_t nmc_debug_region_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
	if(copy_to_user(buf, (const void*)(&atomic_read(&REGION)), sizeof(unsigned int))) {
		return -EFAULT;
	}

	return sizeof(unsigned int);
}

static ssize_t nmc_debug_region_write(struct file *filp, const char *buf, size_t count, loff_t *ppos)
{
	uint32_t flag = 0;

	if(copy_from_user(&flag, buf, sizeof(unsigned int))) {
		return -EFAULT;
	}
	
	if(flag > DBG_REGION_ALL) {
		printk("%s, value (0x%08x) is out of range, stay previous flag (0x%08x)\n", __func__, flag, atomic_read(&REGION));
		return -EFAULT;
	}

	atomic_set(&REGION, (int)flag);
	printk("new debug-region is %x\n", atomic_read(&REGION));

	return sizeof(unsigned int);
}

void kmsgdump_write(char *fmt, ...)
{
	char buf[256] = {0,};
	va_list args;
	int len;

	va_start(args, fmt);
	len = vsprintf(buf, fmt, args);
	va_end(args);

	if((len>=256) || (len<0))
		buf[256-1] = '\n';

#if !defined (NM73131_0_BOARD)
//#define LOG_TIME_STAMP //NM73131 is not support : rachel check required
#endif

#ifdef LOG_TIME_STAMP
	{
		char buf_t[30] = {0,};
		unsigned long rem_nsec;
		u64 ts = local_clock();
		
		rem_nsec = do_div(ts, 1000000000);
		sprintf(buf_t, "[%5lu.%06lu] ", (unsigned long)ts, rem_nsec / 1000);

		queue_write(&g_dumpqueue, (void*)buf_t, strlen(buf_t));
	}
#endif

	queue_write(&g_dumpqueue, (void*)buf, strlen(buf));
	wake_up(&nmc_msgdump_waitqueue);

}

static ssize_t kmsgdump_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
	int read_size = 0;
	int buf_size = queue_data_size(&g_dumpqueue);
	
	if(buf_size > 0)
	{
		unsigned char *text = kmalloc(buf_size, GFP_KERNEL);
		read_size = queue_read(&g_dumpqueue, (void*)text, (int)buf_size);
		if(copy_to_user(buf, text, read_size)) {
			kfree(text);
			return -EFAULT;
		}
		kfree(text);
		return read_size;
	}
	// return -EAGAIN;
	return 0;
}

static unsigned int kmsgdump_poll(struct file *file, poll_table *wait)
{
	if(queue_data_size(&g_dumpqueue))
		return POLLIN | POLLRDNORM;
	poll_wait(file, &nmc_msgdump_waitqueue, wait);
	if(queue_data_size(&g_dumpqueue))
		return POLLIN | POLLRDNORM;
	return 0;
}


/*
--------------------------------------------------------------------------------
*/

#define FOPS(_open, _read, _write, _poll) { \
	.owner	= THIS_MODULE, \
	.open	= (_open), \
	.read	= (_read), \
	.write	= (_write), \
	.poll		= (_poll), \
}

struct nmc_debugfs_info_t {
	const char *name;
	int perm;
	unsigned int data;
	struct file_operations fops;
};

static struct nmc_debugfs_info_t debugfs_info[] = {
	{ "nmc_debug_level",	0666,	(DEBUG | ERR), FOPS(nmc_debug_level_open, nmc_debug_level_read, nmc_debug_level_write,NULL), },
	{ "nmc_debug_region",	0666,	(INIT_DBG | GENERIC_DBG | CFG80211_DBG),	FOPS(nmc_debug_region_open, nmc_debug_region_read, nmc_debug_region_write, NULL), },
	{ "nmc_debug_dump",		0666,	0,				FOPS(nmc_debug_dump_open, kmsgdump_read, NULL, kmsgdump_poll), },
};

int nmc_debugfs_init(void)
{
	int i;

	struct dentry *debugfs_files;
	struct nmc_debugfs_info_t *info;

	nmc_dir = debugfs_create_dir("nmi_wifi", NULL);
	if(nmc_dir ==  ERR_PTR(-ENODEV)) {
		/* it's not error. the debugfs is just not being enabled. */
		printk("ERR, kernel has built without debugfs support\n");
		return 0;
	}

	if(!nmc_dir) {
		printk("ERR, debugfs create dir\n");
		return -1;
	}
	
	for(i=0; i<ARRAY_SIZE(debugfs_info) ; i++)
	{
		info = &debugfs_info[i];
		printk("create the debugfs file, %s\n", info->name);
		debugfs_files = debugfs_create_file(info->name,
							info->perm,
							nmc_dir,
							&info->data,
							&info->fops);
		
		if(!debugfs_files) {
			printk("fail to create the debugfs file, %s\n", info->name);
			debugfs_remove_recursive(nmc_dir);	
			return -1;
		}
	}

	queue_init(&g_dumpqueue, DUMP_QUEUE_MAX);

	return 0;
}

void nmc_debugfs_remove(void)
{
	queue_deinit(&g_dumpqueue);

	debugfs_remove_recursive(nmc_dir);
}

#endif

