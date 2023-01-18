#include "clock.h"
#include "clock.h"
#include "drf/drf.h"
#include "chip_hal/clock.h"
#include "manual/stm32f401.h"

static void configurePllCfgr(void)
{
  /* PLL values M25 N336 P4 Q7 */
  uint32_t pll_cfgr = REG_RD32(DRF_REG(_RCC, _PLLCFGR));

  /* PLL Q = 7 0111 */
  pll_cfgr = FLD_SET_DRF_NUM(_RCC, _PLLCFGR, _PLLQ, 7, pll_cfgr);

  /* PLL SRC = HSE */
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLSRC, _HSE, pll_cfgr);

  /* PLL P = 4 01 */
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLP, _DIV4, pll_cfgr);

  /* PLL N = 336 101010000 */
  pll_cfgr = FLD_SET_DRF_NUM(_RCC, _PLLCFGR, _PLLN, 336, pll_cfgr);

  /* PLL M = 25 011001 */
  pll_cfgr = FLD_SET_DRF_NUM(_RCC, _PLLCFGR, _PLLM, 25, pll_cfgr);

  REG_WR32(DRF_REG(_RCC, _PLLCFGR), pll_cfgr);
}

void configureCr(void)
{
  uint32_t cr = REG_RD32(DRF_REG(_RCC, _CR));

  cr = FLD_SET_DRF(_RCC, _CR, _PLLON, _ON, cr);
  cr = FLD_SET_DRF(_RCC, _CR, _HSEON, _ON, cr);

  REG_WR32(DRF_REG(_RCC, _CR), cr);

  while (FLD_TEST_DRF(_RCC, _CR, _PLLRDY, _NOT_READY, REG_RD32(DRF_REG(_RCC, _CR))))
    ;
  while (FLD_TEST_DRF(_RCC, _CR, _HSERDY, _NOT_READY, REG_RD32(DRF_REG(_RCC, _CR))))
    ;
}

void configureCfgr(void)
{
    uint32_t cfgr = REG_RD32(DRF_REG(_RCC, _CFGR));

    /* divide by 2*/
    cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE1, _DIV2, cfgr);

    REG_WR32(DRF_REG(_RCC, _CFGR), cfgr);
}

void chipHalClockInit()
{
    configurePllCfgr();

    configureCfgr();

    configureCr();
}
