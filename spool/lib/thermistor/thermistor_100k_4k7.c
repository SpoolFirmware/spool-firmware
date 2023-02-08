#include "thermistors.h"
#include "misc.h"


// TODO: evaulate
const static struct ThermistorTableEntry entries[] = {
  {  268 << 6, F16(150 )},
  {  293 << 6, F16(145 )},
  {  320 << 6, F16(141 )},
  {  379 << 6, F16(133 )},
  {  445 << 6, F16(122 )},
  {  516 << 6, F16(108 )},
  {  591 << 6, F16( 98 )},
  {  665 << 6, F16( 88 )},
  {  737 << 6, F16( 79 )},
  {  801 << 6, F16( 70 )},
  {  857 << 6, F16( 55 )},
  {  903 << 6, F16( 46 )},
  {  939 << 6, F16( 39 )},
  {  954 << 6, F16( 33 )},
  {  966 << 6, F16( 27 )},
  {  977 << 6, F16( 22 )},
  {  999 << 6, F16( 15 )},
  { 1004 << 6, F16(  5 )},
  { 1008 << 6, F16(  0 )},
  { 1012 << 6, F16( -5 )},
  { 1016 << 6, F16(-10 )},
  { 1020 << 6, F16(-15 )},
};


const struct ThermistorTable t100k_4k7_table = {
    .numEntries = ARRAY_LENGTH(entries),
    .pTable = entries,
};
