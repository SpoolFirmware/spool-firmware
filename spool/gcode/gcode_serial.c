#include "core/spool.h"
#include "gcode_serial.h"
#include "gcode/gcode_parser.h"
#include "dbgprintf.h"

const static char OK[5] = "OK\r\n";

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
                xQueueSend(MotionPlannerTaskQueue, &cmd, portMAX_DELAY);
                platformSendResponse(OK, sizeof(OK) - 1);
                break;
            case GcodeM84:
                platformDisableStepper(0xFF);
                platformSendResponse(OK, sizeof(OK) - 1);
                break;
            case GcodeM104:
            case GcodeM105:
            case GcodeM109:
                xQueueSend(ThermalTaskQueue, &cmd, portMAX_DELAY);
                break;
            default:
                WARN();
                platformSendResponse(OK, sizeof(OK) - 1);
                break;
            }
            continue;
        }
        err = initParser(&gcodeParser);
        WARN_ON_ERR(err);
    }
}
