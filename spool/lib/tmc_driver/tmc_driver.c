#include "tmc_driver.h"
#include "drf.h"
#include "error.h"
#include "dbgprintf.h"
#include "tmc_2209.h"

/* ------------------------- Private Defines -------------------------------- */
#define DRF_TMC_ADDRESS_ADDR     6 : 0
#define DRF_TMC_ADDRESS_RW       7 : 7
#define DRF_TMC_ADDRESS_RW_READ  0U
#define DRF_TMC_ADDRESS_RW_WRITE 1U

/* ------------------------- Private Variables ------------------------------ */
static const uint8_t sop = 0x55;

/* --------------------- Static Function Prototypes ------------------------- */
static uint8_t sCalculateCRC(const void *pData, size_t len);
static void sTmcWriteRegister(struct TMCDriver *pDriver, uint8_t regAddr,
                              uint32_t regVal);
static uint32_t sTmcReadRegister(struct TMCDriver *pDriver, uint8_t regAddr);

/* ----------------- Static Function Implementations ------------------------ */
void sTmcWriteRegister(struct TMCDriver *pDriver, uint8_t regAddr,
                       uint32_t regVal)
{
    uint8_t txBuffer[8];

    if (pDriver == NULL)
        panic();

    txBuffer[0] = sop;
    txBuffer[1] = pDriver->slaveAddr;
    txBuffer[2] = DRF_NUM(_TMC, _ADDRESS, _ADDR, regAddr) |
                  DRF_DEF(_TMC, _ADDRESS, _RW, _WRITE);
    txBuffer[3] = (regVal >> 24U) & 0xFF;
    txBuffer[4] = (regVal >> 16U) & 0xFF;
    txBuffer[5] = (regVal >> 8U) & 0xFF;
    txBuffer[6] = (regVal >> 0U) & 0xFF;
    txBuffer[7] = sCalculateCRC(txBuffer, sizeof(txBuffer) - 1);
    pDriver->send(pDriver, txBuffer, sizeof(txBuffer));
}

uint32_t sTmcReadRegister(struct TMCDriver *pDriver, uint8_t regAddr)
{
    uint8_t txBuffer[4];
    uint8_t rxBuffer[12];

    if (pDriver == NULL)
        panic();

    txBuffer[0] = sop;
    txBuffer[1] = pDriver->slaveAddr;
    txBuffer[2] = DRF_NUM(_TMC, _ADDRESS, _ADDR, regAddr) |
                  DRF_DEF(_TMC, _ADDRESS, _RW, _READ);
    txBuffer[3] = sCalculateCRC(txBuffer, sizeof(txBuffer) - 1);
    pDriver->send(pDriver, txBuffer, sizeof(txBuffer));
    pDriver->recv(pDriver, rxBuffer, sizeof(rxBuffer));
    uint32_t val = 0;
    val |= ((uint32_t)rxBuffer[7]) << 24U;
    val |= ((uint32_t)rxBuffer[8]) << 16U;
    val |= ((uint32_t)rxBuffer[9]) << 8U;
    val |= ((uint32_t)rxBuffer[10]) << 0U;
    return val;
}

uint8_t sCalculateCRC(const void *pData, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = ((const uint8_t *)pData)[i];
        for (uint8_t j = 0; j < 8; j++) {
            if ((crc >> 7) ^ (byte & 0x1)) {
                crc = (crc << 1) ^ 0x7;
            } else {
                crc = (crc << 1);
            }
            byte = byte >> 1;
        }
    }
    return crc;
}

/* ----------------- Public Function Implementations ------------------------ */
void tmcDriverConstruct(struct TMCDriver *pDriver, uint8_t slaveAddr,
                        tmc_send_fn sendFn, tmc_recv_fn recvFn)
{
    if (pDriver == NULL)
        panic();

    pDriver->state = TMCDriverUninitialized;
    pDriver->slaveAddr = slaveAddr;
    pDriver->send = sendFn;
    pDriver->recv = recvFn;
}

void tmcDriverPreInit(struct TMCDriver *pDriver)
{
    // Delay 3*8 bit time
    sTmcWriteRegister(pDriver, DRF_TMC_SLAVECONF,
                      DRF_NUM(_TMC, _SLAVECONF, _SENDDELAY, 2));
}

void tmcDriverInitialize(struct TMCDriver *pDriver)
{
    if (pDriver == NULL)
        panic();

    uint32_t gconf = sTmcReadRegister(pDriver, DRF_TMC_GCONF);
    gconf = FLD_SET_DRF(_TMC, _GCONF, _PDN_DISABLE, _DISABLED, gconf);
    gconf = FLD_SET_DRF(_TMC, _GCONF, _MSTEP_REG_SELECT, _REG, gconf);
    sTmcWriteRegister(pDriver, DRF_TMC_GCONF, gconf);

    uint32_t chopConf = sTmcReadRegister(pDriver, DRF_TMC_CHOPCONF);
    chopConf = FLD_SET_DRF(_TMC, _CHOPCONF, _MRES, _16, chopConf);
    sTmcWriteRegister(pDriver, DRF_TMC_CHOPCONF, chopConf);

    pDriver->state = TMCDriverInitialized;
}

void tmcDriverSetMstep(struct TMCDriver *pDriver, uint16_t mstep)
{
}

void tmcDriverSetCurrent(struct TMCDriver *pDriver, uint8_t iRun, uint8_t iHold,
                         uint8_t iHoldDelay)
{
    uint32_t iholdirun = DRF_NUM(_TMC, _IHOLDIRUN, _IRUN, iRun) |
                         DRF_NUM(_TMC, _IHOLDIRUN, _IHOLD, iHold) |
                         DRF_NUM(_TMC, _IHOLDIRUN, _IHOLDDELAY, iHoldDelay);
    sTmcWriteRegister(pDriver, DRF_TMC_IHOLDIRUN, iholdirun);
}
