#pragma once
#include "FreeRTOS.h"
#include "platform/platform.h"
#include "task.h"
#include "queue.h"

#define GCODE_SERIAL_TIMEOUT 1000

void gcodeSerialTaskInit(void);

portTASK_FUNCTION_PROTO(gcodeSerialTask, pvParameters);
