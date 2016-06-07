/*******************************************************************************
 * Copyright (c) 2009-2016, MAV'RIC Development Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*******************************************************************************
 * \file mavlink_waypoint_handler.c
 *
 * \author MAV'RIC Team
 * \author Nicolas Dousse
 * \author Matthew Douglas
 *
 * \brief The MAVLink waypoint handler
 *
 ******************************************************************************/


#include "communication/mavlink_waypoint_handler.hpp"
#include <cstdlib>
#include "hal/common/time_keeper.hpp"

extern "C"
{
#include "util/print_util.h"
#include "util/maths.h"
#include "util/constants.h"
}


//------------------------------------------------------------------------------
// PRIVATE FUNCTIONS IMPLEMENTATION
//------------------------------------------------------------------------------


void Mavlink_waypoint_handler::send_count(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg)
{
    mavlink_mission_request_list_t packet;

    mavlink_msg_mission_request_list_decode(msg, &packet);

    // Check if this message is for this system and subsystem
    if (((uint8_t)packet.target_system == (uint8_t)sysid)
            && ((uint8_t)packet.target_component == (uint8_t)MAV_COMP_ID_MISSIONPLANNER))
    {
        mavlink_message_t _msg;
        mavlink_msg_mission_count_pack(sysid,
                                       waypoint_handler->mavlink_stream_.compid(),
                                       &_msg,
                                       msg->sysid,
                                       msg->compid,
                                       waypoint_handler->waypoint_count_);
        waypoint_handler->mavlink_stream_.send(&_msg);

        if (waypoint_handler->waypoint_count_ != 0)
        {
            waypoint_handler->waypoint_sending_ = true;
            waypoint_handler->waypoint_receiving_ = false;
            waypoint_handler->start_timeout_ = time_keeper_get_ms();
        }

        waypoint_handler->sending_waypoint_num_ = 0;
        print_util_dbg_print("Will send ");
        print_util_dbg_print_num(waypoint_handler->waypoint_count_, 10);
        print_util_dbg_print(" waypoints\r\n");
    }
}

void Mavlink_waypoint_handler::send_waypoint(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg)
{
    if (waypoint_handler->waypoint_sending_)
    {
        mavlink_mission_request_t packet;

        mavlink_msg_mission_request_decode(msg, &packet);

        print_util_dbg_print("Asking for waypoint number ");
        print_util_dbg_print_num(packet.seq, 10);
        print_util_dbg_print("\r\n");

        // Check if this message is for this system and subsystem
        if (((uint8_t)packet.target_system == (uint8_t)sysid)
                && ((uint8_t)packet.target_component == (uint8_t)MAV_COMP_ID_MISSIONPLANNER))
        {
            waypoint_handler->sending_waypoint_num_ = packet.seq;
            if (waypoint_handler->sending_waypoint_num_ < waypoint_handler->waypoint_count_)
            {
                //  Prototype of the function "mavlink_msg_mission_item_send" found in mavlink_msg_mission_item.h :
                // mavlink_msg_mission_item_send (  mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint16_t seq,
                //                                  uint8_t frame, uint16_t command, uint8_t current, uint8_t autocontinue, float param1,
                //                                  float param2, float param3, float param4, float x, float y, float z)
                mavlink_message_t _msg;
                mavlink_msg_mission_item_pack(sysid,
                                              waypoint_handler->mavlink_stream_.compid(),
                                              &_msg,
                                              msg->sysid,
                                              msg->compid,
                                              packet.seq,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].frame,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].command,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].current,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].autocontinue,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].param1,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].param2,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].param3,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].param4,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].x,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].y,
                                              waypoint_handler->waypoint_list_[waypoint_handler->sending_waypoint_num_].z);
                waypoint_handler->mavlink_stream_.send(&_msg);

                print_util_dbg_print("Sending waypoint ");
                print_util_dbg_print_num(waypoint_handler->sending_waypoint_num_, 10);
                print_util_dbg_print("\r\n");

                waypoint_handler->start_timeout_ = time_keeper_get_ms();
            }
        }
    }
}

void Mavlink_waypoint_handler::receive_ack_msg(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg)
{
    mavlink_mission_ack_t packet;

    mavlink_msg_mission_ack_decode(msg, &packet);

    // Check if this message is for this system and subsystem
    if (((uint8_t)packet.target_system == (uint8_t)sysid)
            && ((uint8_t)packet.target_component == (uint8_t)MAV_COMP_ID_MISSIONPLANNER))
    {
        waypoint_handler->waypoint_sending_ = false;
        waypoint_handler->sending_waypoint_num_ = 0;
        print_util_dbg_print("Acknowledgment received, end of waypoint sending.\r\n");
    }
}

void Mavlink_waypoint_handler::receive_count(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg)
{
    mavlink_mission_count_t packet;

    mavlink_msg_mission_count_decode(msg, &packet);

    print_util_dbg_print("Count:");
    print_util_dbg_print_num(packet.count, 10);
    print_util_dbg_print("\r\n");

    // Check if this message is for this system and subsystem
    if (((uint8_t)packet.target_system == (uint8_t)sysid)
            && ((uint8_t)packet.target_component == (uint8_t)MAV_COMP_ID_MISSIONPLANNER))
    {
        if (waypoint_handler->waypoint_receiving_ == false)
        {
            // comment these lines if you want to add new waypoints to the list instead of overwriting them
            waypoint_handler->waypoint_onboard_count_ = 0;
            waypoint_handler->waypoint_count_ = 0;
            //---//

            if ((packet.count + waypoint_handler->waypoint_count_) > MAX_WAYPOINTS)
            {
                packet.count = MAX_WAYPOINTS - waypoint_handler->waypoint_count_;
            }
            waypoint_handler->waypoint_count_ =  packet.count + waypoint_handler->waypoint_count_;
            print_util_dbg_print("Receiving ");
            print_util_dbg_print_num(packet.count, 10);
            print_util_dbg_print(" new waypoints. ");
            print_util_dbg_print("New total number of waypoints:");
            print_util_dbg_print_num(waypoint_handler->waypoint_count_, 10);
            print_util_dbg_print("\r\n");

            waypoint_handler->waypoint_receiving_   = true;
            waypoint_handler->waypoint_sending_     = false;
            waypoint_handler->waypoint_request_number_ = 0;


            waypoint_handler->start_timeout_ = time_keeper_get_ms();
        }

        mavlink_message_t _msg;
        mavlink_msg_mission_request_pack(sysid,
                                         waypoint_handler->mavlink_stream_.compid(),
                                         &_msg,
                                         msg->sysid,
                                         msg->compid,
                                         waypoint_handler->waypoint_request_number_);
        waypoint_handler->mavlink_stream_.send(&_msg);

        print_util_dbg_print("Asking for waypoint ");
        print_util_dbg_print_num(waypoint_handler->waypoint_request_number_, 10);
        print_util_dbg_print("\r\n");
    }

}

void Mavlink_waypoint_handler::receive_waypoint(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg)
{
    mavlink_mission_item_t packet;

    mavlink_msg_mission_item_decode(msg, &packet);

    // Check if this message is for this system and subsystem
    if (((uint8_t)packet.target_system == (uint8_t)sysid)
            && ((uint8_t)packet.target_component == (uint8_t)MAV_COMP_ID_MISSIONPLANNER))
    {
        waypoint_handler->start_timeout_ = time_keeper_get_ms();

        waypoint_struct_t new_waypoint;

        new_waypoint.command = packet.command;

        new_waypoint.x = packet.x; // longitude
        new_waypoint.y = packet.y; // latitude
        new_waypoint.z = packet.z; // altitude

        new_waypoint.autocontinue = packet.autocontinue;
        new_waypoint.frame = packet.frame

        new_waypoint.param1 = packet.param1;
        new_waypoint.param2 = packet.param2;
        new_waypoint.param3 = packet.param3;
        new_waypoint.param4 = packet.param4;

        print_util_dbg_print("New waypoint received ");
        //print_util_dbg_print("(");
        //print_util_dbg_print_num(new_waypoint.x,10);
        //print_util_dbg_print(", ");
        //print_util_dbg_print_num(new_waypoint.y,10);
        //print_util_dbg_print(", ");
        //print_util_dbg_print_num(new_waypoint.z,10);
        //print_util_dbg_print(") Autocontinue:");
        //print_util_dbg_print_num(new_waypoint.autocontinue,10);
        //print_util_dbg_print(" Frame:");
        //print_util_dbg_print_num(new_waypoint.frame,10);
        //print_util_dbg_print(" Current :");
        //print_util_dbg_print_num(packet.current,10);
        //print_util_dbg_print(" Seq :");
        //print_util_dbg_print_num(packet.seq,10);
        //print_util_dbg_print(" command id :");
        //print_util_dbg_print_num(packet.command,10);
        print_util_dbg_print(" requested num :");
        print_util_dbg_print_num(waypoint_handler->waypoint_request_number_, 10);
        print_util_dbg_print(" receiving num :");
        print_util_dbg_print_num(packet.seq, 10);
        //print_util_dbg_print(" is it receiving :");
        //print_util_dbg_print_num(waypoint_handler->waypoint_receiving_,10); // boolean value
        print_util_dbg_print("\r\n");

        //current = 2 is a flag to tell us this is a "guided mode" waypoint and not for the mission
        if (packet.current == 2)
        {
            // verify we received the command;
            mavlink_message_t _msg;
            mavlink_msg_mission_ack_pack(sysid,
                                         waypoint_handler->mavlink_stream_.compid(),
                                         &_msg,
                                         msg->sysid,
                                         msg->compid,
                                         MAV_MISSION_UNSUPPORTED);
            waypoint_handler->mavlink_stream_.send(&_msg);
        }
        else if (packet.current == 3)
        {
            //current = 3 is a flag to tell us this is a alt change only

            // verify we received the command
            mavlink_message_t _msg;
            mavlink_msg_mission_ack_pack(waypoint_handler->mavlink_stream_.sysid(),
                                         waypoint_handler->mavlink_stream_.compid(),
                                         &_msg,
                                         msg->sysid,
                                         msg->compid,
                                         MAV_MISSION_UNSUPPORTED);
            waypoint_handler->mavlink_stream_.send(&_msg);
        }
        else
        {
            // Check if receiving waypoints
            if (waypoint_handler->waypoint_receiving_)
            {
                // check if this is the requested waypoint
                if (packet.seq == waypoint_handler->waypoint_request_number_)
                {
                    print_util_dbg_print("Receiving good waypoint, number ");
                    print_util_dbg_print_num(waypoint_handler->waypoint_request_number_, 10);
                    print_util_dbg_print(" of ");
                    print_util_dbg_print_num(waypoint_handler->waypoint_count_ - waypoint_handler->waypoint_onboard_count_, 10);
                    print_util_dbg_print("\r\n");

                    waypoint_handler->waypoint_list_[waypoint_handler->waypoint_onboard_count_ + waypoint_handler->waypoint_request_number_] = new_waypoint;
                    waypoint_handler->waypoint_request_number_++;

                    if ((waypoint_handler->waypoint_onboard_count_ + waypoint_handler->waypoint_request_number_) == waypoint_handler->waypoint_count_)
                    {
                        MAV_MISSION_RESULT type = MAV_MISSION_ACCEPTED;

                        mavlink_message_t _msg;
                        mavlink_msg_mission_ack_pack(waypoint_handler->mavlink_stream_.sysid(),
                                                     waypoint_handler->mavlink_stream_.compid(),
                                                     &_msg,
                                                     msg->sysid,
                                                     msg->compid, type);
                        waypoint_handler->mavlink_stream_.send(&_msg);

                        print_util_dbg_print("flight plan received!\n");
                        waypoint_handler->waypoint_receiving_ = false;
                        waypoint_handler->waypoint_onboard_count_ = waypoint_handler->waypoint_count_;

                        waypoint_handler->start_wpt_time_ = time_keeper_get_ms();

                        waypoint_handler->state_.nav_plan_active = false;
                        waypoint_handler->nav_plan_init();
                    }
                    else
                    {
                        mavlink_message_t _msg;
                        mavlink_msg_mission_request_pack(waypoint_handler->mavlink_stream_.sysid(),
                                                         waypoint_handler->mavlink_stream_.compid(),
                                                         &_msg,
                                                         msg->sysid,
                                                         msg->compid,
                                                         waypoint_handler->waypoint_request_number_);
                        waypoint_handler->mavlink_stream_.send(&_msg);

                        print_util_dbg_print("Asking for waypoint ");
                        print_util_dbg_print_num(waypoint_handler->waypoint_request_number_, 10);
                        print_util_dbg_print("\n");
                    }
                } //end of if (packet.seq == waypoint_handler->waypoint_request_number_)
                else
                {
                    MAV_MISSION_RESULT type = MAV_MISSION_INVALID_SEQUENCE;

                    mavlink_message_t _msg;
                    mavlink_msg_mission_ack_pack(waypoint_handler->mavlink_stream_.sysid(),
                                                 waypoint_handler->mavlink_stream_.compid(),
                                                 &_msg,
                                                 msg->sysid,
                                                 msg->compid,
                                                 type);
                    waypoint_handler->mavlink_stream_.send(&_msg);
                }
            } //end of if (waypoint_handler->waypoint_receiving_)
            else
            {
                MAV_MISSION_RESULT type = MAV_MISSION_ERROR;
                print_util_dbg_print("Not ready to receive waypoints right now!\r\n");

                mavlink_message_t _msg;
                mavlink_msg_mission_ack_pack(waypoint_handler->mavlink_stream_.sysid(),
                                             waypoint_handler->mavlink_stream_.compid(),
                                             &_msg,
                                             msg->sysid,
                                             msg->compid,
                                             type);
                waypoint_handler->mavlink_stream_.send(&_msg);
            } //end of else of if (waypoint_handler->waypoint_receiving_)
        } //end of else (packet.current != 2 && !=3 )
    } //end of if this message is for this system and subsystem
}

void Mavlink_waypoint_handler::set_current_waypoint(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg)
{
    mavlink_mission_set_current_t packet;

    mavlink_msg_mission_set_current_decode(msg, &packet);

    // Check if this message is for this system and subsystem
    if (((uint8_t)packet.target_system == (uint8_t)sysid)
            && ((uint8_t)packet.target_component == (uint8_t)MAV_COMP_ID_MISSIONPLANNER))
    {
        if (packet.seq < waypoint_handler->waypoint_count_)
        {
            current_waypoint_index_ = packet.seq;

            mavlink_message_t _msg;
            mavlink_msg_mission_current_pack(sysid,
                                             waypoint_handler->mavlink_stream_.compid(),
                                             &_msg,
                                             packet.seq);
            waypoint_handler->mavlink_stream_.send(&_msg);

            print_util_dbg_print("Set current waypoint to number");
            print_util_dbg_print_num(packet.seq, 10);
            print_util_dbg_print("\r\n");

            waypoint_handler->start_wpt_time_ = time_keeper_get_ms();

            waypoint_handler->state_.nav_plan_active = false;
            waypoint_handler->nav_plan_init();
        }
        else
        {
            mavlink_message_t _msg;
            mavlink_msg_mission_ack_pack(waypoint_handler->mavlink_stream_.sysid(),
                                         waypoint_handler->mavlink_stream_.compid(),
                                         &_msg,
                                         msg->sysid,
                                         msg->compid,
                                         MAV_CMD_ACK_ERR_ACCESS_DENIED);
            waypoint_handler->mavlink_stream_.send(&_msg);
        }
    } //end of if this message is for this system and subsystem
}


void Mavlink_waypoint_handler::clear_waypoint_list_(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg)
{
    mavlink_mission_clear_all_t packet;

    mavlink_msg_mission_clear_all_decode(msg, &packet);

    // Check if this message is for this system and subsystem
    if (((uint8_t)packet.target_system == (uint8_t)sysid)
            && ((uint8_t)packet.target_component == (uint8_t)MAV_COMP_ID_MISSIONPLANNER))
    {
        if (waypoint_handler->waypoint_count_ > 0)
        {
            waypoint_handler->waypoint_count_ = 0;
            waypoint_handler->waypoint_onboard_count_ = 0;
            waypoint_handler->state_.nav_plan_active = false;
            waypoint_handler->hold_waypoint_set_ = false;

            mavlink_message_t _msg;
            mavlink_msg_mission_ack_pack(waypoint_handler->mavlink_stream_.sysid(),
                                         waypoint_handler->mavlink_stream_.compid(),
                                         &_msg,
                                         msg->sysid,
                                         msg->compid,
                                         MAV_CMD_ACK_OK);
            waypoint_handler->mavlink_stream_.send(&_msg);

            print_util_dbg_print("Cleared Waypoint list.\r\n");
        }
    }
}


void Mavlink_waypoint_handler::control_time_out_waypoint_msg()
{
    if (waypoint_sending_ || waypoint_receiving_)
    {
        uint32_t tnow = time_keeper_get_ms();

        if ((tnow - start_timeout_) > timeout_max_waypoint_)
        {
            start_timeout_ = tnow;
            if (waypoint_sending_)
            {
                waypoint_sending_ = false;
                print_util_dbg_print("Sending waypoint timeout\r\n");
            }
            if (waypoint_receiving_)
            {
                waypoint_receiving_ = false;

                print_util_dbg_print("Receiving waypoint timeout\r\n");
                waypoint_count_ = 0;
                waypoint_onboard_count_ = 0;
            }
        }
    }
}


//------------------------------------------------------------------------------
// PUBLIC FUNCTIONS IMPLEMENTATION
//------------------------------------------------------------------------------

Mavlink_waypoint_handler::Mavlink_waypoint_handler(State& state_, Mavlink_message_handler& message_handler, const Mavlink_stream& mavlink_stream_, conf_t config):
            waypoint_count_(0),
            current_waypoint_index_(0),
            hold_waypoint_set_(false),
            start_wpt_time_(time_keeper_get_ms()),
            mavlink_stream_(mavlink_stream_),
            state_(state_),
            waypoint_sending_(false),
            waypoint_receiving_(false),
            sending_waypoint_num_(0),
            waypoint_request_number_(0),
            waypoint_onboard_count_(0),
            start_timeout_(time_keeper_get_ms()),
            timeout_max_waypoint_(10000),
            config_(config)
{
    bool init_success = true;

    // Add callbacks for waypoint handler messages requests
    Mavlink_message_handler::msg_callback_t callback;

    callback.message_id     = MAVLINK_MSG_ID_MISSION_ITEM; // 39
    callback.sysid_filter   = MAVLINK_BASE_STATION_ID;
    callback.compid_filter  = MAV_COMP_ID_ALL;
    callback.function       = (Mavlink_message_handler::msg_callback_func_t)      &receive_waypoint;
    callback.module_struct  = (Mavlink_message_handler::handling_module_struct_t) this;
    init_success &= message_handler.add_msg_callback(&callback);

    callback.message_id     = MAVLINK_MSG_ID_MISSION_REQUEST; // 40
    callback.sysid_filter   = MAVLINK_BASE_STATION_ID;
    callback.compid_filter  = MAV_COMP_ID_ALL;
    callback.function       = (Mavlink_message_handler::msg_callback_func_t)      &send_waypoint;
    callback.module_struct  = (Mavlink_message_handler::handling_module_struct_t) this;
    init_success &= message_handler.add_msg_callback(&callback);

    callback.message_id     = MAVLINK_MSG_ID_MISSION_SET_CURRENT; // 41
    callback.sysid_filter   = MAVLINK_BASE_STATION_ID;
    callback.compid_filter  = MAV_COMP_ID_ALL;
    callback.function       = (Mavlink_message_handler::msg_callback_func_t)      &set_current_waypoint;
    callback.module_struct  = (Mavlink_message_handler::handling_module_struct_t) this;
    init_success &= message_handler.add_msg_callback(&callback);

    callback.message_id     = MAVLINK_MSG_ID_MISSION_REQUEST_LIST; // 43
    callback.sysid_filter   = MAVLINK_BASE_STATION_ID;
    callback.compid_filter  = MAV_COMP_ID_ALL;
    callback.function       = (Mavlink_message_handler::msg_callback_func_t)      &send_count;
    callback.module_struct  = (Mavlink_message_handler::handling_module_struct_t) this;
    init_success &= message_handler.add_msg_callback(&callback);

    callback.message_id     = MAVLINK_MSG_ID_MISSION_COUNT; // 44
    callback.sysid_filter   = MAVLINK_BASE_STATION_ID;
    callback.compid_filter  = MAV_COMP_ID_ALL;
    callback.function       = (Mavlink_message_handler::msg_callback_func_t)      &receive_count;
    callback.module_struct  = (Mavlink_message_handler::handling_module_struct_t) this;
    init_success &= message_handler.add_msg_callback(&callback);

    callback.message_id     = MAVLINK_MSG_ID_MISSION_CLEAR_ALL; // 45
    callback.sysid_filter   = MAVLINK_BASE_STATION_ID;
    callback.compid_filter  = MAV_COMP_ID_ALL;
    callback.function       = (Mavlink_message_handler::msg_callback_func_t)      &clear_waypoint_list_;
    callback.module_struct  = (Mavlink_message_handler::handling_module_struct_t) this;
    init_success &= message_handler.add_msg_callback(&callback);

    callback.message_id     = MAVLINK_MSG_ID_MISSION_ACK; // 47
    callback.sysid_filter   = MAVLINK_BASE_STATION_ID;
    callback.compid_filter  = MAV_COMP_ID_ALL;
    callback.function       = (Mavlink_message_handler::msg_callback_func_t)      &receive_ack_msg;
    callback.module_struct  = (Mavlink_message_handler::handling_module_struct_t) this;
    init_success &= message_handler.add_msg_callback(&callback);

    init_homing_waypoint();
    nav_plan_init();

    if(!init_success)
    {
        print_util_dbg_print("[MAVLINK_WAYPOINT_HANDLER] constructor: ERROR\r\n");
    }
}


void Mavlink_waypoint_handler::init_homing_waypoint()
{
    waypoint_struct_t waypoint;

    waypoint_count_ = 1;
    waypoint_onboard_count_ = waypoint_count_;
    current_waypoint_index_ = 0;

    //Set home waypoint
    waypoint.autocontinue = 0;
    waypoint.frame = MAV_FRAME_LOCAL_NED;
    waypoint.command = MAV_CMD_NAV_WAYPOINT;

    waypoint.x = 0.0f;
    waypoint.y = 0.0f;
    waypoint.z = -config_.auto_take_off_altitude;

    waypoint.param1 = 10; // Hold time in decimal seconds
    waypoint.param2 = 2; // Acceptance radius in meters
    waypoint.param3 = 0; //  0 to pass through the WP, if > 0 radius in meters to pass by WP. Positive value for clockwise orbit, negative value for counter-clockwise orbit. Allows trajectory control.
    waypoint.param4 = 0; // Desired yaw angle at MISSION (rotary wing)

    waypoint_list_[0] = waypoint;
}

const waypoint_struct_t* Mavlink_waypoint_handler::current_waypoint() const
{
    // If it is a good index
    if (current_waypoint_index_ >= 0 && current_waypoint_index_ < waypoint_count_)
    {
        return &waypoint_list_[current_waypoint_index_];
    }
    else // TODO: Return an error structure
    {
        // For now, return last waypoint structure
        return &waypoint_list_[waypoint_count_-1];
    }
}

const waypoint_struct_t* Mavlink_waypoint_handler::next_waypoint() const
{
    // If it is a good index
    if (current_waypoint_index_ >= 0 && current_waypoint_index_ < waypoint_count_)
    {
        // Check if the next waypoint exists
        if ((current_waypoint_index_ + 1) != waypoint_count_)
        {
            return &waypoint_list_[current_waypoint_index_ + 1];
        }
        else // No next waypoint, set to first
        {
            return &waypoint_list_[0];
        }
    }
    else // TODO: Return an error structure
    {
        // For now, set to last waypoint structure
        return &waypoint_list_[waypoint_count_-1];
    }
}

void Mavlink_waypoint_handler::advance_to_next_waypoint()
{
    // If the current waypoint index is the last waypoint, go to first waypoint
    if (current_waypoint_index_ == (waypoint_count_-1))
    {
        current_waypoint_index_ = 0;
    }
    else // Update current in both waypoints
    {
        current_waypoint_index_++;
    }
}

local_position_t Mavlink_waypoint_handler::curent_waypoint_local_position(global_position_t origin) const
{
    // Get the current waypoint
    waypoint_struct_t wpt = current_waypoint();

    return convert_waypoint_to_local_position(&wpt, origin);
}

local_position_t Mavlink_waypoint_handler::convert_waypoint_to_local_position(waypoint_struct_t* wpt, global_position_t origin) const
{
    // Get the local position from this waypoint
    global_position_t waypoint_global;
    local_position_t wpt_local_pos;
    global_position_t origin_relative_alt;

    for (uint8_t i = 0; i < 3; i++)
    {
        wpt_local_pos.pos[i] = 0.0f;
    }
    wpt_local_pos.origin = origin;
    wpt_local_pos.heading = maths_deg_to_rad(wpt->param4);

    switch (wpt->frame)
    {
        case MAV_FRAME_GLOBAL:
            waypoint_global.latitude    = wpt->x;
            waypoint_global.longitude   = wpt->y;
            waypoint_global.altitude    = wpt->z;
            waypoint_global.heading     = maths_deg_to_rad(wpt->param4);
            wpt_local_pos = coord_conventions_global_to_local_position(waypoint_global, origin);

            print_util_dbg_print("waypoint_global: lat (x1e7):");
            print_util_dbg_print_num(waypoint_global.latitude * 10000000, 10);
            print_util_dbg_print(" long (x1e7):");
            print_util_dbg_print_num(waypoint_global.longitude * 10000000, 10);
            print_util_dbg_print(" alt (x1000):");
            print_util_dbg_print_num(waypoint_global.altitude * 1000, 10);
            print_util_dbg_print(" waypoint_coor: x (x100):");
            print_util_dbg_print_num(wpt_local_pos.pos[X] * 100, 10);
            print_util_dbg_print(", y (x100):");
            print_util_dbg_print_num(wpt_local_pos.pos[Y] * 100, 10);
            print_util_dbg_print(", z (x100):");
            print_util_dbg_print_num(wpt_local_pos.pos[Z] * 100, 10);
            print_util_dbg_print(" localOrigin lat (x1e7):");
            print_util_dbg_print_num(origin.latitude * 10000000, 10);
            print_util_dbg_print(" long (x1e7):");
            print_util_dbg_print_num(origin.longitude * 10000000, 10);
            print_util_dbg_print(" alt (x1000):");
            print_util_dbg_print_num(origin.altitude * 1000, 10);
            print_util_dbg_print("\r\n");
            break;

        case MAV_FRAME_LOCAL_NED:
            wpt_local_pos.pos[X] = wpt->x;
            wpt_local_pos.pos[Y] = wpt->y;
            wpt_local_pos.pos[Z] = wpt->z;
            wpt_local_pos.heading = maths_deg_to_rad(wpt->param4);
            wpt_local_pos.origin = coord_conventions_local_to_global_position(wpt_local_pos);
            break;

        case MAV_FRAME_MISSION:
            // Problem here: rec is not defined here
            //mavlink_msg_mission_ack_send(MAVLINK_COMM_0,rec->msg.sysid,rec->msg.compid,MAV_CMD_ACK_ERR_NOT_SUPPORTED);
            break;
        case MAV_FRAME_GLOBAL_RELATIVE_ALT:
            waypoint_global.latitude = wpt->x;
            waypoint_global.longitude = wpt->y;
            waypoint_global.altitude = wpt->z;
            waypoint_global.heading     = maths_deg_to_rad(wpt->param4);

            origin_relative_alt = origin;
            origin_relative_alt.altitude = 0.0f;
            wpt_local_pos = coord_conventions_global_to_local_position(waypoint_global, origin_relative_alt);

            wpt_local_pos.heading = maths_deg_to_rad(wpt->param4);

            print_util_dbg_print("LocalOrigin: lat (x1e7):");
            print_util_dbg_print_num(origin_relative_alt.latitude * 10000000, 10);
            print_util_dbg_print(" long (x1e7):");
            print_util_dbg_print_num(origin_relative_alt.longitude * 10000000, 10);
            print_util_dbg_print(" global alt (x1000):");
            print_util_dbg_print_num(origin.altitude * 1000, 10);
            print_util_dbg_print(" waypoint_coor: x (x100):");
            print_util_dbg_print_num(wpt_local_pos.pos[X] * 100, 10);
            print_util_dbg_print(", y (x100):");
            print_util_dbg_print_num(wpt_local_pos.pos[Y] * 100, 10);
            print_util_dbg_print(", z (x100):");
            print_util_dbg_print_num(wpt_local_pos.pos[Z] * 100, 10);
            print_util_dbg_print("\r\n");

            break;
        case MAV_FRAME_LOCAL_ENU:
            // Problem here: rec is not defined here
            //mavlink_msg_mission_ack_send(MAVLINK_COMM_0,rec->msg.sysid,rec->msg.compid,MAV_CMD_ACK_ERR_NOT_SUPPORTED);
            break;

    }

    return wpt_local_pos;
}

static waypoint_local_struct_t Mavlink_waypoint_handler::convert_to_waypoint_local_struct(Mavlink_waypoint_handler::waypoint_struct_t* current_waypoint, global_position_t origin, dubin_state_t* dubin_state)
{
    global_position_t waypoint_global;
    waypoint_local_struct_t wpt;
    local_position_t waypoint_coor;
    global_position_t origin_relative_alt;

    for (uint8_t i = 0; i < 3; i++)
    {
        waypoint_coor.pos[i] = 0.0f;
    }
    waypoint_coor.origin = origin;
    waypoint_coor.heading = maths_deg_to_rad(current_waypoint->param4);

    switch (current_waypoint->frame)
    {
        case MAV_FRAME_GLOBAL:
            waypoint_global.latitude    = current_waypoint->x;
            waypoint_global.longitude   = current_waypoint->y;
            waypoint_global.altitude    = current_waypoint->z;
            waypoint_global.heading     = maths_deg_to_rad(current_waypoint->param4);
            waypoint_coor = coord_conventions_global_to_local_position(waypoint_global, origin);

            print_util_dbg_print("waypoint_global: lat (x1e7):");
            print_util_dbg_print_num(waypoint_global.latitude * 10000000, 10);
            print_util_dbg_print(" long (x1e7):");
            print_util_dbg_print_num(waypoint_global.longitude * 10000000, 10);
            print_util_dbg_print(" alt (x1000):");
            print_util_dbg_print_num(waypoint_global.altitude * 1000, 10);
            print_util_dbg_print(" waypoint_coor: x (x100):");
            print_util_dbg_print_num(waypoint_coor.pos[X] * 100, 10);
            print_util_dbg_print(", y (x100):");
            print_util_dbg_print_num(waypoint_coor.pos[Y] * 100, 10);
            print_util_dbg_print(", z (x100):");
            print_util_dbg_print_num(waypoint_coor.pos[Z] * 100, 10);
            print_util_dbg_print(" localOrigin lat (x1e7):");
            print_util_dbg_print_num(origin.latitude * 10000000, 10);
            print_util_dbg_print(" long (x1e7):");
            print_util_dbg_print_num(origin.longitude * 10000000, 10);
            print_util_dbg_print(" alt (x1000):");
            print_util_dbg_print_num(origin.altitude * 1000, 10);
            print_util_dbg_print("\r\n");
            break;

        case MAV_FRAME_LOCAL_NED:
            waypoint_coor.pos[X] = current_waypoint->x;
            waypoint_coor.pos[Y] = current_waypoint->y;
            waypoint_coor.pos[Z] = current_waypoint->z;
            waypoint_coor.heading = maths_deg_to_rad(current_waypoint->param4);
            waypoint_coor.origin = coord_conventions_local_to_global_position(waypoint_coor);
            break;

        case MAV_FRAME_MISSION:
            // Problem here: rec is not defined here
            //mavlink_msg_mission_ack_send(MAVLINK_COMM_0,rec->msg.sysid,rec->msg.compid,MAV_CMD_ACK_ERR_NOT_SUPPORTED);
            break;
        case MAV_FRAME_GLOBAL_RELATIVE_ALT:
            waypoint_global.latitude = current_waypoint->x;
            waypoint_global.longitude = current_waypoint->y;
            waypoint_global.altitude = current_waypoint->z;
            waypoint_global.heading     = maths_deg_to_rad(current_waypoint->param4);

            origin_relative_alt = origin;
            origin_relative_alt.altitude = 0.0f;
            waypoint_coor = coord_conventions_global_to_local_position(waypoint_global, origin_relative_alt);

            waypoint_coor.heading = maths_deg_to_rad(current_waypoint->param4);

            print_util_dbg_print("LocalOrigin: lat (x1e7):");
            print_util_dbg_print_num(origin_relative_alt.latitude * 10000000, 10);
            print_util_dbg_print(" long (x1e7):");
            print_util_dbg_print_num(origin_relative_alt.longitude * 10000000, 10);
            print_util_dbg_print(" global alt (x1000):");
            print_util_dbg_print_num(origin.altitude * 1000, 10);
            print_util_dbg_print(" waypoint_coor: x (x100):");
            print_util_dbg_print_num(waypoint_coor.pos[X] * 100, 10);
            print_util_dbg_print(", y (x100):");
            print_util_dbg_print_num(waypoint_coor.pos[Y] * 100, 10);
            print_util_dbg_print(", z (x100):");
            print_util_dbg_print_num(waypoint_coor.pos[Z] * 100, 10);
            print_util_dbg_print("\r\n");

            break;
        case MAV_FRAME_LOCAL_ENU:
            // Problem here: rec is not defined here
            //mavlink_msg_mission_ack_send(MAVLINK_COMM_0,rec->msg.sysid,rec->msg.compid,MAV_CMD_ACK_ERR_NOT_SUPPORTED);
            break;

    }

    wpt.waypoint = waypoint_coor;
    // WARNING: Acceptance radius (param2) is used as the waypoint radius (should be param3) for a fixed-wing
    wpt.radius = current_waypoint->param2;
    wpt.loiter_time = current_waypoint->param1;

    *dubin_state = DUBIN_INIT;

    return wpt;
}
