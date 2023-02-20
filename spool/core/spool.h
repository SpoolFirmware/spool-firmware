#pragma once

#include "misc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ------------------ Common Constants ---------------- */
#define SECONDS_PER_MIN 60

/*!
 * Thermal Task Inbound Queue
 */
#define THERMAL_TASK_QUEUE_SIZE 2
extern QueueHandle_t ThermalTaskQueue;

/*!
 * Parser Task Inbound Queue
 */
#define PARSER_RESPONSE_QUEUE_SIZE 1
extern QueueHandle_t ResponseQueue;

/*
 * Planner Task Inbound (GCode) Queue
 */
#define MOTION_PLANNER_TASK_QUEUE_SIZE 4
extern QueueHandle_t MotionPlannerTaskQueue;

/*!
 * Step Execution Queue, not really a task but
 * the ISR is the consumer.
 */
#define STEPPER_EXECUTION_QUEUE_SIZE 20
extern QueueHandle_t StepperExecutionQueue;

#define PLATFORM_FEATURE_ENABLED(feature) platformFeature ## feature
