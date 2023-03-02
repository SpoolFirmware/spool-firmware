#pragma once
#include "lib/st7920/st7920.h"

/**
 * @brief called before ui init
 * 
 * @param drv st7920 driver
 */
void uiSt7920Init(const struct St7920 *drv);