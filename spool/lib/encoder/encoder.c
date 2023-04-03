#include "encoder.h"
#include "dbgprintf.h"
#include "error.h"
#include "FreeRTOS.h"
#include "platform/platform.h"

void encoderInit(struct Encoder *drv, struct IOLine btn, struct IOLine en1,
                 struct IOLine en2)
{
	drv->btn = btn;
	drv->en1 = en1;
	drv->en2 = en2;
	drv->state.position = 0;
	drv->state.presses = 0;
    drv->state.prevState = 0;
    halGpioEnableInterrupt(en1, GpioExtiModeFallingEdge | GpioExtiModeRisingEdge);
    halGpioEnableInterrupt(btn, GpioExtiModeFallingEdge);
}

void encoder1Callback(struct Encoder *drv, uint64_t timeUs)
{
    uint8_t en1 = halGpioRead(drv->en1);
    uint8_t en2 = halGpioRead(drv->en2);
    if (timeUs < drv->state.lastEncoderUs + ENCODER_ENCODER_THROTTLE) {
	    goto encoder1Callback_exit;
    }
    uint32_t s = drv->state.prevState & 0x3;
    if (en1) s |= 4;
    if (en2) s |= 8;

    switch (s) {
        case 3: case 12:
            drv->state.position += 1; break;
        case 6: case 9:
            drv->state.position -= 1; break;
        default:
            break;
            // panic();
    }
    drv->state.prevState = s >> 2;
    dbgPrintf("value %d\n", drv->state.position);

encoder1Callback_exit:
    drv->state.lastEncoderUs = timeUs;
}

void encoderButtonCallback(struct Encoder *drv, uint64_t timeUs)
{
	if (timeUs < drv->state.lastButtonUs + ENCODER_BUTTON_THROTTLE) {
		return;
	}
	drv->state.lastButtonUs = timeUs;
	drv->state.presses++;
	dbgPrintf("button\n");
}

void encoderGetAndClear(struct Encoder *drv, struct EncoderState *out)
{
	portENTER_CRITICAL();
	*out = drv->state;
	drv->state.position = 0;
	drv->state.presses = 0;
	portEXIT_CRITICAL();
}
