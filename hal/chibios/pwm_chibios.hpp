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
 * \file pwm_chibios.hpp
 *
 * \author MAV'RIC Team
 * \author Julien Lecoeur
 *
 * \brief Wrapper class for PWM using ChibiOS/HAL
 *
 ******************************************************************************/


#ifndef PWM_CHIBIOS_HPP_
#define PWM_CHIBIOS_HPP_

#include "hal/common/pwm.hpp"

extern "C"
{
    #include "hal.h"
}

/**
* \brief Wrapper class for PWM using ChibiOS/HAL
*/
class Pwm_chibios
{
public:

    /**
     * \brief   Enumeration of PWM channels
     */
    enum channel_id_t
    {
        CHANNEL_0 = 0,      ///< Channel 0
        CHANNEL_1 = 1,      ///< Channel 1
        CHANNEL_2 = 2,      ///< Channel 2
        CHANNEL_3 = 3,      ///< Channel 3
    };

    /**
     * \brief   Enumeration of possible alternate function for PWM
     */
    enum af_t
    {
        AF_1 = PAL_MODE_ALTERNATE(1),       ///< Alternate function 1
        AF_2 = PAL_MODE_ALTERNATE(2),       ///< Alternate function 2
        AF_3 = PAL_MODE_ALTERNATE(3),       ///< Alternate function 3
        AF_9 = PAL_MODE_ALTERNATE(9),       ///< Alternate function 9
    };

    /**
     * \brief   Configuration structure
     */
    struct conf_t
    {
        PWMDriver*      driver;                 ///< Pointer to PWM driver
        PWMConfig       config;                 ///< Driver configuration
        channel_id_t    channel;                ///< Channel ID
        ioportid_t      port;                   ///< Port
        ioportmask_t    pin;                    ///< Pin
        af_t            alternate_function;     ///< Alternate function
    };

    /**
     * \brief   Default configuration structure
     *
     *\return   Config
     */
    static inline conf_t default_config(void);

    /**
     * \brief Constructor
     *
     * \param config    Configuration structure
     */
    Pwm_chibios(conf_t config = default_config());

    /**
     * \brief   Initialize the hardware line for servos
     *
     * \return  Success
     */
    bool init(void);


    /**
     * \brief   Set pulse width
     *
     * \param  pulse_us     Pulse length in us
     *
     * \return Success
     */
    bool set_pulse_width_us(uint16_t pulse_us);


    /**
     * \brief   Set pulse period
     *
     * \param   period_us   Pulse period in us
     *
     * \return  Success
     */
    bool set_period_us(uint16_t period_us);

private:
    PWMDriver*      driver_;                 ///< Pointer to PWM driver
    PWMConfig       config_;                 ///< Driver configuration
    channel_id_t    channel_;                ///< Channel ID
    ioportid_t      port_;                   ///< Port
    ioportmask_t    pin_;                    ///< Pin
    af_t            alternate_function_;     ///< Alternate function
};


/**
 * \brief   Default configuration structure
 *
 *\return   Config
 */
Pwm_chibios::conf_t Pwm_chibios::default_config(void)
{
    conf_t conf = {};

    conf.driver = &PWMD1;
    conf.config =
    {
        10000,                                    /* 10kHz PWM clock frequency.   */
        10000,                                    /* Initial PWM period 1S.       */
        NULL,
        {
            {PWM_OUTPUT_ACTIVE_HIGH, NULL},
            {PWM_OUTPUT_DISABLED, NULL},
            {PWM_OUTPUT_DISABLED, NULL},
            {PWM_OUTPUT_DISABLED, NULL}
        },
        0,
        0
    };
    conf.channel            = CHANNEL_0;
    conf.port               = GPIOA;
    conf.pin                = GPIOA_PIN8;
    conf.alternate_function = AF_1;

    return conf;
}


#endif /* PWM_CHIBIOS_HPP_ */