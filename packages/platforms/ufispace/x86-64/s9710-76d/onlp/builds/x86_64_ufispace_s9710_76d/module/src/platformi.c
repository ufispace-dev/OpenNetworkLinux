/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 * Platform Implementation
 *
 ***********************************************************/
#include <onlp/platformi/base.h>
#include "platform_lib.h"

/**
 * @brief Return the name of the the platform implementation.
 * @note This will be called prior to any other calls into the
 * platform driver, including the onlp_platformi_sw_init() function below.
 *
 * The platform implementation name should match the current
 * ONLP platform name.
 *
 * IF the platform implementation name equals the current platform name,
 * initialization will continue.
 *
 * If the platform implementation name does not match, the following will be
 * attempted:
 *
 *    onlp_platformi_set(current_platform_name);
 * If this call is successful, initialization will continue.
 * If this call fails, platform initialization will abort().
 *
 * The onlp_platformi_set() function is optional.
 * The onlp_platformi_get() is not optional.
 */
const char* onlp_platformi_get(void)
{
    return "x86-64-ufispace-s9710-76d-r0";
}

/**
 * @brief Set the platform explicitly if necessary.
 * @param platform The platform name.
 */
int onlp_platformi_set(const char* platform)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the platform software module.
 * @note This should not touch the hardware.
 */
int onlp_platformi_sw_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Platform module hardware initialization.
 * @param flags The initialization flags.
 */
int onlp_platformi_hw_init(uint32_t flags)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Initialize the platform manager features.
 */
int onlp_platformi_manage_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Perform necessary platform fan management.
 * @note This function should automatically adjust the FAN speeds
 * according to the platform conditions.
 */
int onlp_platformi_manage_fans(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Perform necessary platform LED management.
 * @note This function should automatically adjust the LED indicators
 * according to the platform conditions.
 */
int onlp_platformi_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
