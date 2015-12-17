/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

/**
 * @addtogroup STM32
 */
/*@{*/


#include "cv_osal.h"
#define OSAL_MODULE_DEBUG
#define OSAL_MODULE_DEBUG_LEVEL OSAL_DEBUG_INFO
#define MODULE_NAME "INIT"
#include "cv_osal_dbg.h"
#include "cv_vam.h"
#include "cv_cms_def.h"


#define FIRMWARE_VERSION "V2.0.000" 
#ifdef RELEASE
#define FIRMWARE_IDEN "rel" 
#else
#define FIRMWARE_IDEN "dbg" 
#endif

extern void osal_dbg_init(void);
extern void param_init(void);
extern void gps_init(void);
extern void wnet_init(void);
extern void vam_init(void);
extern void vsa_init(void);
extern void sys_init(void);
extern void mda_init(void);
cms_global_t cms_envar, *p_cms_envar;

extern void cv_oam_init();

static uint8_t device_eletronic_signature[12];

void des_init(void)
{
    #define DES_BASE 0x1FFF7A10

    volatile uint32_t *des_reg = (uint32_t *)DES_BASE;

    memcpy(&device_eletronic_signature[0], (void *)des_reg, 4);
    memcpy(&device_eletronic_signature[4], (void *)(des_reg+1), 4);
    memcpy(&device_eletronic_signature[8], (void *)(des_reg+2), 4);
}

uint8_t des(int offset)
{
    return device_eletronic_signature[offset];
}


void global_init(void)
{
    p_cms_envar = &cms_envar;
    memset(p_cms_envar, 0, sizeof(cms_global_t));
}

int main(int argc, char *argv[])
{
//    system("./hostapd -d /etc/hostapd.conf -B");
    global_init();
    param_init();
// 	nmea_init();
    wnet_init();
    vam_init();
    mda_init();
//    vsa_init();    
    sys_init();

    vam_start();
//    vsa_start();
    gps_init();

    cv_oam_init();
    
    while (1){
        osal_sleep(10000);
    }
}

void get_version(void)
{
  osal_printf("\nFirm: %s[%s,%s %s]\n\n", FIRMWARE_VERSION, FIRMWARE_IDEN, __TIME__, __DATE__);
}

