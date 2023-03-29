#include "encoder.h"
#include "dbgprintf.h"
#include "FreeRTOS.h"

void encoderInit(struct Encoder *drv, struct IOLine btn, struct IOLine en1,
                 struct IOLine en2)
{
    halGpioEnableInterrupt(en1, GpioExtiModeRisingEdge);
    halGpioEnableInterrupt(btn, GpioExtiModeRisingEdge);
	drv->btn = btn;
	drv->en1 = en1;
	drv->en2 = en2;
	drv->state.position = 0;
	drv->state.presses = 0;
}

void encoder1Callback(struct Encoder *drv, uint64_t timeUs)
{
    if (timeUs < drv->state.lastEncoderUs + ENCODER_ENCODER_THROTTLE) {
	return;
    }
    drv->state.lastEncoderUs = timeUs;
    if (halGpioRead(drv->en2)) {
	drv->state.position++;
    } else {
	drv->state.position--;
    }
    dbgPrintf("value %d\n", drv->state.position);
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
