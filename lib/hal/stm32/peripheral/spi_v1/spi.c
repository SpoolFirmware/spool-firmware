#include "spi.h"

void halSpiInit(struct SPIDevice *pDevice, const struct SPIConfig *pConfig);
void halSpiStart(struct SPIDevice *pDevice);
void halSpiSend(struct SPIDevice *pDevice, const void* data, uint32_t len);
void halSpiXchg(struct SPIDevice *pDevice, const void* pDataOut, void* pDataIn, uint32_t len);
void halSpiWaitTxIdle(struct SPIDevice *pDevice);