/*
 * mavlink_stream.h
 *
 * a wrapper for mavlink to use the stream interface
 *
 * Created: 14/02/2013 17:10:05
 *  Author: julien
 */ 


#ifndef MAVLINK_STREAM_H_
#define MAVLINK_STREAM_H_

#include "streams.h"
#include "mavlink_bridge.h"
#include "mavlink/include/common/mavlink.h"

typedef struct {
	mavlink_message_t msg;
	mavlink_status_t status;
} Mavlink_Received_t;

void init_mavlink(byte_stream_t *transmit_stream);
void mavlink_receive(stream_data_t* data, uint8_t element);
void handle_mavlink_message(mavlink_channel_t chan, mavlink_message_t* msg);


#endif /* MAVLINK_STREAM_H */