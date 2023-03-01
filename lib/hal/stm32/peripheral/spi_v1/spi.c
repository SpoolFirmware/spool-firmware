#include "string.h"
#include "spi.h"
#include "misc.h"
#include "manual/mcu.h"
#include "manual/irq.h"
#include "FreeRTOS.h"

#define HAL_WR32(DEV, REG, VAL) REG_WR32(((DEV)->base) + DRF_##REG, (VAL))
#define HAL_RD32(DEV, REG)      REG_RD32(((DEV)->base) + DRF_##REG)

void halSpiInit(struct SPIDevice *pDevice, const struct SPIConfig *pConfig,
                uint32_t base)
{
    configASSERT(pDevice);

    memset(pDevice, 0, sizeof(struct SPIDevice));
    pDevice->smphrHandle = xSemaphoreCreateBinaryStatic(&pDevice->smphr);

    pDevice->base = base;

    uint8_t br;
    // Figure out what the BR should be
    for (br = 0; br < 7; br++) {
        if ((pConfig->clkSpeed >> (br + 1)) <= pConfig->baudrate) {
            break;
        }
    }

    // TODO DuplexMode
    configASSERT(pConfig->duplexMode == SPIDuplexMode_FullDuplex);
    uint32_t cr1 = DRF_DEF(_SPI1, _CR1, _BIDIMODE, _BIDIRECTIONAL) |
                   DRF_NUM(_SPI1, _CR1, _BR, br);

    configASSERT(pConfig->mode == SPIMode_Master); // Slave not yet supported
    if (pConfig->mode == SPIMode_Master) {
        cr1 = FLD_SET_DRF(_SPI1, _CR1, _MSTR, _MASTER, cr1);
    }
    if (pConfig->frameFormat == SPIFrameFormat_LSBFirst) {
        cr1 = FLD_SET_DRF(_SPI1, _CR1, _LSBFIRST, _LSB_FIRST, cr1);
    }
    if (pConfig->clockPolarity == SPIClockPolarity_IdleHigh) {
        cr1 = FLD_SET_DRF(_SPI1, _CR1, _CPOL, _IDLE_HIGH, cr1);
    }
    if (pConfig->clockPhase == SPIClockPhase_SecondEdge) {
        cr1 = FLD_SET_DRF(_SPI1, _CR1, _CPHA, _SECOND_EDGE, cr1);
    }
    HAL_WR32(pDevice, SPI1_CR1, cr1);

    uint32_t cr2 = DRF_DEF(_SPI1, _CR2, _RXNEIE, _NOT_MASKED);
    if (pConfig->slaveSelect == SPISlaveSelectManaged) {
        cr2 |= DRF_DEF(_SPI1, _CR2, _SSOE, _ENABLED);
    }
    HAL_WR32(pDevice, SPI1_CR2, cr2);
}

void halSpiStart(struct SPIDevice *pDevice)
{
    // Enable SPI Periph
    HAL_WR32(pDevice, SPI1_CR1,
             FLD_SET_DRF(_SPI1, _CR1, _SPE, _ENABLED,
                         HAL_RD32(pDevice, SPI1_CR1)));
}

void halSpiSend(struct SPIDevice *pDevice, const void *data, uint32_t len)
{
    halSpiXchg(pDevice, data, NULL, len);
}

void halSpiXchg(struct SPIDevice *pDevice, const void *pDataOut, void *pDataIn,
                uint32_t len)
{
    pDevice->txBuffer = pDataOut;
    pDevice->rxBuffer = pDataIn;
    if (pDataOut != NULL) {
        pDevice->txCnt = len;
    }
    if (pDataIn != NULL) {
        pDevice->rxCnt = len;
    }

    // Notify
    HAL_WR32(pDevice, SPI1_CR2,
             FLD_SET_DRF(_SPI1, _CR2, _TXEIE, _NOT_MASKED,
                         HAL_RD32(pDevice, SPI1_CR2)));

    xSemaphoreTake(pDevice->smphrHandle, portMAX_DELAY);
}

void halSpiWaitTxIdle(struct SPIDevice *pDevice)
{
    while (FLD_TEST_DRF(_SPI1, _SR, _BSY, _BUSY,
                        REG_RD32(pDevice->base + DRF_SPI1_SR)))
        ;
}

void halSpiLLServiceInterrupt(struct SPIDevice *pDev)
{
    if (pDev->txCnt > 0 || pDev->rxCnt > 0) {
        uint32_t sr = HAL_RD32(pDev, SPI1_SR);
        if (FLD_TEST_DRF(_SPI1, _SR, _RXNE, _NOT_EMPTY, sr)) {
            uint8_t in = (uint8_t)HAL_RD32(pDev, SPI1_DR);
            if (pDev->rxBuffer != NULL && pDev->rxCnt != 0) {
                *(uint8_t *)pDev->rxBuffer = in;
                pDev->rxBuffer = ((uint8_t *)pDev->rxBuffer) + 1;
                pDev->rxCnt--;
            }
        }

        if (FLD_TEST_DRF(_SPI1, _SR, _TXE, _EMPTY, sr)) {
            uint8_t out = 0;
            if (pDev->txBuffer != NULL && pDev->txCnt != 0) {
                out = *(const uint8_t *)pDev->txBuffer;
                pDev->txBuffer = ((const uint8_t *)pDev->txBuffer) + 1;
                pDev->txCnt--;
            }
            HAL_WR32(pDev, SPI1_DR, out);
        }
    } else {
        HAL_WR32(pDev, SPI1_CR2,
                 FLD_SET_DRF(_SPI1, _CR2, _TXEIE, _MASKED,
                             HAL_RD32(pDev, SPI1_CR2)));
        xSemaphoreGive(pDev->smphrHandle);
    }
}
