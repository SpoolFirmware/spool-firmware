#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* -------------------------- Forward Declarations -------------------------- */
struct TMCDriver;

/* ---------------------------- Public Type Defines ------------------------- */
typedef void (*tmc_send_fn)(const struct TMCDriver *pDriver, uint8_t *pData,
                            size_t len);
typedef size_t (*tmc_recv_fn)(const struct TMCDriver *pDriver, uint8_t *pData,
                              size_t bufferLen);
enum TMCDriverState {
    TMCDriverUninitialized = 0,
    TMCDriverInitialized,
    TMCDriverError,
};

struct TMCDriver {
    enum TMCDriverState state;
    uint8_t slaveAddr;
    tmc_send_fn send;
    tmc_recv_fn recv;
};

/* ------------------------- Public Interfaces ------------------------------ */
void tmcDriverConstruct(struct TMCDriver *pDriver, uint8_t slaveAddr,
                        tmc_send_fn sendFn, tmc_recv_fn recvFn);

void tmcDriverInitialize(struct TMCDriver *pDriver);
