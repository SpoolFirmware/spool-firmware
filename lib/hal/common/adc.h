#pragma once
#include <stdint.h>

struct ADCDriver;
struct ADCConfig;

void halADCInit(struct ADCDriver *pD, struct ADCConfig *pCfg);
void halADCStart(struct ADCDriver *pD);

uint8_t halADCGetResolutionBits(struct ADCDriver *pD);
uint16_t halADCSampleStream(struct ADCDriver *pD, uint32_t streamId);
