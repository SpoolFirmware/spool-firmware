#include "string.h"
#include "spi.h"
#include "misc.h"
#include "manual/mcu.h"
#include "manual/irq.h"
#include "FreeRTOS.h"

#define HAL_WR32(DEV, REG, VAL) REG_WR32(((DEV)->base) + DRF_ ## REG, (VAL))
#define HAL_RD32(DEV, REG) REG_RD32(((DEV)->base) + DRF_ ## REG)


void halSpiInit(struct SPIDevice *pDevice, const struct SPIConfig *pConfig, uint32_t base)
{
    memset(pDevice, 0, sizeof(struct SPIDevice));

    pDevice->base = base;
    
    uint8_t br;
    // Figure out what the BR should be
    for(br = 0; br < 7; br++) {
        if ((pConfig->clkSpeed >> (br+1)) <= pConfig->baudrate) {
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

    HAL_WR32(pDevice, SPI1_CR2, 0); // TODO: Interrupt Handling
}

void halSpiStart(struct SPIDevice *pDevice)
{
    // Enable SPI Periph
    HAL_WR32(pDevice, SPI1_CR1, 
        FLD_SET_DRF(_SPI1, _CR1, _SPE, _ENABLED, 
            HAL_RD32(pDevice, SPI1_CR1)));
}

void halSpiSend(struct SPIDevice *pDevice, const void* data, uint32_t len)
{

}

void halSpiXchg(struct SPIDevice *pDevice, const void* pDataOut, void* pDataIn, uint32_t len)
{

}

void halSpiWaitTxIdle(struct SPIDevice *pDevice)
{

}
