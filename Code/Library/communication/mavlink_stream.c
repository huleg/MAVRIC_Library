/**
 * \page The MAV'RIC License
 *
 * The MAV'RIC Framework
 *
 * Copyright © 2011-2014
 *
 * Laboratory of Intelligent Systems, EPFL
 */


/**
 * \file mavlink_stream.c
 * 
 * A wrapper for mavlink to use the stream interface
 */


#include "mavlink_stream.h"
#include "buffer.h"
#include "onboard_parameters.h"
#include "print_util.h"

#include "mavlink_actions.h"	// TODO: remove this dependency to src/ folder

mavlink_system_t mavlink_system;
mavlink_system_t mavlink_mission_planner;

static volatile byte_stream_t* mavlink_out_stream;
static volatile byte_stream_t* mavlink_in_stream;
static volatile Buffer_t mavlink_in_buffer;

task_set_t mavlink_tasks;


//------------------------------------------------------------------------------
// PRIVATE FUNCTIONS DECLARATION
//------------------------------------------------------------------------------

/**
 * \brief			Mavlink parsing of message
 *
 * \param stream	Pointer to the mavlink receive stream structure
 * \param rec		Pointer to the mavlink receive message structure 
 *
 * \return			Error code, 0 if no message decoded, 1 else
 */
static uint8_t mavlink_stream_receive(byte_stream_t* stream, Mavlink_Received_t* rec);


/**
 * \brief	Receive mavlink message
 */
static void mavlink_stream_receive_handler(void);


/**
 * \brief		handling specific mavlink message
 *
 * \param rec	Pointer to the mavlink receive message structure
 */
static void mavlink_stream_handle_message(Mavlink_Received_t* rec);


//------------------------------------------------------------------------------
// PRIVATE FUNCTIONS IMPLEMENTATION
//------------------------------------------------------------------------------

static uint8_t mavlink_stream_receive(byte_stream_t* stream, Mavlink_Received_t* rec) 
{
	uint8_t byte;
	while(stream->bytes_available(stream->data) > 0) 
	{
		byte = stream->get(stream->data);
		if(mavlink_parse_char(MAVLINK_COMM_0, byte, &rec->msg, &rec->status)) 
		{
			return 1;
		}
	}
	return 0;
}


static void mavlink_stream_receive_handler() 
{
	Mavlink_Received_t rec;
	
	if(mavlink_stream_receive((byte_stream_t*)mavlink_in_stream, &rec)) 
	{
		mavlink_stream_handle_message(&rec);
	}
}


void mavlink_stream_handle_message(Mavlink_Received_t* rec) 
{
	if (rec->msg.sysid == MAVLINK_BASE_STATION_ID) 
	{
		//print_util_dbg_print("\n Received message with ID");
		//print_util_dbg_print_num(rec->msg.msgid, 10);
		//print_util_dbg_print(" from system");
		//print_util_dbg_print_num(rec->msg.sysid, 10);
		//print_util_dbg_print(" for component");
		//print_util_dbg_print_num(rec->msg.compid,10);
		//print_util_dbg_print( "\n");
		
		switch(rec->msg.msgid) 
		{
			case MAVLINK_MSG_ID_PARAM_REQUEST_LIST: 
			{ // 21
				mavlink_param_request_list_t request;
				mavlink_msg_param_request_list_decode(&rec->msg, &request);
			
				print_util_dbg_print("msg comp id:");
				print_util_dbg_print_num(request.target_component,10);
				print_util_dbg_print("\n");
			
				// Check if this message is for this system
				if ((uint8_t)request.target_system == (uint8_t)mavlink_system.sysid) 
				{
					print_util_dbg_print("Sending all parameters \n");
					onboard_parameters_send_all_parameters();
				}				
			}
			break;
			case MAVLINK_MSG_ID_PARAM_REQUEST_READ: 
			{ //20
				mavlink_param_request_read_t request;
				mavlink_msg_param_request_read_decode(&rec->msg, &request);
				// Check if this message is for this system and subsystem
				if ((uint8_t)request.target_system == (uint8_t)mavlink_system.sysid)
				//&& (uint8_t)request.target_component == (uint8_t)mavlink_system.compid)
				 {
					print_util_dbg_print("Sending parameter ");
					print_util_dbg_print(request.param_id);
					onboard_parameters_send_parameter(&request);
				}				
			}
			break;
			case MAVLINK_MSG_ID_PARAM_SET: 
			{ //23
				mavlink_stream_suspend_downstream(100000);
				onboard_parameters_receive_parameter(rec);
			}
			break;

			case MAVLINK_MSG_ID_REQUEST_DATA_STREAM: 
			{ // 66
				volatile mavlink_request_data_stream_t request;
				mavlink_msg_request_data_stream_decode(&rec->msg, (mavlink_request_data_stream_t*) &request);
				// TODO: control target_component == compid!
				if ((uint8_t)request.target_system == (uint8_t)mavlink_system.sysid)
				//&& (uint8_t)request.target_component == (uint8_t)mavlink_system.compid)
				{
					print_util_dbg_print("stream request:");
					print_util_dbg_print_num(request.target_component,10);
					if (request.req_stream_id==255) 
					{
						int32_t i;
						print_util_dbg_print("send all\n");
						// send full list of streams
						for (i=0; i<mavlink_tasks.number_of_tasks; i++) 
						{
							task_entry_t *task=scheduler_get_task_by_index(&mavlink_tasks, i);
							scheduler_run_task_now(task);
						}					
					} 
					else 
					{
						task_entry_t *task =scheduler_get_task_by_id(&mavlink_tasks, request.req_stream_id);
						print_util_dbg_print(" stream="); print_util_dbg_print_num(request.req_stream_id, 10);
						print_util_dbg_print(" start_stop=");print_util_dbg_print_num(request.start_stop, 10);
						print_util_dbg_print(" rate=");print_util_dbg_print_num(request.req_message_rate,10);
						print_util_dbg_print("\n");
						print_util_dbg_print("\n");
						if (request.start_stop) 
						{
							scheduler_change_run_mode(task, RUN_REGULAR);
						}
						else 
						{
							scheduler_change_run_mode(task, RUN_NEVER);
						}
						if (request.req_message_rate>0) 
						{
							scheduler_change_task_period(task, SCHEDULER_TIMEBASE / (uint32_t)request.req_message_rate);
						}
					}
				}
			}	
			break;
		}
	}

	// handle all platform-specific messages in mavlink-actions:
	mavlink_actions_handle_specific_messages(rec);
}


//------------------------------------------------------------------------------
// PUBLIC FUNCTIONS IMPLEMENTATION
//------------------------------------------------------------------------------

void mavlink_comm_send_ch(mavlink_channel_t chan, uint8_t ch)
{
	if (chan == MAVLINK_COMM_0)
	{
		mavlink_out_stream->put(mavlink_out_stream->data, ch);
	}
}


void mavlink_stream_init(byte_stream_t *transmit_stream, byte_stream_t *receive_stream, int32_t sysid) 
{
	mavlink_tasks.number_of_tasks=30;
	
	mavlink_system.sysid = sysid; // System ID, 1-255
	mavlink_system.compid = 50; // Component/Subsystem ID, 1-255
	mavlink_system.type = MAV_TYPE_QUADROTOR;
	
	mavlink_mission_planner.sysid = mavlink_system.sysid;
	mavlink_mission_planner.compid = MAV_COMP_ID_MISSIONPLANNER;
	mavlink_mission_planner.type = MAV_TYPE_QUADROTOR;
	
	mavlink_out_stream = transmit_stream;
	mavlink_in_stream = receive_stream;
	
	scheduler_init(&mavlink_tasks);
	
	scheduler_add_task(&mavlink_tasks, 100000, RUN_REGULAR, &onboard_parameters_send_scheduled_parameters, MAVLINK_MSG_ID_PARAM_VALUE);
}

void mavlink_stream_flush() 
{
	if (mavlink_out_stream->flush != NULL) 
	{
		mavlink_out_stream->flush(mavlink_out_stream->data);	
	}
}

task_return_t mavlink_stream_protocol_update() 
{
	task_return_t result=0;
	mavlink_stream_receive_handler();
	if ((mavlink_out_stream->buffer_empty(mavlink_out_stream->data))==true) 
	{
		result = scheduler_run_update(&mavlink_tasks, ROUND_ROBIN);
	}
	return result;
}

task_set_t* mavlink_stream_get_taskset() 
{
	return &mavlink_tasks;
}

void mavlink_stream_suspend_downstream(uint32_t delay) 
{
	int32_t i;
	for (i=0; i<mavlink_tasks.number_of_tasks; i++) 
	{
		scheduler_suspend_task(&mavlink_tasks.tasks[i], delay);
	}	
}