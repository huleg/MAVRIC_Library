/*
 * central_data.h
 *
 * Created: 16/09/2013 12:18:00
 *  Author: sfx
 */ 


#ifndef CENTRAL_DATA_H_
#define CENTRAL_DATA_H_

#include <stdbool.h>
#include <stdint.h>

#include "stdbool.h"

#include "time_keeper.h"
//#include "i2c_driver_int.h"
#include "qfilter.h"
#include "imu.h"

// #include "stabilisation.h"
// #include "stabilisation_copter.h"
#include "stabilisation_hybrid.h"

#include "remote_controller.h"
#include "pid_control.h"
#include "streams.h"
//#include "uart_int.h"
#include "print_util.h"

#include "bmp085.h"
#include "mavlink_stream.h"
#include "coord_conventions.h"
#include "onboard_parameters.h"
#include "servo_pwm.h"

#include "gps_ublox.h"
#include "mavlink_waypoint_handler.h"
#include "estimator.h"
#include "simulation.h"
#include "bmp085.h"
#include "conf_sim_model.h"
#include "neighbor_selection.h"
#include "position_estimation.h"

static const servo_output servo_failsafe[NUMBER_OF_SERVO_OUTPUTS]={{.value=0}, {.value=0}, {.value=0}, {.value=0}, {.value=-600}, {.value=-600}, {.value=-600}, {.value=-600}};

enum CRITICAL_BEHAVIOR_ENUM{
	CLIMB_TO_SAFE_ALT = 1,
	FLY_TO_HOME_WP = 2,
	CRITICAL_LAND = 3,
};

typedef struct  {
	Imu_Data_t imu1;
	Control_Command_t controls;
	Control_Command_t controls_nav;

	Stabiliser_Stack_hybrid_t stabiliser_stack;

	simulation_model_t uav_model;
	servo_output servos[NUMBER_OF_SERVO_OUTPUTS];
	Buffer_t xbee_in_buffer, wired_in_buffer;
	byte_stream_t xbee_out_stream;
	byte_stream_t xbee_in_stream;
	byte_stream_t wired_out_stream, wired_in_stream;
	
	Buffer_t gps_buffer;
	byte_stream_t gps_stream_in;
	byte_stream_t gps_stream_out;
	gps_Data_type GPS_data;
	
	Estimator_Data_t estimation;
	simulation_model_t sim_model;
	
	position_estimator_t position_estimator;
	
	// aliases
	byte_stream_t *telemetry_down_stream, *telemetry_up_stream;
	byte_stream_t *debug_out_stream, *debug_in_stream;
	
	waypoint_struct waypoint_list[MAX_WAYPOINTS];
	waypoint_struct current_waypoint;
	uint16_t number_of_waypoints;
	int8_t current_wp_count;
	
	local_coordinates_t waypoint_coordinates, waypoint_hold_coordinates, waypoint_critical_coordinates;
	float dist2wp_sqr;
	
	bool waypoint_set;
	bool waypoint_sending;
	bool waypoint_receiving;
	bool waypoint_hold_init;
	bool critical_landing;
	bool critical_init;
	bool critical_next_state;
	
	bool collision_avoidance;
	
	uint8_t mav_mode;
	uint8_t mav_state;
	uint32_t simulation_mode;
	
	pressure_data pressure;
	//float pressure_filtered;
	//float altitude_filtered;
	
	uint8_t number_of_neighbors;
	float safe_size;
	track_neighbor_t listNeighbors[MAX_NUM_NEIGHBORS];
	
	enum CRITICAL_BEHAVIOR_ENUM critical_behavior;
	
} central_data_t;


void initialise_central_data(void);

central_data_t* get_central_data(void);

byte_stream_t* get_telemetry_upstream(void);
byte_stream_t* get_telemetry_downstream(void);
byte_stream_t* get_debug_stream(void);

Imu_Data_t* get_imu_data();
Control_Command_t* get_control_inputs_data();

#define STDOUT &get_debug_stream()

#endif /* CENTRAL_DATA_H_ */