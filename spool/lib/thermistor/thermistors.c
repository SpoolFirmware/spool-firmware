#include "thermistors.h"

fix16_t thermistorEvaulate(const struct ThermistorTable *pTable, uint16_t scaledValue)
{
    int lowerBound = 0;

    for (int i = 0; i < pTable->numEntries - 1; i++) {
        if (pTable->pTable[i].rawValue > scaledValue) {
            break;
        }
        lowerBound = i;
    }

    const struct ThermistorTableEntry *const entry1 = &pTable->pTable[lowerBound];
    const struct ThermistorTableEntry *const entry2 = &pTable->pTable[lowerBound + 1];

    const uint16_t deltaX = entry2->rawValue - entry1->rawValue;
    const fix16_t deltaX_f = fix16_from_int(deltaX);
    const fix16_t deltaY_f = fix16_sub(entry2->degC, entry1->degC);
    const fix16_t slope_f = fix16_div(deltaY_f, deltaX_f);

    const int measuredOffset = ((int)scaledValue) - entry1->rawValue;
    const fix16_t measuredOffset_f = fix16_from_int(measuredOffset);

    const fix16_t temp = fix16_add(entry1->degC, fix16_mul(measuredOffset_f, slope_f));

    return temp;
}
