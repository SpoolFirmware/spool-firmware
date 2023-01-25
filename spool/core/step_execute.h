#pragma once
#include "FreeRTOS.h"
#include "queue.h"
#include "platform/platform.h"

void executeStep(void);
void stepExecuteSetQueue(QueueHandle_t queueHandle);