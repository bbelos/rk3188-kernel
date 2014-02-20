/*
	Copyright (c) Newport Media Inc.  All rights reserved.
	Copyright (c) WRG Inc.  All rights reserved.

	Use of this sample source code is subject to the terms of the Newport Media Inc.
	license agreement under which you licensed this sample source code. If you did not
	accept the terms of the license agreement, you are not authorized to use this
	sample source code. For the terms of the license, please see the license agreement
	between you and Newport Media Inc.
	THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
*/
/**
	@file			queue.c
	@description	generic QUEUE object with auto drop.
	@author			Austin Shin (austin.shin@wrg.co.kr)
	@date			2009-02-17
*/

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <android/log.h> //added 

#include "nmi_queue.h"

#ifdef _MSC_VER
#define inline __inline
#endif

#include <linux/spinlock.h>
#include <linux/slab.h>

static spinlock_t g_lock;

/* read write option */
#define OPTION_PARTIAL_READ			// enable partial read
//#define OPTION_PARTIAL_WRITE		// enable partial write
#define OPTION_AUTO_DROP			// force write with auto dropping.


/* debugging */
#define ASSERT(x)
//#define ASSERT(x)		(!(x)) ? assert(x);

static inline int __align(int v, unsigned int n)
{
	return (v + (n-1)) & ~(n-1);
}

int queue_init(queue* p, int len)
{
	ASSERT(p);
	ASSERT(p->buffer);
	ASSERT(len > 0);

	spin_lock_init(&g_lock);

	//p->buffer = malloc(len);
	p->buffer = kmalloc(len, GFP_KERNEL);

	if ( p->buffer == NULL )
	{
		return 0;
	}

	p->size = len;

	queue_reset(p);

	return len;
}

int queue_deinit(queue* p)
{
	if( p->buffer == NULL )
		return -1;
	
	queue_reset(p);

	kfree(p->buffer); p->buffer = NULL;

	return 0;
}

int queue_reset(queue* p)
{
	ASSERT(p);
	ASSERT(p->buffer);
	ASSERT(p->size > 0);

	if(p->buffer == NULL)
		return 0;

	spin_lock(&g_lock);

	memset(p->buffer, 0, p->size);
	p->wr = 0;
	p->rd = 0;

	spin_unlock(&g_lock);

	return 0;
}

int queue_data_size(queue* p)
{
	ASSERT(p);

	if(p->wr >= p->rd)
		return (p->wr - p->rd);
	else
		return (p->wr + (p->size - p->rd));
}

int queue_free_size(queue* p)
{
	ASSERT(p);

	return p->size - queue_data_size(p);
}

int queue_drop(queue* p, int len)
{
	ASSERT(p);
	ASSERT(len < p->size);

	if(p->rd + len < p->size)
	{
		p->rd += len;
	}
	else
	{
		p->rd = p->rd + len - p->size;
	}

	return len;	
}

int queue_write(queue* p, void* data, int len)
{
	int freesize;

	ASSERT(p);
	ASSERT(data);
	ASSERT(len < p->size);

	if(p->buffer == NULL)
		return 0;

	spin_lock(&g_lock);

	freesize = queue_free_size(p);

	if(len > freesize)
	{
#if defined(OPTION_PARTIAL_WRITE)
		len = freesize;
#elif defined(OPTION_AUTO_DROP)
		queue_drop(p, __align(len - freesize, 1)+1);
#else
		spin_unlock(&g_lock);
		return 0;
#endif
	}

	if(p->wr + len < p->size)
	{
		memcpy(p->buffer + p->wr, data, len);
		p->wr += len;
	}
	else
	{
		int part1 = p->size - p->wr;
		int part2 = len - part1;

		memcpy(p->buffer + p->wr, data, part1);
		memcpy(p->buffer, (char*)(data) + part1, part2);
		p->wr = part2;

	}

	spin_unlock(&g_lock);

	return len;
}


int queue_read(queue* p, void* data, int len)
{
	int datasize;

	ASSERT(p);
	ASSERT(data);
	ASSERT(len < p->size);

	if(p->buffer == NULL)
		return 0;

	spin_lock(&g_lock);

	datasize = queue_data_size(p);

	if (len > datasize)
	{
		//printk("queue underrun\n");
#if defined(OPTION_PARTIAL_READ)
		len = datasize;
#else
		spin_unlock(&g_lock);

		return 0;
#endif
	}

	if(p->rd + len < p->size)
	{
		memcpy(data, p->buffer + p->rd, len);
		p->rd += len;
	}
	else
	{
		int part1 = p->size - p->rd;
		int part2 = len - part1;

		memcpy(data, p->buffer + p->rd, part1);
		memcpy((char*)(data) + part1, p->buffer, part2);
		p->rd = part2;
	}

	spin_unlock(&g_lock);

	return len;	
}


