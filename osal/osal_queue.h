/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : osal_queue.h
 @brief  : osal_queue.c header file
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/

#ifndef _OSAL_QUEUE_H_
#define _OSAL_QUEUE_H_


#include "os_core.h"
#include "osal_cmn.h"


typedef struct _osal_queue {
    osal_mqd_t mq_des;
    uint32_t msg_size;
} osal_queue_t;

osal_queue_t *osal_queue_create(const char *qname, uint32_t qdepth, uint32_t data_size);
osal_status_t osal_queue_delete(osal_queue_t * queue);
osal_status_t osal_queue_recv(osal_queue_t *queue, void *data, uint32_t *len, int timeout);
osal_status_t osal_queue_send(osal_queue_t *queue, void *data, uint32_t len, uint32_t priority, int timeout);

#endif
