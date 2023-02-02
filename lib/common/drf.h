#pragma once

#include <stdint.h>

/* stolen from https://github.com/NVIDIA/open-gpu-kernel-modules/blob/758b4ee8189c5198504cb1c3c5bc29027a9118a3/src/common/sdk/nvidia/inc/nvmisc.h */

/*!
 * DRF MACRO README:
 *
 * Glossary:
 *      DRF: Device, Register, Field
 *      FLD: Field
 *      REF: Reference
 *
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA                   0xDEADBEEF
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_GAMMA             27:0
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA             31:28
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA_ZERO   0x00000000
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA_ONE    0x00000001
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA_TWO    0x00000002
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA_THREE  0x00000003
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA_FOUR   0x00000004
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA_FIVE   0x00000005
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA_SIX    0x00000006
 * #define DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA_SEVEN  0x00000007
 *
 *
 * Device = _DEVICE_OMEGA
 *   This is the common "base" that a group of registers in a manual share
 *
 * Register = _REGISTER_ALPHA
 *   Register for a given block of defines is the common root for one or more fields and constants
 *
 * Field(s) = _FIELD_GAMMA, _FIELD_ZETA
 *   These are the bit ranges for a given field within the register
 *   Fields are not required to have defined constant values (enumerations)
 *
 * Constant(s) = _ZERO, _ONE, _TWO, ...
 *   These are named values (enums) a field can contain; the width of the constants should not be larger than the field width
 *
 * MACROS:
 *
 * DRF_SHIFT:
 *      Bit index of the lower bound of a field
 *      DRF_SHIFT(DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA) == 28
 *
 * DRF_SHIFT_RT:
 *      Bit index of the higher bound of a field
 *      DRF_SHIFT_RT(DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA) == 31
 *
 * DRF_MASK:
 *      Produces a mask of 1-s equal to the width of a field
 *      DRF_MASK(DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA) == 0xF (four 1s starting at bit 0)
 *
 * DRF_SHIFTMASK:
 *      Produces a mask of 1s equal to the width of a field at the location of the field
 *      DRF_SHIFTMASK(DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA) == 0xF0000000
 *
 * DRF_DEF:
 *      Shifts a field constant's value to the correct field offset
 *      DRF_DEF(_DEVICE_OMEGA, _REGISTER_ALPHA, _FIELD_ZETA, _THREE) == 0x30000000
 *
 * DRF_NUM:
 *      Shifts a number to the location of a particular field
 *      DRF_NUM(_DEVICE_OMEGA, _REGISTER_ALPHA, _FIELD_ZETA, 3) == 0x30000000
 *      NOTE: If the value passed in is wider than the field, the value's high bits will be truncated
 *
 * DRF_SIZE:
 *      Provides the width of the field in bits
 *      DRF_SIZE(DRF_DEVICE_OMEGA_REGISTER_ALPHA_FIELD_ZETA) == 4
 *
 * DRF_VAL:
 *      Provides the value of an input within the field specified
 *      DRF_VAL(_DEVICE_OMEGA, _REGISTER_ALPHA, _FIELD_ZETA, 0xABCD1234) == 0xA
 *      This is sort of like the inverse of DRF_NUM
 */

/*! Address of a register DRF_REG(_DEVICE_OMEGA, _REGISTER_ALPHA)*/
#define DRF_REG(d,r)			(DRF_BASE(DRF##d)+(DRF##d##r))

#define DRF_ISBIT(bitval, drf)               \
	((bitval != 0) ? drf )
#define DRF_BASE(drf)           (0?drf)
#define DRF_EXTENT(drf)         (1?drf)
#define DRF_SHIFT(drf) ((DRF_ISBIT(0, drf)) % 32)
#define DRF_SHIFT_RT(drf) ((DRF_ISBIT(1, drf)) % 32)
#define DRF_SIZE(drf) (DRF_EXTENT(drf) - DRF_BASE(drf) + 1U)
#define DRF_MASK(drf)   \
	(0xFFFFFFFFU >> \
	 (31 - ((DRF_ISBIT(1, drf)) % 32) + ((DRF_ISBIT(0, drf)) % 32)))
#define DRF_DEF(d, r, f, c) ((DRF##d##r##f##c) << DRF_SHIFT(DRF##d##r##f))
#define DRF_NUM(d, r, f, n) \
	(((n)&DRF_MASK(DRF##d##r##f)) << DRF_SHIFT(DRF##d##r##f))
#define DRF_SHIFTMASK(drf) (DRF_MASK(drf) << (DRF_SHIFT(drf)))
#define DRF_VAL(d, r, f, v) \
	(((v) >> DRF_SHIFT(DRF##d##r##f)) & DRF_MASK(DRF##d##r##f))

#define DRF_IDX_DEF(d,r,f,i,c)          ((DRF##d##r##f##c)<<DRF_SHIFT(DRF##d##r##f(i)))
#define DRF_IDX_NUM(d,r,f,i,n)          (((n)&DRF_MASK(DRF##d##r##f(i)))<<DRF_SHIFT(DRF##d##r##f(i)))
#define DRF_IDX_VAL(d,r,f,i,v)          (((v)>>DRF_SHIFT(DRF##d##r##f(i)))&DRF_MASK(DRF##d##r##f(i)))

#define FLD_SET_DRF(d,r,f,c,v)                  (((v) & ~DRF_SHIFTMASK(DRF##d##r##f)) | DRF_DEF(d,r,f,c))
#define FLD_SET_DRF_NUM(d,r,f,n,v)              (((v) & ~DRF_SHIFTMASK(DRF##d##r##f)) | DRF_NUM(d,r,f,n))
#define FLD_IDX_SET_DRF(d,r,f,i,c,v)            (((v) & ~DRF_SHIFTMASK(DRF##d##r##f(i))) | DRF_IDX_DEF(d,r,f,i,c))
#define FLD_IDX_SET_DRF_NUM(d,r,f,i,n,v)        (((v) & ~DRF_SHIFTMASK(DRF##d##r##f(i))) | DRF_IDX_NUM(d,r,f,i,n))

#define FLD_TEST_DRF(d,r,f,c,v)                 ((DRF_VAL(d, r, f, (v)) == DRF##d##r##f##c))
#define FLD_TEST_DRF_AND(d,r,f,c,v)             ((DRF_VAL(d, r, f, (v)) & DRF##d##r##f##c))
#define FLD_TEST_DRF_NUM(d,r,f,n,v)             ((DRF_VAL(d, r, f, (v)) == (n)))
#define FLD_IDX_TEST_DRF(d,r,f,i,c,v)           ((DRF_IDX_VAL(d, r, f, i, (v)) == DRF##d##r##f##c))

#define REG_RD32(reg)	(*((volatile uint32_t*)(reg)))
#define REG_WR32(reg, val) (*((volatile uint32_t*)(reg)) = (val))

#define REG_FLD_SET_DRF(d,r,f,c)		\
	(REG_WR32(DRF_REG(d, r), FLD_SET_DRF(d, r, f, c, REG_RD32(DRF_REG(d, r)))))
#define REG_FLD_TEST_DRF(d,r,f,c)		\
	(FLD_TEST_DRF(d,r,f,c,REG_RD32(DRF_REG(d,r))))
