/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : osal_queue.c
 @brief  : mqueue api adapt
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/
#include "osal_queue.h"

osal_queue_t *osal_queue_create(const char *qname, uint32_t max_msgs, uint32_t msg_size)
{
    int ret = 0;
    osal_queue_t * queue;
    osal_mqd_t mqd;
    queue = (osal_queue_t *)os_malloc(sizeof(osal_queue_t));
    if (queue == NULL)
        return NULL;
    memset(queue, 0, sizeof(osal_queue_t));

    /* flags = 0, block */
    ret = os_queue_create(&mqd, qname, max_msgs, msg_size, 0);
    if (ret){    
        os_free(queue);
        return NULL;
    }
    queue->mq_des = mqd;
    queue->msg_size = msg_size;
    return queue;
}

osal_status_t osal_queue_delete(osal_queue_t * queue)
{
    int ret = 0;
    if (queue == NULL){
        return OSAL_STATUS_EMPTY;
    }
    ret = os_queue_delete(queue->mq_des);
    os_free(queue);
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_ERROR_UNDEFINED;
}

osal_status_t osal_queue_recv(osal_queue_t *queue, void *data, uint32_t *len, int timeout)
{
    int ret = 0;
    if (queue == NULL){
        return OSAL_STATUS_EMPTY;
    }
    /* timeout -1:  block */
    if (-1 == timeout) {
        ret = os_queue_recv(queue->mq_des, data, queue->msg_size, len);
        ret = (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS : OSAL_STATUS_ERROR_UNDEFINED;
    }
    else{
        ret = os_mq_timedrecv(queue->mq_des, data, queue->msg_size, timeout);
        if (ret > 0) {
            *len  = ret;
            ret = OSAL_STATUS_SUCCESS;
        }
        else {
            ret = OSAL_STATUS_TIMEOUT;
        }
    }
    return ret;
}

/* when msg queue is full,  return after timeout */
osal_status_t osal_queue_send(osal_queue_t *queue, void *data, uint32_t len, uint32_t priority, int timeout)
{
    int ret = 0;
    if (queue == NULL || data == NULL){
        return OSAL_STATUS_EMPTY;
    }
    if (-1 == timeout) {
        ret = os_queue_send(queue->mq_des, data, len, priority);
    }
    else{
        ret = os_mq_timedsend(queue->mq_des, data, len, priority, timeout);    
    }
    return (ret == OSAL_EOK) ? OSAL_STATUS_SUCCESS: OSAL_STATUS_TIMEOUT;
}


