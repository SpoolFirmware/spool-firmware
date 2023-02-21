#pragma once

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

struct TemperatureReport {
    uint16_t bed;
    uint16_t bedTarget;
    uint16_t extruders[1];
    uint16_t extrudersTarget[1];
};

extern TaskHandle_t thermalTaskHandle;

void thermalTaskInit(void);

void thermalGetTempReport(struct TemperatureReport *pReport);
