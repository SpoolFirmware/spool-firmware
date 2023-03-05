#include "misc.h"
#include "string.h"
#include "stdio.h"

#include "core/spool.h"
#include "compiler.h"
#include "error.h"

#include "gcode/gcode.h"
#include "gcode/gcode_serial.h"
#include "gcode/gcode_parser.h"

const static char OK[5] = "ok\n";

const static char BUSY[] = "busy: processing\n";

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

    int len = snprintf(printBuffer, 64, " T:%d /%d B:%d /%d @:%d B@:%d\n",
                       pReport->extruders[0], pReport->extrudersTarget[0],
                       pReport->bed, pReport->bedTarget,
                       pReport->extrudersTarget[0], pReport->bedTarget);
    platformSendResponse(printBuffer, len);
}

static void waitAndRespond(void)
{
    struct GcodeResponse resp;

    while (xQueueReceive(ResponseQueue, &resp,
                         pdMS_TO_TICKS(GCODE_SERIAL_TIMEOUT)) == pdFALSE) {
        platformSendResponse(BUSY, strlen(BUSY));
        thermalGetTempReport(&resp.tempReport);
        sendTempReport(&resp.tempReport);
    }
    switch (resp.respKind) {
    case ResponseOK:
        platformSendResponse(OK, strlen(OK));
        break;
    case ResponseTemp:
        platformSendResponse(OK, 2);
        sendTempReport(&resp.tempReport);
        break;
    default:
        panic();
        break;
    }
}

portTASK_FUNCTION(gcodeSerialTask, pvParameters)
{
    status_t err = initParser(&gcodeParser);
    uint32_t seqCounter = 0;
    struct GcodeCommand cmd;
    struct GcodeCommand syncCmd = { .kind = GcodeISRSync };

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
            case GcodeM101:
            case GcodeM103:
                xQueueSend(MotionPlannerTaskQueue, &cmd, portMAX_DELAY);
                break;
            /* pretend ok, wait for sync */
            case GcodeM104:
            case GcodeM106:
            case GcodeM107:
            case GcodeM108:
            case GcodeM140: {
                xQueueSend(ThermalTaskQueue, &cmd, portMAX_DELAY);
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
                break;
            }
            case GcodeM105:
                xQueueSend(ThermalTaskQueue, &cmd, portMAX_DELAY);
                break;
            case GcodeM_IDGAF:
                break;
            default:
                panic();
                break;
            }
            if (gcodeIsBlocking(cmd.kind)) {
                waitAndRespond();
            } else {
                platformSendResponse(OK, strlen(OK));
            }
            continue;
        }
        err = initParser(&gcodeParser);
        WARN_ON_ERR(err);
    }
}
