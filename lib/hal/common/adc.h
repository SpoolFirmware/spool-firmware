#pragma once
#include <stdint.h>

struct ADCDriver;

void halADCInit(struct ADCDriver *pD);
void halADCStart(struct ADCDriver *pD);

uint16_t halADCSample(struct ADCDriver *pD, uint32_t streamId);
