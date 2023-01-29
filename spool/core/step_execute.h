#pragma once
#include "FreeRTOS.h"
#include "queue.h"
#include "platform/platform.h"

void stepExecuteHalt(void);
void stepExecuteHaltSync(void);

void stepExecuteSetQueue(QueueHandle_t queueHandle);
