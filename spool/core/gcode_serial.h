#pragma once
#include "FreeRTOS.h"
#include "platform/platform.h"
#include "task.h"
#include "queue.h"

QueueHandle_t gcodeSerialInit(void);

portTASK_FUNCTION_PROTO(gcodeSerialTask, pvParameters);
