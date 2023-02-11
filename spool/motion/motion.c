#include "motion.h"

#include "core/spool.h"
#include "platform/platform.h"
#include "step_schedule.h"
#include "step_plan_ng.h"
#include "step_execute.h"

void motionInit(void)
{
    StepperExecutionQueue = xQueueCreate(STEPPER_EXECUTION_QUEUE_SIZE, sizeof(struct StepperJob));
    if (StepperExecutionQueue == NULL) {
        panic();
    }
    platformDisableStepper(0xFF);
    motionPlannerTaskInit();
    initPlanner();
}
