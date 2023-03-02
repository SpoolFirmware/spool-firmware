#include "string.h"
#include "spi.h"
#include "misc.h"
#include "manual/mcu.h"
#include "manual/irq.h"
#include "FreeRTOS.h"

#define HAL_WR32(DEV, REG, VAL) REG_WR32(((DEV)->base) + DRF_SPI1_##REG, (VAL))
#define HAL_RD32(DEV, REG)      REG_RD32(((DEV)->base) + DRF_SPI1_##REG)

void halSpiInit(struct SPIDevice *pDevice, const struct SPIConfig *pConfig)
{
    configASSERT(pDevice);
    configASSERT(pDevice->base);

    memset(&pDevice->inner, 0, sizeof(pDevice->inner));
    pDevice->inner.smphrHandle = xSemaphoreCreateBinaryStatic(&pDevice->inner.smphr);

    HAL_WR32(pDevice, CR1, 0); // Disable SPI

    uint32_t br;
    // Figure out what the BR should be
    for (br = 0; br < 7; br++) {
        if (((pConfig->clkSpeed) >> (br + 1)) <= pConfig->baudrate) {
            break;
        }
    }

    // TODO DuplexMode
    configASSERT(pConfig->duplexMode == SPIDuplexMode_FullDuplex);
    uint32_t cr1 = DRF_NUM(_SPI1, _CR1, _BR, br);

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

    cr1 |= DRF_DEF(_SPI1, _CR1, _SSI, _SLAVE_NOT_SELECTED);
    cr1 |= DRF_DEF(_SPI1, _CR1, _SSM, _ENABLED);

    HAL_WR32(pDevice, CR1, cr1);

    uint32_t cr2 = 0;
    cr2 |= DRF_DEF(_SPI1, _CR2, _SSOE, _ENABLED);
    HAL_WR32(pDevice, CR2, cr2);
}

void halSpiStart(struct SPIDevice *pDevice)
{
    // Enable SPI Periph
    HAL_WR32(pDevice, CR1,
             FLD_SET_DRF(_SPI1, _CR1, _SPE, _ENABLED,
                         HAL_RD32(pDevice, CR1)));
}

void halSpiSend(struct SPIDevice *pDevice, const void *data, uint32_t len)
{
    halSpiXchg(pDevice, data, NULL, len);
}

void s_spiSendByte(struct SPIDevice *pDev)
{
    uint8_t out = 0xFF;
    if (pDev->inner.txCnt > 0) {
        if (pDev->inner.txBuffer != NULL) {
            out = *(const uint8_t *)pDev->inner.txBuffer;
            pDev->inner.txBuffer = ((const uint8_t *)pDev->inner.txBuffer) + 1;
        }
        pDev->inner.txCnt--;
    }
    HAL_WR32(pDev, DR, out);
}

void halSpiXchg(struct SPIDevice *pDevice, const void *pDataOut, void *pDataIn,
                uint32_t len)
{
    pDevice->inner.txBuffer = pDataOut;
    pDevice->inner.rxBuffer = pDataIn;
    pDevice->inner.txCnt = len;
    pDevice->inner.rxCnt = len;

    // Notify
    portENTER_CRITICAL();
    const uint32_t sr = HAL_RD32(pDevice, SR);
    if (FLD_TEST_DRF(_SPI1, _SR, _RXNE, _NOT_EMPTY, sr)) {
        HAL_RD32(pDevice, DR);
    }
    HAL_WR32(pDevice, CR2,
             FLD_SET_DRF(_SPI1, _CR2, _TXEIE, _NOT_MASKED,
                         HAL_RD32(pDevice, CR2)));
    HAL_WR32(pDevice, CR2,
             FLD_SET_DRF(_SPI1, _CR2, _RXNEIE, _NOT_MASKED,
                         HAL_RD32(pDevice, CR2)));
    if (FLD_TEST_DRF(_SPI1, _SR, _TXE, _EMPTY, sr)) {
        s_spiSendByte(pDevice);
    }
    portEXIT_CRITICAL();

    xSemaphoreTake(pDevice->inner.smphrHandle, portMAX_DELAY);
}

void halSpiWaitTxIdle(struct SPIDevice *pDevice)
{
    while (FLD_TEST_DRF(_SPI1, _SR, _BSY, _BUSY,
                        HAL_RD32(pDevice, SR)))
        ;
}

void halSpiLLServiceInterrupt(struct SPIDevice *pDev)
{
    uint32_t sr = HAL_RD32(pDev, SR);
    if (pDev->inner.rxCnt > 0) {
        if (FLD_TEST_DRF(_SPI1, _SR, _RXNE, _NOT_EMPTY, sr)) {
            uint8_t in = (uint8_t)HAL_RD32(pDev, DR);
            if (pDev->inner.rxCnt > 0) {
                if (pDev->inner.rxBuffer != NULL) {
                    *(uint8_t *)pDev->inner.rxBuffer = in;
                    pDev->inner.rxBuffer = ((uint8_t *)pDev->inner.rxBuffer) + 1;
                }
                pDev->inner.rxCnt--;
            }
        }
    } else {
        HAL_WR32(pDev, CR2,
                 FLD_SET_DRF(_SPI1, _CR2, _RXNEIE, _MASKED,
                             HAL_RD32(pDev, CR2)));
    }
    if (pDev->inner.txCnt > 0) {
        if (FLD_TEST_DRF(_SPI1, _SR, _TXE, _EMPTY, sr)) {
            s_spiSendByte(pDev);
        }
    } else {
        HAL_WR32(pDev, CR2,
                 FLD_SET_DRF(_SPI1, _CR2, _TXEIE, _MASKED,
                             HAL_RD32(pDev, CR2)));
    }
    if (pDev->inner.txCnt == 0 && pDev->inner.rxCnt == 0) {
        xSemaphoreGiveFromISR(pDev->inner.smphrHandle, NULL);
    }
}
