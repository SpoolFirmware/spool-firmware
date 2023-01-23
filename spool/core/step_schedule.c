#include "step_schedule.h"
#include <stdint.h>
#include "platform/platform.h"

struct StepTick queueBuf[STEP_QUEUE_SIZE];

StaticQueue_t stepQueue;

QueueHandle_t stepTaskInit(void)
{
    return xQueueCreateStatic(STEP_QUEUE_SIZE, sizeof(struct StepTick),
                              (uint8_t *)queueBuf, &stepQueue);
    ;
}

/* 1/32 microstepping */
/* 200 steps per rotation */
/* 0.00589048125mm per microstepping */
static uint32_t stepPerMM = 170; /* ish, TODO fix inaccuracy */
static uint32_t maxVelX = 10; /* mm/s */
static uint32_t maxAccX = 1000; /* */

static void moveX(void)
{
    /* moving both motors by 1mm */
    setStepper(0b11, 0b11, 0b11);
}

portTASK_FUNCTION(stepScheduleTask, pvParameters)
{
    QueueHandle_t *queue = (QueueHandle_t *)pvParameters;

}
