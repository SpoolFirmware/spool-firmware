#pragma once

#include <stdint.h>

struct SPIDevice;

enum SPIMode 
{
    SPIMode_Slave = 0,
    SPIMode_Master,
};

enum SPIFrameFormat
{
    SPIFrameFormat_MSBFirst = 0,
    SPIFrameFormat_LSBFirst,
};

enum SPIFrameSize
{
    SPIFrameSize_8Bit = 0,
    SPIFrameSize_16Bit,
};

enum SPIDuplexMode {
    SPIDuplexMode_FullDuplex = 0,
    SPIDuplexMode_RxOnly,
    SPIDuplexMode_TxOnly,
};

enum SPIClockPolarity {
    SPIClockPolarity_IdleLow = 0,
    SPIClockPolarity_IdleHigh,
};

enum SPIClockPhase {
    SPIClockPhase_FirstEdge = 0,
    SPIClockPhase_SecondEdge,
};


struct SPIConfig 
{
    enum SPIMode mode;
    enum SPIFrameFormat frameFormat;
    enum SPIFrameSize frameSize;
    enum SPIDuplexMode duplexMode;
    enum SPIClockPolarity clockPolarity;
    enum SPIClockPhase clockPhase;
    uint32_t baudrate;
    uint32_t clkSpeed;
};

void halSpiInit(struct SPIDevice *pDevice, const struct SPIConfig *pConfig);
void halSpiStart(struct SPIDevice *pDevice);
void halSpiSend(struct SPIDevice *pDevice, const void* data, uint32_t len);
void halSpiXchg(struct SPIDevice *pDevice, const void* pDataOut, void* pDataIn, uint32_t len);
void halSpiWaitTxIdle(struct SPIDevice *pDevice);
