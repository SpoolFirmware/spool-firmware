#include "motion.h"

#include "platform/platform.h"
#include "step_schedule.h"
#include "step_plan_ng.h"
#include "step_execute.h"

void motionInit(QueueHandle_t gcodeCommandQueue)
{
    platformDisableStepper(0xFF);
    QueueHandle_t stepperJobQueue = stepTaskInit(gcodeCommandQueue);
    initPlanner();
    stepExecuteSetQueue(stepperJobQueue);
}
