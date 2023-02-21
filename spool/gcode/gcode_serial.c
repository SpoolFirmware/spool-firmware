#include "misc.h"
#include "string.h"
#include "stdio.h"

#include "core/spool.h"
#include "compiler.h"
#include "error.h"

#include "gcode/gcode.h"
#include "gcode/gcode_serial.h"
#include "gcode/gcode_parser.h"

const static char OK[5] = "ok\r\n";

const static char BUSY[] = "busy:doing shit\r\n";
STATIC_ASSERT(ARRAY_SIZE(BUSY) == 18);

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

static void sendTempReport(struct TemperatureReport *pReport)
{
    char printBuffer[64];

    snprintf(printBuffer, 64, " T:%d /%d B:%d /%d @:%d B@:%d\r\n",
             pReport->extruders[0], pReport->extrudersTarget[0], pReport->bed,
             pReport->bedTarget, pReport->extrudersTarget[0],
             pReport->bedTarget);
    platformSendResponse(printBuffer, strlen(printBuffer));
}

static void waitAndRespond(void)
{
    struct GcodeResponse resp;

    while (xQueueReceive(ResponseQueue, &resp,
                         pdMS_TO_TICKS(GCODE_SERIAL_TIMEOUT)) == pdFALSE) {
        platformSendResponse(BUSY, sizeof(BUSY) - 1);
        thermalGetTempReport(&resp.tempReport);
        sendTempReport(&resp.tempReport);
    }
    switch (resp.respKind) {
    case ResponseOK:
        platformSendResponse(OK, sizeof(OK) - 1);
        break;
    case ResponseTemp:
        platformSendResponse(OK, 2);
        sendTempReport(&resp.tempReport);
        break;
    }
}

portTASK_FUNCTION(gcodeSerialTask, pvParameters)
{
    status_t err = initParser(&gcodeParser);
    uint32_t seqCounter = 0;
    struct GcodeCommand cmd;

    WARN_ON_ERR(err);
    for (;;) {
        err = parseGcode(&gcodeParser, &cmd);
        WARN_ON_ERR(err);
        if (STATUS_OK(err)) {
            cmd.seq.seqNumber = seqCounter++;

            switch (cmd.kind) {
            case GcodeG0:
            case GcodeG1:
            case GcodeG28:
            case GcodeG29:
            case GcodeG90:
            case GcodeG91:
            case GcodeG92:
            case GcodeM82:
            case GcodeM83:
            case GcodeM84:
                xQueueSend(MotionPlannerTaskQueue, &cmd, portMAX_DELAY);
                platformSendResponse(OK, sizeof(OK) - 1);
                break;
            /* pretend ok, wait for sync */
            case GcodeM107:
            case GcodeM106:
            case GcodeM108:
            case GcodeM140:
            case GcodeM104: {
                platformSendResponse(OK, sizeof(OK) - 1);
                xQueueSend(ThermalTaskQueue, &cmd, portMAX_DELAY);
                struct GcodeCommand syncCmd = { .kind = GcodeISRSync };
                syncCmd.seq.seqNumber = seqCounter++;
                xQueueSend(MotionPlannerTaskQueue, &syncCmd, portMAX_DELAY);
                break;
            }
            /* wait to heatup */
            case GcodeM109:
            case GcodeM190: {
                xQueueSend(ThermalTaskQueue, &cmd, portMAX_DELAY);
                struct GcodeCommand syncCmd = { .kind = GcodeISRSync };
                syncCmd.seq.seqNumber = seqCounter++;
                xQueueSend(MotionPlannerTaskQueue, &syncCmd, portMAX_DELAY);
                waitAndRespond();
                break;
            }
            case GcodeM105:
                xQueueSend(ThermalTaskQueue, &cmd, portMAX_DELAY);
                waitAndRespond();
                break;
            case GcodeM_IDGAF:
                platformSendResponse(OK, sizeof(OK) - 1);
                break;
            default:
                panic();
                break;
            }
            continue;
        }
        err = initParser(&gcodeParser);
        WARN_ON_ERR(err);
    }
}
