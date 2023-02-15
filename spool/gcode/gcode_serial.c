#include "misc.h"
#include "string.h"
#include "stdio.h"

#include "core/spool.h"

#include "gcode/gcode.h"
#include "gcode/gcode_serial.h"
#include "gcode/gcode_parser.h"

const static char OK[5] = "ok\r\n";

static TaskHandle_t gcodeSerialTaskHandle;

static struct GcodeParser gcodeParser;

void dumpSerialBuffer(void)
{
    char c;
    while (platformRecvCommand(&c, 1, 0) != 0)
        ;
}

void gcodeSerialTaskInit(void)
{
    ResponseQueue =
        xQueueCreate(PARSER_RESPONSE_QUEUE_SIZE, sizeof(struct GcodeResponse));
    if (ResponseQueue == NULL) {
        panic();
    }

    if (xTaskCreate(gcodeSerialTask, "gcodeSerial", 1024, NULL,
                    tskIDLE_PRIORITY + 1, &gcodeSerialTaskHandle) != pdTRUE) {
        panic();
    }
}

status_t receiveChar(char *c)
{
    *c = '\0';
    while (platformRecvCommand(c, 1, portMAX_DELAY) == 0 || *c == '\0')
        ;
    return StatusOk;
}

static void waitAndRespond(void)
{
    char printBuffer[64];
    struct GcodeResponse resp;
    xQueueReceive(ResponseQueue, &resp, portMAX_DELAY);
    switch (resp.respKind) {
    case ResponseOK:
        platformSendResponse(OK, sizeof(OK) - 1);
        break;
    case ResponseTemp:
        snprintf(printBuffer, 64, "ok T:%d B:%d\r\n",
            resp.tempReport.extruders[0],
            resp.tempReport.bed);
        platformSendResponse(printBuffer, strlen(printBuffer));
        break;
    }
}

portTASK_FUNCTION(gcodeSerialTask, pvParameters)
{
    status_t err = initParser(&gcodeParser);
    struct GcodeCommand cmd;

    WARN_ON_ERR(err);
    for (;;) {
        err = parseGcode(&gcodeParser, &cmd);
        WARN_ON_ERR(err);
        if (STATUS_OK(err)) {
            switch (cmd.kind) {
            case GcodeG0:
            case GcodeG1:
            case GcodeG28:
            case GcodeG90:
            case GcodeG91:
            case GcodeG92:
            case GcodeM82:
            case GcodeM83:
                xQueueSend(MotionPlannerTaskQueue, &cmd, portMAX_DELAY);
                platformSendResponse(OK, sizeof(OK) - 1);
                break;
            case GcodeM84:
                platformDisableStepper(0xFF);
                platformSendResponse(OK, sizeof(OK) - 1);
                break;
            case GcodeM104:
            case GcodeM105:
            case GcodeM106:
            case GcodeM107:
            case GcodeM108:
            case GcodeM109:
                xQueueSend(ThermalTaskQueue, &cmd, portMAX_DELAY);
                waitAndRespond();
                break;
            case GcodeM_IDGAF:
                platformSendResponse(OK, sizeof(OK) - 1);
                break;
            default:
                panic();
                platformSendResponse(OK, sizeof(OK) - 1);
                break;
            }
            continue;
        }
        err = initParser(&gcodeParser);
        WARN_ON_ERR(err);
    }
}
