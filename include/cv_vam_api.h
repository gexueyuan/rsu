/******************************************************************************
	项目名称	：E600G车载终端项目
	文件		：cv_vam_api.h
	描述		：
	版本		：0.1
	作者		：pengrui
	创建日期	：2016-6-20
******************************************************************************/
#include "cv_vam.h"
#ifndef CV_VAM_API_H_
#define CV_VAM_API_H_
int32_t vam_get_config(vam_config_t *config);
int32_t vam_set_config(vam_config_t *config);
int32_t vam_active_rsa(E_VAM_RSA_TYPE rsaType);

#endif /* CV_VAM_API_H_ */
