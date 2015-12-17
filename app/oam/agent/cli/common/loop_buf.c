/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : loop_buf.c
 @brief  : 循环缓冲区
 @author : wanglei
 @history:
           2015-10-29    wanglei    Created file
           ...
******************************************************************************/

#include <pthread.h>
#include <string.h>
#include "loop_buf.h"


/* 初始化循环缓冲区 */
int init_loop_buf(loop_buf_t *buf)
{
    memset(buf->buffer, 0, LOOP_BUF_LEN);
    buf->in = buf->out = 0;
    pthread_mutex_init(&buf->mutex,NULL);
    return 0;
}

/**缓冲区已经填充的元素个数*/
unsigned int loop_buf_sz(loop_buf_t *buf)
{
    return (buf->in-buf->out+LOOP_BUF_LEN)%LOOP_BUF_LEN;
}

/* 返回循环缓冲区剩余长度 */
unsigned int loop_buf_remain(loop_buf_t *buf)
{
    return (buf->out + LOOP_BUF_LEN-1 - buf->in)%LOOP_BUF_LEN;
}

/**缓冲区是否满了*/
int loop_buf_isfull(loop_buf_t *buf)
{
    return ((buf->in+1)%LOOP_BUF_LEN == buf->out);
}
/**缓冲区是不是空的*/
int loop_buf_isempty(loop_buf_t *buf)
{
    return buf->out == buf->in;
}

/* 循环缓冲区指针复位 */
void loop_buf_reset(loop_buf_t *buf)
{
    pthread_mutex_lock(&buf->mutex);
    buf->in = buf->out = 0;
    pthread_mutex_unlock(&buf->mutex);
    return;
}

/**向缓冲区加入buf数据，长度为len.成功true,失败false*/
int loop_buf_push_n(loop_buf_t *buf, unsigned char *p, unsigned int len)
{
    unsigned i = 0;

    pthread_mutex_lock(&buf->mutex);

    if(loop_buf_remain(buf) < len)
    {
        pthread_mutex_unlock(&buf->mutex);
        return 0;
    }

    for(i=0; i<len; i++)
    {
        buf->buffer[buf->in] = p[i];
        buf->in = (buf->in+1)%LOOP_BUF_LEN;
    }

    pthread_mutex_unlock(&buf->mutex);
    
    return i;
}

/**拿出len个，赋值给P*/
int loop_buf_pop_n(loop_buf_t *buf, unsigned char *p, unsigned int len)
{
    unsigned int i = 0;

    if(p==NULL || len<=0)
    {
        return 0;
    }

    pthread_mutex_lock(&buf->mutex);

    len = min(len, loop_buf_sz(buf));

    for(i=0; i<len; i++)
    {
        p[i] = buf->buffer[buf->out];
        buf->out = (buf->out+1)%LOOP_BUF_LEN;
        if ('\r' == p[i])
        {
            ++i;
            break;
        }
    }
    pthread_mutex_unlock(&buf->mutex);

    return i;
}
/* 释放循环缓冲区里的线程锁 */
void fini_loop_buf(loop_buf_t *buf)
{
    pthread_mutex_destroy(&buf->mutex);
}
