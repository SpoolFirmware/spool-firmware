#include "clock.h"
#include "clock.h"
#include "drf/drf.h"
#include "chip_hal/clock.h"
#include "manual/stm32f401.h"

static void configurePllCfgr(void)
{
  /* PLL values M25 N336 P4 Q7 */
  uint32_t pll_cfgr = REG_RD32(DRF_RCC_PLLCFGR);

  /* PLL Q = 7 0111 */
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLQ3, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLQ2, _SET, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLQ1, _SET, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLQ0, _SET, pll_cfgr);

  /* PLL SRC = HSE */
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLSRC, _SET, pll_cfgr);

  /* PLL P = 4 01 */
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLP1, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLP0, _SET, pll_cfgr);

  /* PLL N = 336 101010000 */
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLN8, _SET, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLN7, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLN6, _SET, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLN5, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLN4, _SET, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLN3, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLN2, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLN1, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLN0, _CLR, pll_cfgr);

  /* PLL M = 25 011001 */
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLM5, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLM4, _SET, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLM3, _SET, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLM2, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLM1, _CLR, pll_cfgr);
  pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLM0, _SET, pll_cfgr);

  REG_WR32(DRF_RCC_PLLCFGR, pll_cfgr);
}

void configureCr(void)
{
  /* wait for PLLRDY*/
  while (FLD_TEST_DRF(_RCC, _CR, _PLLRDY, _CLR, REG_RD32(DRF_RCC_CR)))
    ;
  while (FLD_TEST_DRF(_RCC, _CR, _HSERDY, _CLR, REG_RD32(DRF_RCC_CR)))
    ;

  uint32_t cr = REG_RD32(DRF_RCC_CR);

  cr = FLD_SET_DRF(_RCC, _CR, _PLLON, _SET, cr);
  cr = FLD_SET_DRF(_RCC, _CR, _HSEON, _SET, cr);

  REG_WR32(DRF_RCC_CR, cr);
}

void configureCfgr(void)
{
    uint32_t cfgr = REG_RD32(DRF_RCC_CFGR);

    /* divide by 2*/
    cfgr = FLD_SET_DRF_NUM(_RCC, _CFGR, _PPRE1, 0b100, cfgr);

    REG_WR32(DRF_RCC_CFGR, cfgr);
}

void chipHalClockInit()
{
    configurePllCfgr();

    configureCr();
}
