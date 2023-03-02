/******************************************************************************/
/**
@file		sd_spi_platform_dependencies.cpp
@author     Wade Penson
@author		Codetector
@date		Mar 1, 2023
@brief      Interface for the dependencies needed by the SD SPI library.
@copyright  Copyright 2015 Wade Penson

@license    Licensed under the Apache License, Version 2.0 (the "License");
            you may not use this file except in compliance with the License.
            You may obtain a copy of the License at

              http://www.apache.org/licenses/LICENSE-2.0

            Unless required by applicable law or agreed to in writing, software
            distributed under the License is distributed on an "AS IS" BASIS,
            WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
            implied. See the License for the specific language governing
            permissions and limitations under the License.
*/
/******************************************************************************/

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

uint32_t sd_spi_millis(void);

void sd_spi_set_cs(uint8_t high);

void sd_spi_begin_transaction(uint32_t transfer_speed_hz);

void sd_spi_end_transaction(void);

void sd_spi_send_bytes(const uint8_t *bytes, uint32_t len);
void sd_spi_send_byte(uint8_t byte);

void sd_spi_receive_bytes(uint8_t *bytes, uint32_t len);
uint8_t sd_spi_receive_byte(void);

#if defined(__cplusplus)
}
#endif
