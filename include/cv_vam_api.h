/******************************************************************************
	��Ŀ����	��E600G�����ն���Ŀ
	�ļ�		��cv_vam_api.h
	����		��
	�汾		��0.1
	����		��pengrui
	��������	��2016-6-20
******************************************************************************/
#include "cv_vam.h"
#ifndef CV_VAM_API_H_
#define CV_VAM_API_H_
int32_t vam_get_config(vam_config_t *config);
int32_t vam_set_config(vam_config_t *config);
int32_t vam_active_rsa(E_VAM_RSA_TYPE rsaType);

#endif /* CV_VAM_API_H_ */
