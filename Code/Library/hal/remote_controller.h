/*
 * remote_controller.h
 *
 *  Created on: Aug 27, 2013
 *      Author: ndousse
 */


#ifndef REMOTE_CONTROLLER_H_
#define REMOTE_CONTROLLER_H_

#include "conf_platform.h"

#ifdef SPEKTRUM_REMOTE
	#include "spektrum.h"
#endif

#ifdef TURNIGY_REMOTE
	#include "turnigy.h"
#endif

#ifdef JOYSTICK_REMOTE
	#include "joystick.h"
#endif


#include "remote_dsm2.h"


static int16_t inline get_roll_from_remote()	{return rc_get_channel(RC_ROLL); };
static int16_t inline get_pitch_from_remote()	{return rc_get_channel(RC_PITCH); };
static int16_t inline get_yaw_from_remote()	{return rc_get_channel(RC_YAW); };
static int16_t inline get_thrust_from_remote()	{return rc_get_channel(RC_THROTTLE); };


static inline Control_Command_t get_command_from_remote()
{
	Control_Command_t controls;
	controls.rpy[ROLL]= get_roll_from_remote();
	controls.rpy[PITCH]= get_pitch_from_remote();
	controls.rpy[YAW]= get_yaw_from_remote();
	controls.thrust = get_thrust_from_remote();
	
	return controls;
}

static inline void get_channel_mode(uint8_t* chanSwitch)
{
	if (rc_get_channel(RC_SAFETY)<0)
	{
		*chanSwitch |= 0x00;
		}else if(rc_get_channel(RC_SAFETY)>0 && rc_get_channel(RC_ID_MODE)<0){
		*chanSwitch |= 0x01;
		}else if (rc_get_channel(RC_SAFETY)>0 && rc_get_channel(RC_ID_MODE)>20){
		*chanSwitch |= 0x03;
		}else{
		*chanSwitch |= 0x02;
	}
}


#endif //REMOTE_CONTROLLER_H_