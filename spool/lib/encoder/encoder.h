#pragma once
#include "hal/gpio.h"

#define ENCODER_BUTTON_THROTTLE 1000
#define ENCODER_ENCODER_THROTTLE 200

struct EncoderState {
	uint64_t lastButtonUs;
	uint64_t lastEncoderUs;
    int32_t position;
    uint32_t presses;
    uint32_t prevState;
};

/* please don't access state*/
struct Encoder {
    struct IOLine btn;
    struct IOLine en1;
    struct IOLine en2;
    struct EncoderState state;
};

void encoderInit(struct Encoder *drv, struct IOLine btn, struct IOLine en1,
                 struct IOLine en2);

void encoder1Callback(struct Encoder *drv, uint64_t timeUs);

void encoderButtonCallback(struct Encoder *drv, uint64_t timeUs);

void encoderGetAndClear(struct Encoder *drv, struct EncoderState *out);
