#pragma once
#include <stdint.h>
#include "fix16.h"

struct ThermistorTableEntry {
    /**
     * @brief   Raw ADC value, 16-bit resolution
     */
    uint16_t rawValue;
    fix16_t degC;
};

struct ThermistorTable {
    uint16_t numEntries;
    const struct ThermistorTableEntry *pTable;
};

fix16_t thermistorEvaulate(const struct ThermistorTable *pTable, uint16_t scaledValue);

/* -------------------------- Availiable Tables ----------------------------- */
extern const struct ThermistorTable t100k_4k7_table;
