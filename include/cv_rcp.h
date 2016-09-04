/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_rcp.h
 @brief  : this file include the definitions of  the remote communicate 
           protocol of vehicle
 @author : wangyifeng
 @history:
           2014-6-22    wangyifeng    Created file
           2015-1-20    wanglei       Modified. add dsrc msg
           ...
******************************************************************************/
#ifndef __CV_RCP_H__
#define __CV_RCP_H__

#include "cv_wnet.h"

/*****************************************************************************
 * definition of micro                                                       *
*****************************************************************************/
#define  __COMPILE_INLINE__  inline
/*****************************************************************************
 * definition of structs                                                     *
*****************************************************************************/
/* Data Element: DE_DSRC_MessageID. */
typedef enum _DSRC_MSG_ID 
{
    /* All DER forms are now retired and not to be used. */
	DSRCmsgID_reservedMessageId_D	                  = 0,
	DSRCmsgID_alaCarteMessage_D	          	= 1, /* ACM */
	DSRCmsgID_basicSafetyMessage_D	      	= 2, /* BSM, heartbeat msg */
	DSRCmsgID_basicSafetyMessageVerbose_D   = 3, /* used for testing only */
	DSRCmsgID_commonSafetyRequest_D	      	= 4,
	DSRCmsgID_emergencyVehicleAlert_D	    = 5,
	DSRCmsgID_intersectionCollision_D       = 6,
	DSRCmsgID_mapData_D	                  	= 7, /* MAP, GID, intersections */
	DSRCmsgID_nmeaCorrections_D	          	= 8,
	DSRCmsgID_probeDataManagement_D	      	= 9,
	DSRCmsgID_probeVehicleData_D	        = 10,
	DSRCmsgID_roadSideAlert_D	            = 11,
	DSRCmsgID_rtcmCorrections_D             = 12,
	DSRCmsgID_signalPhaseAndTimingMessage_D = 13,
	DSRCmsgID_signalRequestMessage_D	    = 14,
	DSRCmsgID_signalStatusMessage_D	      	= 15,
	DSRCmsgID_travelerInformation_D	      	= 16,
	DSRCmsgID_uperFrame_D                   = 17,

    /* UPER forms. */
    DSRCmsgID_mapData	                  = 18, /* MAP, intersections */
    DSRCmsgID_signalPhaseAndTimingMessage = 19, /* SPAT */
	DSRCmsgID_basicSafetyMessage	      = 20, /* BSM, heartbeat msg */
	DSRCmsgID_commonSafetyRequest	      = 21, /* CSR */
	DSRCmsgID_emergencyVehicleAlert	      = 22, /* EVA */
	DSRCmsgID_intersectionCollision       = 23, /* ICA */
	DSRCmsgID_nmeaCorrections	          = 24, /* NMEA */
	DSRCmsgID_probeDataManagement	      = 25, /* PDM */
	DSRCmsgID_probeVehicleData	          = 26, /* PVD */
	DSRCmsgID_roadSideAlert	              = 27, /* RSA */
	DSRCmsgID_rtcmCorrections             = 28, /* TRCM */
	DSRCmsgID_signalRequestMessage	      = 29, /* SRM */
	DSRCmsgID_signalStatusMessage	      = 30, /* SSM */
	DSRCmsgID_travelerInformation	      = 31, /* TIM */
    DSRCmsgID_personalSafetyMessage       = 32  /* PSM */

    /* Enumeration is extensible. */
    
} E_DSRC_MSG_ID;


/* Part 2 id for message. */
typedef enum _MSG_PART2_ID 
{
    MsgPart2Id_vehicleSafetyExt	        = 0,
	MsgPart2Id_specialVehicleExt	    = 1,
	MsgPart2Id_supplementalVehicleExt	= 2
	
    /* Enumeration is extensible. */
    
} E_MSG_PART2_ID;

/* Data Element: DE_TransmissionState. */
typedef enum _E_TRANSMISSION_STATE 
{
    /* All DER forms are now retired and not to be used. */
    TRANSMISSION_STAT_NEUTRAL	        = 0,
	TRANSMISSION_STAT_PARK	            = 1, 
	TRANSMISSION_STAT_FORWARD_GEARS	    = 2, 
	TRANSMISSION_STAT_REVERSE_GEARS     = 3,
	TRANSMISSION_STAT_RESERVED1	        = 4,
	TRANSMISSION_STAT_RESERVED2	        = 5,
	TRANSMISSION_STAT_RESERVED3         = 6,
	TRANSMISSION_STAT_UNAVAILABLE	    = 7

} E_TRANSMISSION_STATE;


/* Data Element: DE_VehicleType. */
typedef enum _VehicleType 
{
	VehicleType_none	= 0,
	VehicleType_unknown	= 1,
	VehicleType_special	= 2,
	VehicleType_moto	= 3,
	VehicleType_car	    = 4,
	VehicleType_carOther	= 5,
	VehicleType_bus     	= 6,
	VehicleType_axleCnt2	= 7,
	VehicleType_axleCnt3	= 8,
	VehicleType_axleCnt4	= 9,
	VehicleType_axleCnt4Trailer	        = 10,
	VehicleType_axleCnt5Trailer     	= 11,
	VehicleType_axleCnt6Trailer	        = 12,
	VehicleType_axleCnt5MultiTrailer	= 13,
	VehicleType_axleCnt6MultiTrailer	= 14,
	VehicleType_axleCnt7MultiTrailer	= 15
} E_VehicleType;


/* Data Element: DE_ResponseType. */
typedef enum _ResponseType 
{
	ResponseType_notInUseOrNotEquipped	= 0,
	ResponseType_emergency	= 1,
	ResponseType_nonEmergency	= 2,
	ResponseType_pursuit	= 3
} E_ResponseType;


typedef enum _VehicleGroupAffected {
	VehicleGroupAffected_all_vehicles	= 9217,
	VehicleGroupAffected_bicycles	= 9218,
	VehicleGroupAffected_motorcycles	= 9219,
	VehicleGroupAffected_cars	= 9220,
	VehicleGroupAffected_light_vehicles	= 9221,
	VehicleGroupAffected_cars_and_light_vehicles	= 9222,
	VehicleGroupAffected_cars_with_trailers	= 9223,
	VehicleGroupAffected_cars_with_recreational_trailers	= 9224,
	VehicleGroupAffected_vehicles_with_trailers	= 9225,
	VehicleGroupAffected_heavy_vehicles	= 9226,
	VehicleGroupAffected_trucks	= 9227,
	VehicleGroupAffected_buses	= 9228,
	VehicleGroupAffected_articulated_buses	= 9229,
	VehicleGroupAffected_school_buses	= 9230,
	VehicleGroupAffected_vehicles_with_semi_trailers	= 9231,
	VehicleGroupAffected_vehicles_with_double_trailers	= 9232,
	VehicleGroupAffected_high_profile_vehicles	= 9233,
	VehicleGroupAffected_wide_vehicles	= 9234,
	VehicleGroupAffected_long_vehicles	= 9235,
	VehicleGroupAffected_hazardous_loads	= 9236,
	VehicleGroupAffected_exceptional_loads	= 9237,
	VehicleGroupAffected_abnormal_loads	= 9238,
	VehicleGroupAffected_convoys	= 9239,
	VehicleGroupAffected_maintenance_vehicles	= 9240,
	VehicleGroupAffected_delivery_vehicles	= 9241,
	VehicleGroupAffected_vehicles_with_even_numbered_license_plates	= 9242,
	VehicleGroupAffected_vehicles_with_odd_numbered_license_plates	= 9243,
	VehicleGroupAffected_vehicles_with_parking_permits	= 9244,
	VehicleGroupAffected_vehicles_with_catalytic_converters	= 9245,
	VehicleGroupAffected_vehicles_without_catalytic_converters	= 9246,
	VehicleGroupAffected_gas_powered_vehicles	= 9247,
	VehicleGroupAffected_diesel_powered_vehicles	= 9248,
	VehicleGroupAffected_lPG_vehicles	= 9249,
	VehicleGroupAffected_military_convoys	= 9250,
	VehicleGroupAffected_military_vehicles	= 9251
} E_VehicleGroupAffected;

typedef enum _ResponderGroupAffected {
    /* Default phrase, to be used when one of the below does not fit better */
	ResponderGroupAffected_emergency_vehicle_units	        = 9729,
	ResponderGroupAffected_federal_law_enforcement_units	= 9730,
	ResponderGroupAffected_state_police_units	            = 9731,
	ResponderGroupAffected_county_police_units	            = 9732,
	ResponderGroupAffected_local_police_units	            = 9733,
	ResponderGroupAffected_ambulance_units	                = 9734,
	ResponderGroupAffected_rescue_units	                    = 9735,
	ResponderGroupAffected_fire_units                   	= 9736,
	ResponderGroupAffected_hAZMAT_units	                    = 9737,
	ResponderGroupAffected_light_tow_unit               	= 9738,
	ResponderGroupAffected_heavy_tow_unit               	= 9739,
	ResponderGroupAffected_freeway_service_patrols         	= 9740,
	ResponderGroupAffected_transportation_response_units	= 9741,
	ResponderGroupAffected_private_contractor_response_units	= 9742
} E_ResponderGroupAffected;


typedef enum _Extent {
	Extent_useInstantlyOnly	= 0,
	Extent_useFor3meters	= 1,
	Extent_useFor10meters	= 2,
	Extent_useFor50meters	= 3,
	Extent_useFor100meters	= 4,
	Extent_useFor500meters	= 5,
	Extent_useFor1000meters	= 6,
	Extent_useFor5000meters	= 7,
	Extent_useFor10000meters	= 8,
	Extent_useFor50000meters	= 9,
	Extent_useFor100000meters	= 10,
	Extent_forever	= 127
} E_Extent;

/* vehicle event flags. */
typedef enum _VehicleEventFlags 
{
    EventHazardLights               = 0x0001,
    EventStopLineViolation          = 0x0002, /* Intersection Violation */  
    EventABSactivated               = 0x0004,
    EventTractionControlLoss        = 0x0008,
    
    EventStabilityControlactivated  = 0x0010,
    EventHazardousMaterials         = 0x0020,    
    EventReserved1                  = 0x0040,
    EventHardBraking                = 0x0080,
    
    EventLightsChanged              = 0x0100,
    EventWipersChanged              = 0x0200,
    EventFlatTire                   = 0x0400,
    EventDisabledVehicle            = 0x0800,
    
    EventAirBagDeployment           = 0x1000
    
} E_VehicleEventFlags;

typedef enum _TransmissionState {
    TRANS_STATE_neutral   = 0, /* Neutral, speed relative to the vehicle alignment */
    TRANS_STATE_park      = 1, /* Park */
    TRANS_STATE_Forward   = 2, /* Forward gears */
    TRANS_STATE_Reverse   = 3, /* Reverse gears */
    TRANS_STATE_reserved1 = 4, 
    TRANS_STATE_reserved2 = 5, 
    TRANS_STATE_reserved3 = 6, 
    TRANS_STATE_unavailable = 7,         
} E_TransmissionState;

/* Status word for some control module. */
typedef enum _STATUS_WORD 
{
    STATUS_WORD_NOT_EQUIPPED = 0,
    STATUS_WORD_OFF          = 1,
    STATUS_WORD_ON           = 2,
    STATUS_WORD_ENGAGED      = 3
    
} E_STATUS_WORD;

typedef uint16_t itis_codes_t;
typedef uint16_t heading_slice_t;

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
    
typedef struct _rcp_msgid 
{
#ifndef __LITTLE_ENDIAN
    uint16_t     hops: 3;
    uint16_t       id: 5;
    uint16_t reserved: 8;
#else
    uint16_t       id: 5;
    uint16_t     hops: 3;
    uint16_t reserved :8;
#endif
}rcp_msgid_t;

/* Position accuracy used to model the accuracy of each given axis. */
typedef struct _rcp_position_accuracy
{
    /* Semi-major axis accuracy. */
    uint8_t              semi_major;

    /* Semi-minor axis accuracy. */
    uint8_t              semi_minor;

    /* Semi-major axis orientation. */
    uint16_t semi_major_orientation;

}rcp_position_accuracy;

typedef struct _rcp_position
{
    int32_t lat;
    int32_t lon;
    int32_t elev;

    /* Position accuracy for each axis. */
    rcp_position_accuracy accu;
} rcp_position_t;


/* Data frame: DF_AccelerationSet4Way. */
typedef struct _rcp_acceleration
{
    int16_t  lon;  /* Units: 0.01 m/s^2. */
    int16_t lat;
    int8_t  vert;  /* Units: 0.02G = 0.1962. */
    
    int16_t  yaw;  /* Units: 0.01 degrees per second. */
    
}rcp_acceleration_t;


typedef struct _rcp_motion
{

    uint8_t transmission_state;
    uint16_t speed;

#if 0
 #ifndef __LITTLE_ENDIAN	
 
    uint16_t transmission_state :3;
    uint16_t speed              :13;  
    
 #else
 
    uint16_t speed              :13;
    uint16_t transmission_state :3; 

 #endif
#endif

    uint16_t heading; 

    /* DE_SteeringWheelAngle: the angle of the driver's steering wheel. Units of 1.5 degrees. */
    int8_t steering_wheel_angle;
    
    rcp_acceleration_t  acce;                 
    
} rcp_motion_t;


typedef struct _rcp_msg_head
{
    rcp_msgid_t      msg_id;
    uint8_t       msg_count;
    uint8_t temporary_id[4];
    
}rcp_msg_head_t;


/* DDateTime */
typedef struct DDateTime {
	uint16_t year;	    /* OPTIONAL */
	uint8_t	 month;     /* OPTIONAL */
	uint8_t	 day;	    /* OPTIONAL */
	uint8_t	 hour;	    /* OPTIONAL */
	uint8_t	 minute;	/* OPTIONAL */
	uint16_t second;	/* OPTIONAL */
}  DDateTime_t;

typedef struct _ransmission_speed 
{
#ifndef __LITTLE_ENDIAN	
    uint16_t transmissionState:3;    /* e_TransmissionState */
    uint16_t speed:13;               /* (0..8191) -- Units of 0.02 m/s */
#else
    uint16_t speed:13;
    uint16_t transmissionState:3;
#endif
}transmission_speed_t;

/* FullPositionVector */
typedef struct _full_position_vector 
{
	//struct DDateTime utcTime;       /* OPTIONAL */
	int32_t	 lon;
	int32_t	 lat;
	int32_t  elev;  	            /* OPTIONAL */
	uint16_t heading;	            /* OPTIONAL */
	transmission_speed_t speed;	    /* OPTIONAL */
	int32_t	posAccuracy;	        /* OPTIONAL */
	int8_t	timeConfidence;	        /* OPTIONAL */
	int8_t	posConfidence;	        /* OPTIONAL */
	int16_t	speedConfidence;	    /* OPTIONAL */
}full_position_vector_t;

typedef struct _brake_system_status 
{
#ifndef __LITTLE_ENDIAN

    /* DE_BrakeAppliedStatus. */
    uint16_t wheel_brakes_unavailable :1;
    uint16_t wheel_brakes_leftfront   :1;
    uint16_t wheel_brakes_leftrear    :1;
    uint16_t wheel_brakes_rightfront  :1;
    uint16_t wheel_brakes_rightrear   :1;

    /* DE_TractionControlStatus. */
    uint16_t traction                 :2;

    /* DE_AntiLockBrakeStatus. */
    uint16_t abs                      :2;       

    /* DE_StabilityControlStatus. */
    uint16_t scs                      :2; 

    /* DE_BrakeBoostApplied. */
    uint16_t brake_boost              :2; 

    /* DE_AuxiliaryBrakeStatus */
    uint16_t aux_brakes               :2; 

    /* Reserved for later. */
    uint16_t reserved                 :1;
    
#else

    /* Reserved for later. */
    uint16_t reserved                 :1;

    /* DE_AuxiliaryBrakeStatus */
    uint16_t aux_brakes               :2; 

     /* DE_BrakeBoostApplied. */
    uint16_t brake_boost              :2; 

    /* DE_StabilityControlStatus. */
    uint16_t scs                      :2;

    /* DE_AntiLockBrakeStatus. */
    uint16_t abs                      :2; 

    /* DE_TractionControlStatus. */
    uint16_t traction                 :2;

    /* DE_BrakeAppliedStatus. */
    uint16_t wheel_brakes_rightrear   :1;
    uint16_t wheel_brakes_rightfront  :1;
    uint16_t wheel_brakes_leftrear    :1;
    uint16_t wheel_brakes_leftfront   :1;
    uint16_t wheel_brakes_unavailable :1;
    
#endif
} brake_system_status_t;


/* DF_VehicleSize. */
typedef struct _VehicleSize 
{
#ifndef __LITTLE_ENDIAN

    /* 0~1023, uint: 1cm */
    uint32_t width    :10; 
 
    /* 0~4095, unit: 1cm */     
    uint32_t length   :12;

    /* Reserved for later. */
    uint32_t reserved :10;
    
#else

    /* Reserved for later. */
    uint32_t reserved :10;
 
    /* 0~4095, unit: 1cm */
    uint32_t length:12;

    /* 0~1023, uint: 1cm */
    uint32_t width    :10;
    
#endif
}vehicle_size_t;



/* Data Frame: DF_VehicleSafetyExtensions. */
typedef struct _vehicle_safety_ext 
{
	uint16_t events;
    
    /* PathHistory: not supported yed */
    
}  vehicle_safety_ext_t;





/* Message: MSG_BasicSafetyMessage(BSM). */
typedef struct _rcp_msg_basic_safty
{

/* -----Part 1, Sent at all times with each message.----- */

    rcp_msg_head_t         header;

    /* Forward pid, not in J2735. */
    uint8_t         forward_id[4];  
    
    uint16_t              dsecond;
    rcp_position_t       position;
    
    rcp_motion_t           motion;

    /* Brake system status. */
    brake_system_status_t  brakes;

    /* Vehicle size. */
	vehicle_size_t	         size;


/* -----Part 2 Content. Optianal data for message.------ */

    /* Part 2 id for content. */
    uint8_t               part2_id;  

    /* Vehicle safety extensions. */
	vehicle_safety_ext_t safetyExt;
    
}rcp_msg_basic_safty_t;


/* MSG_RoadSideAlert(RSA)  */
typedef struct _msg_roadside_alert
{
    rcp_msgid_t   msg_id;
    uint8_t   msg_count;

    uint16_t	time_stamp;
    itis_codes_t typeEvent;
    itis_codes_t description[8];
    
	uint8_t	priority;
	heading_slice_t	heading;
	uint8_t	extent;
	full_position_vector_t	position;
	itis_codes_t	furtherInfoID;
    uint16_t  crc;
}rcp_msg_roadside_alert_t;

/* MSG_EmergencyVehicleAlert(EVA) */
typedef struct _msg_emergency_vehicle_alert
{
    rcp_msgid_t   msg_id;
    uint16_t	time_stamp;
    
    uint8_t   temporary_id[4];
    uint8_t   forward_id[4];   /* 转发节点pid */
    rcp_msg_roadside_alert_t rsa;
    uint8_t	  responseType;   /* OPTIONAL */
    uint8_t	  details;        /* OPTIONAL */
    uint8_t   mass;           /* OPTIONAL */
    uint8_t	  basicType;      /* OPTIONAL */
    uint16_t  vehicleType;    /* OPTIONAL */
    uint16_t  responseEquip;  /* OPTIONAL */
    uint16_t  responderType;  /* OPTIONAL */
    uint16_t  crc;
}rcp_msg_emergency_vehicle_alert_t;



/* restore all compiler settings in stacks. */
#pragma pack(pop)



#endif /* __CV_RCP_H__ */

