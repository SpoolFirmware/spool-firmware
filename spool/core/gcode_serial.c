#include "gcode_serial.h"
#include "gcode_parser.h"
#include "dbgprintf.h"

#define GCODE_QUEUE_SIZE 10

static struct GcodeCommand queueBuf[GCODE_QUEUE_SIZE];
StaticQueue_t gcodeQueueMeta;

const static char OK[5] = "OK\r\n";

#define STACK_SIZE 1024
static StaticTask_t gcodeSerialTaskBuf;
static StackType_t gcodeSerialStack[STACK_SIZE];
static TaskHandle_t gcodeSerialTaskHandle;

static struct GcodeParser gcodeParser;

void dumpSerialBuffer(void)
{
    char c;
    while (platformRecvCommand(&c, 1, 0) != 0)
        ;
}

QueueHandle_t gcodeSerialInit()
{
    QueueHandle_t handle =
        xQueueCreateStatic(GCODE_QUEUE_SIZE, sizeof(struct GcodeCommand),
                           (uint8_t *)queueBuf, &gcodeQueueMeta);
    configASSERT(handle);
    gcodeSerialTaskHandle = xTaskCreateStatic(
        gcodeSerialTask, "gcodeSerial", STACK_SIZE, (void *)handle,
        /* TODO figure out priorities */
        tskIDLE_PRIORITY + 2, gcodeSerialStack, &gcodeSerialTaskBuf);
    return handle;
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
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    status_t err = initParser(&gcodeParser);
    struct GcodeCommand cmd;

    WARN_ON_ERR(err);
    for (;;) {
        err = parseGcode(&gcodeParser, &cmd);
        WARN_ON_ERR(err);
        if (STATUS_OK(err)) {
            while (xQueueSend(queue, &cmd, portMAX_DELAY) != pdTRUE)
                ;
            platformSendResponse(OK, sizeof(OK) - 1);
            continue;
        }
        err = initParser(&gcodeParser);
        WARN_ON_ERR(err);
    }
}
