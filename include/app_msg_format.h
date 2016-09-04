/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.

 @file   : app_msg_format.h
 @brief  : this file include the application variables and functions prototypes for
           the message format module.
 @author : wangxianwen
 @history:
           2016-1-21    wangxianwen    Created file
           2016-6-16	pengrui		   add some structure
           ...
******************************************************************************/

/* Define to prevent recursive inclusion. */
#ifndef _APP_MSG_FORMAT_H_
#define _APP_MSG_FORMAT_H_



/*
 * CAUTION:
 *    The sentence "declaration __attribute__ ((aligned(x)))" can not be working in this IDE.
 *    We can use this parameter to make the storage larger then the natural size; but we can
 *    not make the storage smaller than the natural size.
 * */

/* Save all the compiler settings. */
#pragma pack(push)

/* store data to reduce data size and off the optimization. */
#pragma pack(1)



/*************************************************
ö������
*************************************************/
/* ��Ϣ����ö��Message type enum*/
typedef enum _msg_type_em
{
    MSGTYPE_RESERVED        = 0x0,
    MSGTYPE_DEBUG     		= 0x1,
    MSGTYPE_SYS_MANAGE 		= 0x2,
    MSGTYPE_V2X_APPLY  		= 0x3,
    MSGTYPE_RAWDATA_DSRC 	= 0x4,
    MSGTYPE_RAWDATA_NMEA 	= 0x5
    
}MSG_TYPE_EM, *MSG_TYPE_EM_PTR;

#define MSG_TYPE_EM_LEN    (sizeof(MSG_TYPE_EM))


/* Message ID for 'MSGTYPE_V2X_APPLY'. */
typedef enum _msg_subtype_em
{
    MSGID_RESERVED              = 0x00,
    MSGID_NBNODE_INFO           = 0x01,
    MSGID_BASIC_VEHICLE_STATUS  = 0x02,
    MSGID_FULL_VEHICLE_STATUS   = 0x03,
    MSGID_VEHICLE_STATIC_INFO   = 0x04,
    MSGID_LCVEHICLE_ALERT_SET   = 0x05,

    MSGID_NBVEHICLE_ALERT       = 0x06,
    MSGID_ROADSIDE_ALERT        = 0x07

}V2X_APPLY_MSGID_EM, *V2X_APPLY_MSGID_EM_PTR;


/* �ڵ���Ϣö�� enum*/
typedef enum _node_infor_type_em
{
    NODE_INFOR_TYPE_SUMMARY = 0x01,       /* �ڵ��Ҫ��Ϣ */
    NODE_INFOR_TYPE_DETAIL  = 0x02        /* �ڵ���ϸ��Ϣ */
    
}NODE_INFOR_TYPE_EM, *NODE_INFOR_TYPE_EM_PTR;

#define NODE_INFOR_TYPE_EM_LEN    (sizeof(NODE_INFOR_TYPE_EM))

/* ������״̬enum*/
typedef enum _transmission_st_type_em
{
	TRANSMISSION_TYPE_NEUTRAL,
	TRANSMISSION_TYPE_PARK,
	TRANSMISSION_TYPE_FORWARDGEARS,
	TRANSMISSION_TYPE_REVERSEGEARS,
	TRANSMISSION_TYPE_RESERVED1,
	TRANSMISSION_TYPE_RESERVED2,
	TRANSMISSION_TYPE_RESERVED3,
	TRANSMISSION_TYPE_UNAVAILABLE
}TRANSMISSION_ST_TYPE_EM;
/* ǣ����״̬ö��enum*/
typedef enum _traction_control_info_type_em
{
	TRACTION_CONTROL_UNAVAILABLE,
	TRACTION_CONTROL_OFF,
	TRACTION_CONTROL_ON,
	TRACTION_CONTROL_ENGAGED
}TRACTION_CONTROL_INFO_TYPE_EM;
/* ABS������״̬ö��enum*/
typedef enum _anti_brake_st_type_em
{
	ANTI_BRAKE_UNAVAILABLE,
	ANTI_RRAKE_OFF,
	ANTI_BRAKE_ON,
	ANTI_BRAKE_ENGAGED
}ANTI_BRAKE_ST_TYPE_EM;
/* SC�ȶ�����״̬ö�� enum*/
typedef enum _stabile_control_st_type_em
{
	STABILE_CONTROL_UNAVAILABLE,
	STABILE_CONTROL_OFF,
	STABILE_CONTROL_ON,
	STABILE_CONTROL_ENGAGED
}STABILE_CONTROL_ST_TYPE_EM;
/* �ƶ�Ӧ��״̬ö�� enum*/
typedef enum _brake_boost_info_type_em
{
	BRAKE_BOOST_UNAVAILABLE,
	BRAKE_BOOST_OFF,
	BRAKE_BOOST_ON
}BRAKE_BOOST_INFO_TYPE_EM;
/*  �����ƶ�ϵͳ״̬ö�� enum*/
typedef enum _aux_brake_st_type_em
{
	/* ������*/
	AUX_BRAKE_UNAVAILABLE,
	/*��״̬*/
	AUX_BRAKE_OFF,
	/*��״̬*/
	AUX_BRAKE_ON,
	/*Ԥ��*/
	AUX_BRAKE_RESERVED
}AUX_BRAKE_ST_TYPE_EM;

/* vehicle gype enum*/
typedef enum _vehicle_type_em
{
	/* Not Equipped, Not known or unavailable*/
	VEHICLE_TYPE_NONE = 0,
	/* Does not fit any other category*/
	VEHICLE_TYPE_UNKONWN,
	/* Special use*/
	VEHICLE_TYPE_SPECIAL,
	/* Motorcycle*/
	VEHICLE_TYPE_MOTO,
	/* Passenger car*/
	VEHICLE_TYPE_CAR,
	/* Four tire single units*/
	VEHICLE_TYPE_CAR_OTHER,
	/* Buses*/
	VEHICLE_TYPE_BUS,
	/* Two axle, six tire single units*/
	VEHICLE_TYPE_AXLECNT2,
	/* Three axle, single units*/
	VEHICLE_TYPE_AXLECNT3,
	/* Four or more axle, single unit*/
	VEHICLE_TYPE_AXLECNT4,
	/* Four or less axle, single trailer*/
	VEHICLE_TYPE_AXLECNT4_TRAILER,
	/* Five or less axle, single trailer*/
	VEHICLE_TYPE_AXLECNT5_TRAILER,
	/* Six or more axle, single trailer*/
	VEHICLE_TYPE_AXLECNT6_TRAILER,
	/* Five or less axle, multi-trailer*/
	VEHICLE_TYPE_AXLECNT5_MULTITRALIER,
	/* Six axle, multi-trailer*/
	VEHICLE_TYPE_AXLECNT6_MULTITRAILER,
	/* Seven or more axle, multi-trailer*/
	VEHICLE_TYPE_AXLECNT7_MULTITRAILER
}VEHICLE_TYPE_EM;


/*************************************************
  �ṹ��
*************************************************/
/* 3Dλ��Position 3D structure. */
typedef struct  _position_3d_st
{
	/* γ��: unit 0.1 micro degree, (-900000000 - +900000001), 900000001 means invalid. */
    int32_t  latitude;                      /* γ�� */
    /* ����: unit 0.1 micro degree, (-1799999999 - 1800000001), 1800000001 means invalid. */
    int32_t longitude;                      /* ���� */
    /* ����: unit 0.1 meter, (-4096 - +61439), -4096 means invalid. */
    int32_t elevation;                      /* ���� */
    
}position_3d_st, *position3d_st_ptr;

#define POSITION3D_ST_LEN    (sizeof(position3d_st))

/* λ����Ϣ׼ȷ�� Position accurary structure. */
typedef struct _position_accu_st
{
	/* ������׼ȷ��: unit 5 cm, (0 - 255) [254 means >=254], 255 means invalid. */
	uint8_t			semimajoraxisAccu;//������׼ȷ��
	/* �̰���׼ȷ��: unit 5 cm, (0 - 255) [254 means >=254], 255 means invalid. */
	uint8_t			semiminoraxisAccu;//�̰���׼ȷ��
	/* �����᷽���: unit 0.0054932479 degree, (0-65534), 65535 means invalid. */
	uint16_t		semimajorAxisOrien;//�����᷽��
}position_accu_st,*position_accu_st_ptr;

#define POSITION_ACCURARY_LEN	sizeof(ehm_position_accu)

/* ���ٶ�accleration 4 way structure*/
typedef struct _accleration_set4way_st
{
	/* ������ٶ�(x): unit 0.01 m/s^2, (-2000 - +2000) [-2000 means <=-2000 +2000 means >=2000], 2001 means invalid. */
	int16_t			lonacce;//x���ٶ�
	/* ������ٶ�(y): unit 0.01 m/s^2, (-2000 - +2001) [-2000 means <=-2000 +2000 means >=2000], 2001 means invalid. */
	int16_t			latacce;//y���ٶ�
	/* ��ֱ���ٶ�(z): unit 0.02G(0.1962 m/s^2), (-127 - +127) [+127 means >= 2.54G -126 means <= -2.52G ], -127 means invalid. */
	int8_t			veracce;//��ֱ���ٶ�
	/* ƫ����: unit 0.01 degree/s, (-32767 - +32767). */
	int16_t			yawrate;//ƫ����
}accleration_set4way_st,*accleration_set4way_st_ptr;

#define ACCLERATION_SET4WAY_LEN	(sizeof(accleration_set4way_st))


/* ���������ƶ�״̬brake applied status structure*/
typedef union _wheel_brake_st
{
	uint8_t wheel_brake_word;

	struct _wheel_brake_bit{
		#ifndef __LITTLE_ENDIAN
		/*Ԥ��*/
			uint8_t	reserved:3;
		/*�Һ��� 	0:disactive		1:active ,�����������ֻ��һ������,���λ:0*/
			uint8_t rightrear:1;
		/*��ǰ��		0:active		1:active ,�����������ֻ��һ������,���λ:0*/
			uint8_t rightfront:1;
		/*�����		0:active		1:active ,�����������ֻ��һ������,��������Ӵ�����Ч*/
			uint8_t leftrear:1;
		/*��ǰ��		0:active		1:active ,�������ǰ��ֻ��һ������,��������Ӵ�����Ч*/
			uint8_t leftfront:1;
		/*������״̬	0:active		1:active ,�����1,���ʾ��ǰ�������Ӵ��ڲ�����״̬*/
			uint8_t unavailable:1;
		#else
		/*������״̬	0:active		1:active ,�����1,���ʾ��ǰ�������Ӵ��ڲ�����״̬*/
			uint8_t unavailable:1;
		/*��ǰ��		0:active		1:active ,�������ǰ��ֻ��һ������,��������Ӵ�����Ч*/
			uint8_t leftfront:1;
		/*�����		0:active		1:active ,�����������ֻ��һ������,��������Ӵ�����Ч*/
			uint8_t leftrear:1;
		/*��ǰ��		0:active		1:active ,�����������ֻ��һ������,���λ:0*/
			uint8_t rightfront:1;
		/*�Һ��� 	0:disactive		1:active ,�����������ֻ��һ������,���λ:0*/
			uint8_t rightrear:1;
		/*Ԥ��*/
			uint8_t	reserved:3;
		#endif
	} wheel_brake_bit;
}wheel_brake_st;


/* �ƶ�ϵͳ״̬ brake system status structure*/
typedef struct _brake_system_status_st
{
	/*�ƶ�Ӧ��״̬ */
	wheel_brake_st wheel_brakes;//�ƶ�Ӧ��״̬
	/*ǣ��������״̬  0:��Ч 	1:�� 	 2:��	3��ʹ����*/
	uint8_t		traction;//ǣ��������״̬
	/*ABS������״̬   0:��Ч 	1:�� 	 2:��	3��ʹ����*/
	uint8_t		abs;//ABS������״̬
	/*SC�ȶ�����״̬   0:��Ч 	1:�� 	 2:��	3��ʹ����*/
	uint8_t		scs;//SC�ȶ�����״̬
	/*�ƶ���ѹӦ��	0:��Ч 	1:�� 	 2:��	*/
	uint8_t		brakeboost;//�ƶ���ѹӦ��
	/*�����ƶ�״̬   0:��Ч 	1:�� 	 2:��	3��Ԥ��*/
	uint8_t		auxbrakes;//�����ƶ�״̬
}brake_system_status_st,*brake_system_status_st_ptr;

#define BRAKE_SYSTEM_STATUS_LEN	(sizeof(brake_system_status_st))


/* �ⲿ��״̬external lights structure*/
typedef union _exterior_lights_st
{
	uint16_t exterior_lights_word;
	struct _exterior_lights_bit
	{
	#ifndef __LITTLE_ENDIAN

		/* reserved*/
		uint16_t		reserved:7;
		/* ͣ��ָʾ�ƿ���*/
		uint16_t		parkinglight:1;
		/* ��ƿ���*/
		uint16_t		foglighton:1;
		/* �ռ��г��ƿ���*/
		uint16_t		daytimerunninglight:1;
		/* �Զ����ȵ��ڿ���*/
		uint16_t		automaticlight:1;
		/* Σ��ָʾ�ƿ���*/
		uint16_t		hazardsignallight:1;
		/* ��ת�źŵƿ���*/
		uint16_t		rightturnsignallight:1;
		/* ��ת�źŵƿ���*/
		uint16_t		leftturnsignallight:1;
		/* Զ��ƿ���*/
		uint16_t		highbeamheadlight:1;
		/* ����ƿ���*/
		uint16_t		lowbeamheadlight:1;

	#else

		/* ����ƿ���*/
		uint16_t		lowbeamheadlight:1;
		/* Զ��ƿ���*/
		uint16_t		highbeamheadlight:1;
		/* ��ת�źŵƿ���*/
		uint16_t		leftturnsignallight:1;
		/* ��ת�źŵƿ���*/
		uint16_t		rightturnsignallight:1;
		/* Σ��ָʾ�ƿ���*/
		uint16_t		hazardsignallight:1;
		/* �Զ����ȵ��ڿ���*/
		uint16_t		automaticlight:1;
		/* �ռ��г��ƿ���*/
		uint16_t		daytimerunninglight:1;
			/* ��ƿ���*/
		uint16_t		foglighton:1;
		/* ͣ��ָʾ�ƿ���*/
		uint16_t		parkinglight:1;
		/* reserved*/
		uint16_t		reserved:7;
	#endif
	}exterior_lights_bit;
}exterior_lights_st, * exterior_lights_st_ptr;


/* ������С�ṹ�� vehicle size structure*/
typedef struct _vehicle_size_st
{
	/* �������: unit 1 cm, (0 - 1023). */
	uint16_t		vehiclewidth;
	/* ��������: unit 1 cm, (0 - 4095). */
	uint16_t		vehiclelength;
}vehicle_size_st,*vehicle_size_st_ptr;

#define VEHICLE_SIZE_LEN	sizeof(vehicle_size_st);


/* �澯��ʶ Alert flag structure */
typedef union _alert_flag_st
{
    /* Alert word for whole group. */
    uint32_t alert_word;                  /* �澯λ��� */

    /* Alert bits. */
    struct _alert_bit
    {
	  #ifndef __LITTLE_ENDIAN
    	uint32_t reserved          :25;   /* ����λ */
		uint32_t rsd_tunnel        :1;    /* ����澯 */
		uint32_t vec_danger_goods  :1;    /* Σ�ջ������䳵�澯 */
		uint32_t vec_ambulance     :1;    /* �Ȼ����澯 */
		uint32_t vec_brake_hard    :1;    /* ����ɲ���澯 */
		uint32_t vec_breakdown     :1;    /* �������ϸ澯 */
		uint32_t vec_neardis_rear  :1;    /* �󳵽�����澯 */
		uint32_t vec_neardis_front :1;    /* ǰ��������澯 */
	  #else
    	uint32_t vec_neardis_front :1;    /* ǰ��������澯 */
		uint32_t vec_neardis_rear  :1;    /* �󳵽�����澯 */
		uint32_t vec_breakdown     :1;    /* �������ϸ澯 */
		uint32_t vec_brake_hard    :1;    /* ����ɲ���澯 */
		uint32_t vec_ambulance     :1;    /* �Ȼ����澯 */
		uint32_t vec_danger_goods  :1;    /* Σ�ջ������䳵�澯 */
		uint32_t rsd_tunnel        :1;    /* ����澯 */
		uint32_t reserved          :25;   /* ����λ */
	  #endif
    } alert_bit;
    
}alert_flag_st, *alert_flag_st_ptr;

#define ALERT_FLAG_ST_LEN    (sizeof(alert_flag_st))

/* Alert bit status. */
#define ALERT_FLAG_BIT_YES         0x01
#define ALERT_FLAG_BIT_NO          0x00
/* ���س����澯��ʶ*/
typedef union __vehicle_alert_st{
	/* ���س����澯��ʶ��*/
	uint32_t  vehicle_alert_words;
	/* ���س����澯��ʶλ*/
	struct _vehicle_alert_bit{
	#ifndef __LITTLE_ENDIAN

		uint32_t reserved             :28;

		/* ����ɲ���澯:00b(Invalid),01b(Off),10b(On),11b(Reserved)*/
		uint32_t vecbrakehardalert    :2;

		/* �������ϸ澯��00b(Invalid),01b(Off),10b(On),11b(Reserved)*/
		uint32_t	vecbreakdownalert :2;

	  #else

		/* �������ϸ澯��00b(Invalid),01b(Off),10b(On),11b(Reserved)*/
		uint32_t	vecbreakdownalert :2;

		/* ����ɲ���澯:00b(Invalid),01b(Off),10b(On),11b(Reserved)*/
		uint32_t vecbrakehardalert    :2;

		uint32_t reserved             :28;

	  #endif
	}vehicle_alert_bit;

}vehicle_alert_st;


/* �ڽڵ��Ҫ��Ϣneigbhour node summary information structure. */
typedef struct  _nb_node_summary_infor_st
{
	/* �ڵ�ID:(0,0,0,0)-invalid id */
    uint8_t       node_id[4];
    /* �������(�Ա���λ��Ϊ��׼): unit 0.01 m, (-32768 - +32767). */
    int16_t longitudinal_dis;
    /* �������(�Ա���λ��Ϊ��׼): unit 0.01 m, (-32768 - +32767). */
    int16_t  lateral_dis;
    /* �����ʻ����(�Ա�����ʻ����Ϊ��׼)��uint 0.0125 degree (0 - +28799) +28800 means invalid*/
    uint16_t	angle;
    /* ��Գ���(�Ա�������Ϊ��׼)��uint 0.02m/s (-8190 - +8190) 8191 means invalid*/
    int16_t	        velocity;
    /* �ź�ǿ��(�ھ�-����ͨ��):uint 1dBm (-127 - +126) [-127 means <=-127 +126 means >=+126] +127 means unavailable value*/
    int8_t		signalstrength;
    /* ������(�ھ�-����ͨ��):	uint 1%	[0 means 0% 100 means 100%] 101 means invalid */
    uint8_t		 losstolerance;

    /* �澯��� */
	alert_flag_st alert_flag;
   
}nb_node_summary_infor_st, *nb_node_summary_infor_st_ptr;

#define NB_NODE_SUMMARY_INFOR_ST_LEN	sizeof(nb_node_summary_infor_st)


/*�ڽڵ���ϸ��Ϣ*/
typedef struct _nb_node_detail_infor_st
{
	/* �ڵ�ID:(0,0,0,0)-invalid id */
	uint8_t       		node_id[4];
	/* 3Dλ�� */
	position_3d_st        pos3d;
	/* λ�þ�ȷ��*/
	position_accu_st 	posaccu;
	/* ������״̬: 0������״̬ 	1��פ��״̬	2:ǰ��״̬	3:����״̬	4:����	5:����	6:����	7:unavailable */
	uint8_t		     tran_state;//������״̬
	/* ����(Դ��GPSģ����߳�������)��uint 0.02m/s (0 - +8190) 8191 means invalid*/
	uint16_t	       velocity;
	/* ������ʻ����(��������Ϊ��׼)��uint 0.0125 degree (0 - +28799) +28800 means invalid*/
	uint16_t	          angle;
	/* ������ת��:uint 1.5 degrees (-126 - +126) [-126 means <= -189 deg +126 means >=+189 deg] +127 means unavaliable*/
	int8_t 	  steer_wheel_angle;
	/* ���ٶ�*/
	accleration_set4way_st  acc;
	/* �ƶ�ϵͳ״̬(���ǰ��Ϊ����,����ǰ��Ϊ�ο�;��� ����Ϊ����,�������Ϊ�ο�)*/
	brake_system_status_st   brake;
	/* �ⲿ�ƹ�״̬*/
	exterior_lights_st	exterlight;
	/* �澯��ʶ*/
	//�����ƣ�Ϊʹ�øñ�ʶ
	alert_flag_st			 alert_flag;
}nb_node_detail_infor_st,*nb_node_detail_infor_st_ptr;

#define NB_NODE_DETAIL_INFOR_ST_LEN    (sizeof(nb_node_detail_infor_st))


/* �ڽڵ���Ϣ���� */
typedef union _nb_node_infor_st
{
    nb_node_summary_infor_st summary_infor;
    nb_node_detail_infor_st   detail_infor;
    
}nb_node_infor_st, *nb_node_infor_st_ptr;




/* Message header structure ---------------------------------------------*/

/* ����֡ͷ��Ϣ�ṹ�� uart Message header structure */
typedef  struct  _uart_msg_header_st
{
	/* ����֡�ױ�ʶ��:	0x55. */
	uint8_t magic_num1;

	/* ����֡�ױ�ʶ��:	0xAA. */
	uint8_t magic_num2;

	/*֡����: DATA + CHK */
	uint16_t    length;
}uart_msg_header_st, *uart_msg_header_st_ptr;

#define UART_MSG_HEADER_ST_LEN         (sizeof(uart_msg_header_st))

/* Magic number for wnet main header. */
#define MSG_HEADER_MAGIC_NUM1          0x55
#define MSG_HEADER_MAGIC_NUM2          0xAA
#define	MSG_CHK_LEN					   0x02


/* Domain size in wnet header structrue. */
#define SIZEOF_MSG_HEADER_MAGIC_NUM1   (sizeof(((msg_header_st_ptr)0)->magic_num1))
#define SIZEOF_MSG_HEADER_MAGIC_NUM2   (sizeof(((msg_header_st_ptr)0)->magic_num2))
#define SIZEOF_MSG_HEADER_LENGTH       (sizeof(((msg_header_st_ptr)0)->length))

/* Domain size in message tail. */
#define SIZEOF_MSG_CHK_DOMAIN           (sizeof(uint16_t))

/* Domain offset in msg header structure. */
#define OFFSET_MSG_HEADER_MAGIC_NUM1   ((uint32_t)&(((msg_header_st_ptr)0)->magic_num1))
#define OFFSET_MSG_HEADER_MAGIC_NUM2   ((uint32_t)&(((msg_header_st_ptr)0)->magic_num2))
#define OFFSET_MSG_HEADER_LENGTH       ((uint32_t)&(((msg_header_st_ptr)0)->length))



/* Data Frames ------------------------------------------------------*/

/*֡��Ϣͷ��Ϣ  frame msg header structure*/
typedef  struct  _frame_msg_header_st
{
#ifndef __LITTLE_ENDIAN

	/*��Ϣ��ʶ��:	0xE*/
	uint8_t	mark:     4;

	/*��ϢԴ��	0:v2x	1:host*/
	uint8_t	src:      1;
    
	/*Ԥ���ֶΣ�	0*/
	uint8_t	reserved1:3;
	
	/*Ԥ���ֶΣ�	0*/
	uint8_t	reserved2:4;
    
	/*��Ϣ����:	0:Ԥ�� 1:������Ϣ 2:ϵͳ������Ϣ 3:v2xӦ����Ϣ 4:dsrc��Ϣ 5:gps��Ϣ 6:δ����*/
	uint8_t	type     :4;
	
#else

	/*Ԥ���ֶΣ�	0*/
	uint8_t	reserved1:3;

	/*��ϢԴ��	0:v2x	1:host*/
	uint8_t	src:      1;
    
	/*��Ϣ��ʶ��:	0xE*/
	uint8_t	mark:     4;
    	
	/*��Ϣ����:	0:Ԥ�� 1:������Ϣ 2:ϵͳ������Ϣ 3:v2xӦ����Ϣ 4:dsrc��Ϣ 5:gps��Ϣ 6:δ����*/
	uint8_t	type:     4;
    
	/*Ԥ���ֶΣ�	0*/
	uint8_t	reserved2:4;
	
#endif
}frame_msg_header_st, *frame_msg_header_st_ptr;

#define FRAME_MSG_HEADER_ST_LEN    (sizeof(frame_msg_header_st))

#define MSG_HEADER_MARK						0xE
#define MSG_SRC_V2X							0x0
#define MSG_SRC_HOST						0x1


/* Message Sets ----------------------------------------------------------*/

/* �ھӽڵ���Ϣ  msg neighbour node info structure */
typedef struct  _msg_vehicle_nb_status_st
{   
    /* Message id. */
    uint8_t                  msg_id;
    
    /* Message system time. */
    uint16_t            system_time;
    /*Node Number*/
    uint8_t				nodenumber;
    /* Set the neigbour's node information type. */
    uint8_t         	node_infor_type;

    /* �ڳ���Ҫ��Ϣ/�ڳ�������Ϣ N * (Node summary/detail information structure). (0 <= N) */ 
    /* nb_node_infor_st_ptr node_infor_ptr; */
    
}msg_vehicle_nb_status_st, *msg_vehicle_nb_status_st_ptr;

#define MSG_VEHICLE_NB_STATUS_ST_LEN    (sizeof(msg_vehicle_nb_status_st))


/* ��������״̬basic Vehicle status structure */
typedef struct _msg_basic_status_st
{
    /* Message id. */
    uint8_t              msg_id;

	/* �ڵ�ID:(0,0,0,0)-invalid id */
	uint8_t          node_id[4];
    
	/*3Dλ��*/
	position_3d_st 	   position;
    
	/* λ�þ�ȷ�� */
	position_accu_st	posaccu;
    
	/* �ٶ�: unit 0.02 m/s, (0 - 8191), 8191 means invalid. */
	uint16_t		   velocity;
	
	/* ��ʻ����: unit 0.0125 degree (�������˳ʱ��н�), (0 - 28800), 28800 means invalid. */
	uint16_t			  angle;
    
}msg_vehicle_basic_status_st, *msg_vehicle_basic_status_st_ptr;

#define MSG_VEHICLE_BASIC_STATUS_ST_LEN		(sizeof(msg_vehicle_basic_status_st))


/* ��������״̬ full Vehicle status structure */
typedef struct _msg_full_status_st
{
    /* Message id. */
    uint8_t                  msg_id;

	/* �ڵ�ID:(0,0,0,0)-invalid id */
	uint8_t       			node_id[4];
	/* 3Dλ��  */
	position_3d_st 			position;
	/* λ�þ�ȷ�� */
	position_accu_st		posaccu;
	/* ������״̬: 0������״̬ 	1��פ��״̬	2:ǰ��״̬	3:����״̬	4:����	5:����	6:����	7:unavailable */
	uint8_t		 			tran_state;
	/* �ٶ�: unit 0.02 m/s, (0 - 8191), 8191 means invalid. */
	uint16_t				velocity;
	/* ��ʻ����: unit 0.0125 degree(�������˳ʱ��н�), 	(0 - 28800), 28800 means invalid. */
	uint16_t				angle;
	/* ������ת��: unit 1.5 degree, (-126 - +127), 127 means invalid. */
	int8_t					steerwa;
	/* 4·���ٶȼ�   */
	accleration_set4way_st	acce4way;
	/* �ƶ�ϵͳ״̬  */
	brake_system_status_st	 braksta;
	/* �ⲿ�ƹ�״̬*/
	exterior_lights_st	  exterlight;


}msg_vehicle_full_status_st, *msg_vehicle_full_status_st_ptr;

#define	MSG_VEHICLE_FULL_STATUS_ST_LEN	(sizeof(msg_vehicle_full_status_st))


/* ������̬��Ϣ Vehicle static info structure */
typedef struct _vehicle_static_info_st
{
    /* Message id. */
    uint8_t                 msg_id;

	/* ��������*/
	uint8_t			  vehicle_type;
	/* �����ߴ�*/
	vehicle_size_st   vehicle_size;
    
}msg_vehicle_static_info_st, *msg_vehicle_static_info_st_ptr;

#define MSG_VEHICLE_STATIC_INFO_ST_LEN	sizeof(msg_vehicle_static_info_st)


/* ���س����澯���� local Vehicle alert set info structure */
typedef struct _msg_local_vehicle_alert_st
{
    /* Message id. */
    uint8_t                  msg_id;
    /*�����澯����*/
    vehicle_alert_st		vehicle_alert_set;

}msg_local_vehicle_alert_st,*msg_local_vehicle_alert_st_ptr;

#define MSG_LOCAL_VEHICLE_ALERT_ST_LEN	sizeof(msg_local_vehicle_alert_st)

/* Macro definition for vehicle alert set. */
#define VEHICLE_ALERT_INVALID    0x00
#define VEHICLE_ALERT_OFF        0x01
#define VEHICLE_ALERT_ON         0x02
#define VEHICLE_ALERT_RESERVED   0x03








/* �ڳ�Σ�ո澯 neighbour vehicle alert info structure*/
typedef struct _msg_nb_vehicle_alert_st
{
    /* Message id. */
    uint8_t                  msg_id;

	/* Message system time. */
    uint16_t            system_time;
    
	/*�ڽڵ�澯��Ϣ,��ѡ����*/
    //nb_node_summary_infor_st_ptr nb_node_ptr;
    
}msg_nb_vehicle_alert_st, *msg_nb_vehicle_alert_st_ptr;

#define	MSG_NB_VEHICLE_ALERT_LEN	sizeof(msg_nb_vehicle_alert_st)


/* ·����ʾ�澯 roadsize alert info structure*/
typedef struct _msg_roadsize_alert_info_st
{
    /* Message id. */
    uint8_t                  msg_id;

	uint8_t 				reserved;
}msg_roadsize_alert_info_st,*msg_roadsize_alert_info_st_ptr;

#define MSG_ROADSIZE_ALERT_INFO_ST_LEN	sizeof(msg_roadsize_alert_info_st)




/* restore all compiler settings in stacks. */
#pragma pack(pop)




#endif /* _APP_MSG_FORMAT_H_ */
