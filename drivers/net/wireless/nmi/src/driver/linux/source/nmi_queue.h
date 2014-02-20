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
	@file			queue.h
	@description	generic QUEUE object with auto drop.
	@author			Austin Shin (austin.shin@wrg.co.kr)
	@date			2009-02-17
*/

#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#if 1
#define QUE_BUFFER_MAX_SIZE		((188*16*5)+1) //(188*16*8)//(188*30*4)//8192	
#else
#define QUE_BUFFER_MAX_SIZE		(188*30*8)//(188*30*4)
#endif

typedef struct _queue
{
	char* buffer;							// data buffer
	int	size;								// buffer size
	int rd, wr;								// r/w offset.
} queue;


int queue_init(queue* p, int len);
int queue_deinit(queue* p);
int queue_reset(queue* p);
int queue_data_size(queue* p);
int queue_free_size(queue* p);
int queue_write(queue* p, void* data, int len);
int queue_read(queue* p, void* data, int len);
int queue_drop(queue* p, int len);


#ifdef __cplusplus
}
#endif

#endif // _QUEUE_H_


