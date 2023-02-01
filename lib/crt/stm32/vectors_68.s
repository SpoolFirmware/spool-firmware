        .syntax unified
        .cpu cortex-m3
        .thumb

        .section .vectors, "ax"
        .align 4
        .globl _vectors
_vectors:
        .long __main_stack_end__
        .long Reset_Handler
        .long NMI_Handler
        .long HardFault_Handler
        .long MemManage_Handler
        .long BusFault_Handler
        .long UsageFault_Handler
        .long SecureFault_Handler
        .long Vector20
        .long Vector24
        .long Vector28
        .long SVC_Handler
        .long DebugMon_Handler
        .long Vector34
        .long PendSV_Handler
        .long SysTick_Handler
        .long Vector40, Vector44, Vector48, Vector4C

        .long Vector50, Vector54, Vector58, Vector5C


        .long Vector60, Vector64, Vector68, Vector6C


        .long Vector70, Vector74, Vector78, Vector7C


        .long Vector80, Vector84, Vector88, Vector8C


        .long Vector90, Vector94, Vector98, Vector9C


        .long VectorA0, VectorA4, VectorA8, VectorAC


        .long VectorB0, VectorB4, VectorB8, VectorBC


        .long VectorC0, VectorC4, VectorC8, VectorCC


        .long VectorD0, VectorD4, VectorD8, VectorDC


        .long VectorE0, VectorE4, VectorE8, VectorEC


        .long VectorF0, VectorF4, VectorF8, VectorFC


        .long Vector100, Vector104, Vector108, Vector10C


        .long Vector110, Vector114, Vector118, Vector11C


        .long Vector120, Vector124, Vector128, Vector12C


        .long Vector130, Vector134, Vector138, Vector13C


        .long Vector140, Vector144, Vector148, Vector14C
# 256 "vectors.s.template"
        .text

        .align 2
        .thumb_func
        .globl Reset_Handler
Reset_Handler:
         b _crt0_entry

        .thumb_func
        .weak NMI_Handler
        .weak HardFault_Handler
        .weak MemManage_Handler
        .weak BusFault_Handler
        .weak UsageFault_Handler
        .weak SecureFault_Handler
        .weak Vector20
        .weak Vector24
        .weak Vector28
        .weak SVC_Handler
        .weak DebugMon_Handler
        .weak Vector34
        .weak PendSV_Handler
        .weak SysTick_Handler
        .weak Vector40, Vector44, Vector48, Vector4C

        .weak Vector50, Vector54, Vector58, Vector5C


        .weak Vector60, Vector64, Vector68, Vector6C


        .weak Vector70, Vector74, Vector78, Vector7C


        .weak Vector80, Vector84, Vector88, Vector8C


        .weak Vector90, Vector94, Vector98, Vector9C


        .weak VectorA0, VectorA4, VectorA8, VectorAC


        .weak VectorB0, VectorB4, VectorB8, VectorBC


        .weak VectorC0, VectorC4, VectorC8, VectorCC


        .weak VectorD0, VectorD4, VectorD8, VectorDC


        .weak VectorE0, VectorE4, VectorE8, VectorEC


        .weak VectorF0, VectorF4, VectorF8, VectorFC


        .weak Vector100, Vector104, Vector108, Vector10C


        .weak Vector110, Vector114, Vector118, Vector11C


        .weak Vector120, Vector124, Vector128, Vector12C


        .weak Vector130, Vector134, Vector138, Vector13C


        .weak Vector140, Vector144, Vector148, Vector14C
# 458 "vectors.s.template"
        .thumb_func
NMI_Handler:
        .thumb_func
HardFault_Handler:
        .thumb_func
MemManage_Handler:
        .thumb_func
BusFault_Handler:
        .thumb_func
UsageFault_Handler:
        .thumb_func
SecureFault_Handler:
        .thumb_func
Vector20:
        .thumb_func
Vector24:
        .thumb_func
Vector28:
        .thumb_func
SVC_Handler:
        .thumb_func
DebugMon_Handler:
        .thumb_func
Vector34:
        .thumb_func
PendSV_Handler:
        .thumb_func
SysTick_Handler:
        .thumb_func
Vector40:
        .thumb_func
Vector44:
        .thumb_func
Vector48:
        .thumb_func
Vector4C:
        .thumb_func
Vector50:
        .thumb_func
Vector54:
        .thumb_func
Vector58:
        .thumb_func
Vector5C:

        .thumb_func
Vector60:
        .thumb_func
Vector64:
        .thumb_func
Vector68:
        .thumb_func
Vector6C:
        .thumb_func
Vector70:
        .thumb_func
Vector74:
        .thumb_func
Vector78:
        .thumb_func
Vector7C:


        .thumb_func
Vector80:
        .thumb_func
Vector84:
        .thumb_func
Vector88:
        .thumb_func
Vector8C:
        .thumb_func
Vector90:
        .thumb_func
Vector94:
        .thumb_func
Vector98:
        .thumb_func
Vector9C:


        .thumb_func
VectorA0:
        .thumb_func
VectorA4:
        .thumb_func
VectorA8:
        .thumb_func
VectorAC:
        .thumb_func
VectorB0:
        .thumb_func
VectorB4:
        .thumb_func
VectorB8:
        .thumb_func
VectorBC:


        .thumb_func
VectorC0:
        .thumb_func
VectorC4:
        .thumb_func
VectorC8:
        .thumb_func
VectorCC:
        .thumb_func
VectorD0:
        .thumb_func
VectorD4:
        .thumb_func
VectorD8:
        .thumb_func
VectorDC:


        .thumb_func
VectorE0:
        .thumb_func
VectorE4:
        .thumb_func
VectorE8:
        .thumb_func
VectorEC:
        .thumb_func
VectorF0:
        .thumb_func
VectorF4:
        .thumb_func
VectorF8:
        .thumb_func
VectorFC:


        .thumb_func
Vector100:
        .thumb_func
Vector104:
        .thumb_func
Vector108:
        .thumb_func
Vector10C:
        .thumb_func
Vector110:
        .thumb_func
Vector114:
        .thumb_func
Vector118:
        .thumb_func
Vector11C:


        .thumb_func
Vector120:
        .thumb_func
Vector124:
        .thumb_func
Vector128:
        .thumb_func
Vector12C:
        .thumb_func
Vector130:
        .thumb_func
Vector134:
        .thumb_func
Vector138:
        .thumb_func
Vector13C:


        .thumb_func
Vector140:
        .thumb_func
Vector144:
        .thumb_func
Vector148:
        .thumb_func
Vector14C:
        .thumb_func
Vector150:
        .thumb_func
Vector154:
        .thumb_func
Vector158:
        .thumb_func
Vector15C:
# 1024 "vectors.s.template"
        bl _unhandled_exception

        .thumb_func
        .weak _unhandled_exception
_unhandled_exception:
.stay:
        b .stay
