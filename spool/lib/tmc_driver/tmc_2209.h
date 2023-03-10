#pragma once

#define DRF_TMC_GCONF                      0x0
#define DRF_TMC_GCONF_PDN_DISABLE          6 : 6
#define DRF_TMC_GCONF_PDN_DISABLE_ENABLED  0U
#define DRF_TMC_GCONF_PDN_DISABLE_DISABLED 1U
#define DRF_TMC_GCONF_MSTEP_REG_SELECT     7 : 7
#define DRF_TMC_GCONF_MSTEP_REG_SELECT_PIN 0U
#define DRF_TMC_GCONF_MSTEP_REG_SELECT_REG 1U
#define DRF_TMC_GSTAT                      0x1
#define DRF_TMC_IFCNT                      0x2
#define DRF_TMC_SLAVECONF                  0x3
#define DRF_TMC_SLAVECONF_SENDDELAY        11 : 8

#define DRF_TMC_IHOLDIRUN            0x10
#define DRF_TMC_IHOLDIRUN_IHOLD      4 : 0
#define DRF_TMC_IHOLDIRUN_IRUN       12 : 8
#define DRF_TMC_IHOLDIRUN_IHOLDDELAY 19 : 16

#define DRF_TMC_CHOPCONF           0x6C
#define DRF_TMC_CHOPCONF_MRES      27 : 24
#define DRF_TMC_CHOPCONF_MRES_FULL 8U
#define DRF_TMC_CHOPCONF_MRES_1    8U
#define DRF_TMC_CHOPCONF_MRES_2    7U
#define DRF_TMC_CHOPCONF_MRES_4    6U
#define DRF_TMC_CHOPCONF_MRES_8    5U
#define DRF_TMC_CHOPCONF_MRES_16   4U
#define DRF_TMC_CHOPCONF_MRES_32   3U
#define DRF_TMC_CHOPCONF_MRES_64   2U
#define DRF_TMC_CHOPCONF_MRES_128  1U
#define DRF_TMC_CHOPCONF_MRES_256  0U
