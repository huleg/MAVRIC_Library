/*
 * mavlink_actions.c
 *
 * Created: 21/03/2013 01:00:56
 *  Author: sfx
 */ 
#include "mavlink_actions.h"

#include "central_data.h"
#include "onboard_parameters.h"
#include "mavlink_stream.h"
#include "scheduler.h"
#include "doppler_radar.h"

central_data_t *central_data;


mavlink_send_heartbeat() {
	mavlink_msg_heartbeat_send(MAVLINK_COMM_0, MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_GENERIC, MAV_MODE_STABILIZE_DISARMED, 0, MAV_STATE_STANDBY);
}	


int even_odd;
void mavlink_send_radar() {
	radar_target_t *target=get_tracked_target();
	//mavlink_msg_radar_tracked_target_send(MAVLINK_COMM_0, time_keeper_get_millis(), 0, 0, target->velocity, target->amplitude, 0.0);
	//mavlink_msg_radar_raw_data_send(MAVLINK_COMM_0, time_keeper_get_millis(), 0, get_raw_FFT());
	//mavlink_msg_radar_raw_data_send(MAVLINK_COMM_0, time_keeper_get_millis(), 1, adc_int_get_buffer(0, 0));
	//mavlink_msg_radar_raw_data_send(MAVLINK_COMM_0, time_keeper_get_millis(), 2, adc_int_get_buffer(0, 1));
	

	
	mavlink_msg_named_value_float_send(MAVLINK_COMM_0, time_keeper_get_millis(), "Rad_vel", target->velocity);
	mavlink_msg_named_value_float_send(MAVLINK_COMM_0, time_keeper_get_millis(), "R_ampl", target->amplitude);


}

void mavlink_send_radar_raw() {
	radar_target_t *target=get_tracked_target();
	int16_t values[64];
	int32_t *fft =get_raw_FFT();
	int i;
	for (i=0; i<64; i++) values[i]=fft[i]/256;
	mavlink_msg_radar_raw_data_send(MAVLINK_COMM_0, time_keeper_get_millis(), 0, values);
	for (i=0; i<64; i++) values[i]=adc_int_get_sample(0, i);
	mavlink_msg_radar_raw_data_send(MAVLINK_COMM_0, time_keeper_get_millis(), 1, values);

	for (i=0; i<64; i++) values[i]=adc_int_get_sample(1, i);
	mavlink_msg_radar_raw_data_send(MAVLINK_COMM_0, time_keeper_get_millis(), 2, values);
	
	//for (i=0; i<64; i++) values[i]=adc_int_get_sample(2, i);
	//mavlink_msg_radar_raw_data_send(MAVLINK_COMM_0, time_keeper_get_millis(), 3, values);

	
//	mavlink_msg_named_value_float_send(MAVLINK_COMM_0, time_keeper_get_millis(), "Radar_velocity", target->velocity);
//	mavlink_msg_named_value_float_send(MAVLINK_COMM_0, time_keeper_get_millis(), "Radar_amplitude", target->amplitude/1000.0);
}
void mavlink_actions_handle_specific_messages (Mavlink_Received_t* rec) {
	
}

void mavlink_actions_init() {
	central_data=central_data_get_pointer_to_struct();
	scheduler_add_task(mavlink_stream_get_taskset(), 1000000, RUN_REGULAR, &mavlink_send_heartbeat, MAVLINK_MSG_ID_HEARTBEAT);
	//scheduler_add_task(mavlink_stream_get_taskset(), 250000, RUN_REGULAR, &mavlink_send_radar, MAVLINK_MSG_ID_NAMED_VALUE_FLOAT);
	scheduler_add_task(mavlink_stream_get_taskset(), 250000, RUN_NEVER, &mavlink_send_radar_raw, MAVLINK_MSG_ID_RADAR_RAW_DATA);

}
