#pragma once
#include <stdint.h>

struct ThermistorTableEntry {
    uint16_t rawValue;
    int16_t degC;
};

struct ThermistorTable {
    uint16_t numEntries;
    uint16_t rawMax;
    const struct ThermistorTableEntry *pTable;
};

extern const struct ThermistorTable t100k_4k7_table;
