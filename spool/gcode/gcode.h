#ifndef GCODE_H_
#define GCODE_H_
#include <stdint.h>
#include <stdbool.h>

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

enum GcodeResponseKind {
    ResponseOK,
};

struct GcodeXYZEF {
    fix16_t x, y, z, e, f;
    bool xSet : 1, ySet : 1, zSet : 1, eSet : 1, fSet : 1;
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
    enum GcodeResponseKind respKind;
    union {
        struct TemperatureReport tempReport;
    };
};



#endif // GCODE_H_
