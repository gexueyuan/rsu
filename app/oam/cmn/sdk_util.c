/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : sdk_util.c
 @brief  : utility function
 @author : wanglei
 @history:
           2015-12-4    wanglei    Created file
           ...
******************************************************************************/
#include <stdio.h>
#include "sdk_util.h"

double __cv_hton_double(double data)
{
    double  data_dbl   = 1.982031;
    unsigned char *pu8 = (unsigned char *)&data_dbl;
    unsigned char temp_u8 = 0;
    int i;


    if (sizeof(double) != 8)
    {
        /* not support this type value in this compiler OK */
        return data;
    }

    if (pu8[0] == 0xE4 && pu8[1] == 0x87 && pu8[2] == 0x4A && pu8[3] == 0x23
        && pu8[4] == 0x66 && pu8[5] == 0xB6 && pu8[6] == 0xFF && pu8[7] == 0x3F)
    /* __LITTLE_ENDIAN__ */
    {
        data_dbl = data;

        for (i = 0; i < 4; ++i)
        {
            temp_u8 = pu8[i];
            pu8[i]  = pu8[7-i];
            pu8[7-i]= temp_u8;
        }
    }

    else if (pu8[0] == 0x3F && pu8[1] == 0xFF && pu8[2] == 0xB6 && pu8[3] == 0x66
        && pu8[4] == 0x23 && pu8[5] == 0x4A && pu8[6] == 0x87 && pu8[7] == 0xE4)
    /*  __BIG_ENDIAN__ */
    {
        return data;  /* do nothing */
    }

    else if (pu8[0] == 0x66 && pu8[1] == 0xB6 && pu8[2] == 0xFF && pu8[3] == 0x3F
        && pu8[4] == 0xE4 && pu8[5] == 0x87 && pu8[6] == 0x4A && pu8[7] == 0x23)

    /*  __MIDDLE_ENDIAN__ */
    {
        data_dbl = data;

        for (i = 0; i < 2; ++i)
        {
            temp_u8 = pu8[i];
            pu8[i]  = pu8[3-i];
            pu8[3-i]= temp_u8;
        }

        for (i = 4; i < 6; ++i)
        {
            temp_u8  = pu8[i];
            pu8[i]   = pu8[11-i];
            pu8[11-i]= temp_u8;
        }
    }

    return (data_dbl);

}


#ifndef  htonlf
#   define  htonlf(x)  __cv_hton_double(x)
#endif

#ifndef  ntohlf
#   define  ntohlf(x)  __cv_hton_double(x)
#endif

uint16_t cv_short_byte_reverse(uint16_t data, int8_t type)
{
    if(type == CV_SDK_DATA_HTON){
        return oam_htons(data);
    }else if (type == CV_SDK_DATA_NTOH){
        return oam_ntohs(data);
    }
    return data;
}

uint32_t cv_int_byte_reverse(uint32_t data, int8_t type)
{
    if(type == CV_SDK_DATA_HTON){
        return oam_htonl(data);
    }else if (type == CV_SDK_DATA_NTOH){
        return oam_ntohl(data);
    }
    return data;
}

uint64_t cv_long_byte_reverse(uint64_t data, int8_t type)
{
    if(type == CV_SDK_DATA_HTON){
        return oam_htonll(data);
    }else if (type == CV_SDK_DATA_NTOH){
        return oam_ntohll(data);
    }
    return data;
}

double double_byte_reverse(double data, int8_t type)
{
    if(type == CV_SDK_DATA_HTON){
        return htonlf(data);
    }else if (type == CV_SDK_DATA_NTOH){
        return ntohlf(data);
    }
    return data;
}


void cv_ipv6_byte_reverse(oam_ipv6_t ipv6, int8_t type)
{
    oam_uint32 i = 0;
    for(i = 0; i < 8; i++){
        ipv6[i] = cv_short_byte_reverse(ipv6[i],type);
    }
}
