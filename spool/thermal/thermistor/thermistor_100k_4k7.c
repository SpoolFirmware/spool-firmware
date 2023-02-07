#include "thermistors.h"
#include "misc.h"

const static struct ThermistorTableEntry entries[] = {
  {  268, 150 },
  {  293, 145 },
  {  320, 141 },
  {  379, 133 },
  {  445, 122 },
  {  516, 108 },
  {  591,  98 },
  {  665,  88 },
  {  737,  79 },
  {  801,  70 },
  {  857,  55 },
  {  903,  46 },
  {  939,  39 },
  {  954,  33 },
  {  966,  27 },
  {  977,  22 },
  {  999,  15 },
  { 1004,   5 },
  { 1008,   0 },
  { 1012,  -5 },
  { 1016, -10 },
  { 1020, -15 },
};


const struct ThermistorTable t100k_4k7_table = {
    .numEntries = ARRAY_LENGTH(entries),
    .rawMax = BIT(10),
    .pTable = entries,
};
