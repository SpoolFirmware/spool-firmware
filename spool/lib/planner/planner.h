#include <stdint.h>
#include <stddef.h>

typedef struct Planner *PlannerHandle;

// Implemented In Rust
PlannerHandle plannerInit(PlannerHandle handle, uint32_t numAxis, uint32_t numStepper);

// Implemented In C
void *portMallocAligned(size_t size, size_t align);
