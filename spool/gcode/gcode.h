#ifndef GCODE_H_
#define GCODE_H_
#include <stdint.h>
#include <stdbool.h>

#include "thermal/thermal.h"
#include "fix16.h"

enum GcodeKind {
    GcodeG0,
    GcodeG1,
    GcodeG28,
    GcodeG29,   // Bed Leveling
    GcodeG90,   // Set Abs Mode
    GcodeG91,   // Set Rel Mode
    GcodeG92,   // Set Position
    GcodeM82,   // Extruder Absolute
    GcodeM83,   // Extruder Relative
    GcodeM84,   // Disable Stepper
    GcodeM104,  // Set Extruder Temperature
    GcodeM105,  // Get Temp
    GcodeM106,  // Set Fan Speed
    GcodeM107,  // Fan OFF
    GcodeM108,  // Cancel Heating
    GcodeM109,  // Set E Temp and Wait
    GcodeM140,  // Set Bed
    GcodeM190,  // Set Bed and Wait
    GcodeM_IDGAF,  // IDGAF
};

enum GcodeResponseKind {
    ResponseOK,
    ResponseTemp,
};

struct GcodeXYZEF {
    fix16_t x, y, z, e, f;
    bool xSet : 1, ySet : 1, zSet : 1, eSet : 1, fSet : 1;
};

struct GcodeFanSpeed {
    fix16_t speed;
};

struct GcodeTemp {
    fix16_t sTemp;
    fix16_t rTemp;
};

struct GcodeCommand {
    enum GcodeKind kind;
    union {
        struct GcodeXYZEF xyzef;
        struct GcodeTemp temperature;
        struct GcodeFanSpeed fan;
    };
};

struct GcodeResponse {
    enum GcodeResponseKind respKind;
    union {
        struct TemperatureReport tempReport;
    };
};



#endif // GCODE_H_
