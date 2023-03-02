#pragma once
#include <stdint.h>

/**
 * @brief hook for writing a tile to a 12864 display
 * 
 * @param x0 x start
 * @param x1 x end
 * @param y row
 * @param buf row buffer
 */
void uiWriteTile(uint8_t x0, uint8_t x1, uint8_t y,
               const uint8_t *buf);

/**
 * @brief ui for 12864 display
 */
void uiInit(void);
