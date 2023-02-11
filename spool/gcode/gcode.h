#ifndef GCODE_H_
#define GCODE_H_
#include <stdint.h>
#include "fix16.h"

enum GcodeKind {
    GcodeG0,
    GcodeG1,
    GcodeG28,
    GcodeM84,
    GcodeM104,
    GcodeM105,
    GcodeM109,
};

struct GcodeXYZEF {
    fix16_t x, y, z, e, f;
};

struct GcodeTemp {
    fix16_t sTemp;
    fix16_t rTemp;
};

struct TemperatureReport {
    uint16_t bed;
    uint16_t extruders[3];
};

struct GcodeCommand {
    enum GcodeKind kind;
    union {
        struct GcodeXYZEF xyzef;
        struct GcodeTemp temperature;
    };
};

struct GcodeResponse {
    enum GcodeKind kind;
    union {
        struct TemperatureReport tempReport;
    };
};



#endif // GCODE_H_
