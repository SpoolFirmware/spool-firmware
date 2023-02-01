        .syntax unified
        .cpu cortex-m4
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


        .long Vector150, Vector154, Vector158, Vector15C


        .long Vector160, Vector164, Vector168, Vector16C


        .long Vector170, Vector174, Vector178, Vector17C


        .long Vector180, Vector184, Vector188, Vector18C


        .long Vector190, Vector194, Vector198, Vector19C


        .long Vector1A0, Vector1A4, Vector1A8, Vector1AC


        .long Vector1B0, Vector1B4, Vector1B8, Vector1BC


        .long Vector1C0, Vector1C4, Vector1C8, Vector1CC


        .long Vector1D0, Vector1D4, Vector1D8, Vector1DC


        .long Vector1E0, Vector1E4, Vector1E8, Vector1EC


        .long Vector1F0, Vector1F4, Vector1F8, Vector1FC


        .long Vector200, Vector204, Vector208, Vector20C


        .long Vector210, Vector214, Vector218, Vector21C


        .long Vector220, Vector224, Vector228, Vector22C


        .long Vector230, Vector234, Vector238, Vector23C


        .long Vector240, Vector244, Vector248, Vector24C


        .long Vector250, Vector254, Vector258, Vector25C


        .long Vector260, Vector264, Vector268, Vector26C


        .long Vector270, Vector274, Vector278, Vector27C


        .long Vector280, Vector284, Vector288, Vector28C


        .long Vector290, Vector294, Vector298, Vector29C


        .long Vector2A0, Vector2A4, Vector2A8, Vector2AC


        .long Vector2B0, Vector2B4, Vector2B8, Vector2BC


        .long Vector2C0, Vector2C4, Vector2C8, Vector2CC


        .long Vector2D0, Vector2D4, Vector2D8, Vector2DC


        .long Vector2E0, Vector2E4, Vector2E8, Vector2EC


        .long Vector2F0, Vector2F4, Vector2F8, Vector2FC


        .long Vector300, Vector304, Vector308, Vector30C


        .long Vector310, Vector314, Vector318, Vector31C


        .long Vector320, Vector324, Vector328, Vector32C


        .long Vector330, Vector334, Vector338, Vector33C


        .long Vector340, Vector344, Vector348, Vector34C


        .long Vector350, Vector354, Vector358, Vector35C


        .long Vector360, Vector364, Vector368, Vector36C


        .long Vector370, Vector374, Vector378, Vector37C


        .long Vector380, Vector384, Vector388, Vector38C


        .long Vector390, Vector394, Vector398, Vector39C


        .long Vector3A0, Vector3A4, Vector3A8, Vector3AC


        .long Vector3B0, Vector3B4, Vector3B8, Vector3BC


        .long Vector3C0, Vector3C4, Vector3C8, Vector3CC


        .long Vector3D0, Vector3D4, Vector3D8, Vector3DC


        .long Vector3E0, Vector3E4, Vector3E8, Vector3EC


        .long Vector3F0, Vector3F4, Vector3F8, Vector3FC


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


        .weak Vector150, Vector154, Vector158, Vector15C


        .weak Vector160, Vector164, Vector168, Vector16C


        .weak Vector170, Vector174, Vector178, Vector17C


        .weak Vector180, Vector184, Vector188, Vector18C


        .weak Vector190, Vector194, Vector198, Vector19C


        .weak Vector1A0, Vector1A4, Vector1A8, Vector1AC


        .weak Vector1B0, Vector1B4, Vector1B8, Vector1BC


        .weak Vector1C0, Vector1C4, Vector1C8, Vector1CC


        .weak Vector1D0, Vector1D4, Vector1D8, Vector1DC


        .weak Vector1E0, Vector1E4, Vector1E8, Vector1EC


        .weak Vector1F0, Vector1F4, Vector1F8, Vector1FC


        .weak Vector200, Vector204, Vector208, Vector20C


        .weak Vector210, Vector214, Vector218, Vector21C


        .weak Vector220, Vector224, Vector228, Vector22C


        .weak Vector230, Vector234, Vector238, Vector23C


        .weak Vector240, Vector244, Vector248, Vector24C


        .weak Vector250, Vector254, Vector258, Vector25C


        .weak Vector260, Vector264, Vector268, Vector26C


        .weak Vector270, Vector274, Vector278, Vector27C


        .weak Vector280, Vector284, Vector288, Vector28C


        .weak Vector290, Vector294, Vector298, Vector29C


        .weak Vector2A0, Vector2A4, Vector2A8, Vector2AC


        .weak Vector2B0, Vector2B4, Vector2B8, Vector2BC


        .weak Vector2C0, Vector2C4, Vector2C8, Vector2CC


        .weak Vector2D0, Vector2D4, Vector2D8, Vector2DC


        .weak Vector2E0, Vector2E4, Vector2E8, Vector2EC


        .weak Vector2F0, Vector2F4, Vector2F8, Vector2FC


        .weak Vector300, Vector304, Vector308, Vector30C


        .weak Vector310, Vector314, Vector318, Vector31C


        .weak Vector320, Vector324, Vector328, Vector32C


        .weak Vector330, Vector334, Vector338, Vector33C


        .weak Vector340, Vector344, Vector348, Vector34C


        .weak Vector350, Vector354, Vector358, Vector35C


        .weak Vector360, Vector364, Vector368, Vector36C


        .weak Vector370, Vector374, Vector378, Vector37C


        .weak Vector380, Vector384, Vector388, Vector38C


        .weak Vector390, Vector394, Vector398, Vector39C


        .weak Vector3A0, Vector3A4, Vector3A8, Vector3AC


        .weak Vector3B0, Vector3B4, Vector3B8, Vector3BC


        .weak Vector3C0, Vector3C4, Vector3C8, Vector3CC


        .weak Vector3D0, Vector3D4, Vector3D8, Vector3DC


        .weak Vector3E0, Vector3E4, Vector3E8, Vector3EC


        .weak Vector3F0, Vector3F4, Vector3F8, Vector3FC


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


        .thumb_func
Vector160:
        .thumb_func
Vector164:
        .thumb_func
Vector168:
        .thumb_func
Vector16C:
        .thumb_func
Vector170:
        .thumb_func
Vector174:
        .thumb_func
Vector178:
        .thumb_func
Vector17C:


        .thumb_func
Vector180:
        .thumb_func
Vector184:
        .thumb_func
Vector188:
        .thumb_func
Vector18C:
        .thumb_func
Vector190:
        .thumb_func
Vector194:
        .thumb_func
Vector198:
        .thumb_func
Vector19C:


        .thumb_func
Vector1A0:
        .thumb_func
Vector1A4:
        .thumb_func
Vector1A8:
        .thumb_func
Vector1AC:
        .thumb_func
Vector1B0:
        .thumb_func
Vector1B4:
        .thumb_func
Vector1B8:
        .thumb_func
Vector1BC:


        .thumb_func
Vector1C0:
        .thumb_func
Vector1C4:
        .thumb_func
Vector1C8:
        .thumb_func
Vector1CC:
        .thumb_func
Vector1D0:
        .thumb_func
Vector1D4:
        .thumb_func
Vector1D8:
        .thumb_func
Vector1DC:


        .thumb_func
Vector1E0:
        .thumb_func
Vector1E4:
        .thumb_func
Vector1E8:
        .thumb_func
Vector1EC:
        .thumb_func
Vector1F0:
        .thumb_func
Vector1F4:
        .thumb_func
Vector1F8:
        .thumb_func
Vector1FC:


        .thumb_func
Vector200:
        .thumb_func
Vector204:
        .thumb_func
Vector208:
        .thumb_func
Vector20C:
        .thumb_func
Vector210:
        .thumb_func
Vector214:
        .thumb_func
Vector218:
        .thumb_func
Vector21C:


        .thumb_func
Vector220:
        .thumb_func
Vector224:
        .thumb_func
Vector228:
        .thumb_func
Vector22C:
        .thumb_func
Vector230:
        .thumb_func
Vector234:
        .thumb_func
Vector238:
        .thumb_func
Vector23C:


        .thumb_func
Vector240:
        .thumb_func
Vector244:
        .thumb_func
Vector248:
        .thumb_func
Vector24C:
        .thumb_func
Vector250:
        .thumb_func
Vector254:
        .thumb_func
Vector258:
        .thumb_func
Vector25C:


        .thumb_func
Vector260:
        .thumb_func
Vector264:
        .thumb_func
Vector268:
        .thumb_func
Vector26C:
        .thumb_func
Vector270:
        .thumb_func
Vector274:
        .thumb_func
Vector278:
        .thumb_func
Vector27C:


        .thumb_func
Vector280:
        .thumb_func
Vector284:
        .thumb_func
Vector288:
        .thumb_func
Vector28C:
        .thumb_func
Vector290:
        .thumb_func
Vector294:
        .thumb_func
Vector298:
        .thumb_func
Vector29C:


        .thumb_func
Vector2A0:
        .thumb_func
Vector2A4:
        .thumb_func
Vector2A8:
        .thumb_func
Vector2AC:
        .thumb_func
Vector2B0:
        .thumb_func
Vector2B4:
        .thumb_func
Vector2B8:
        .thumb_func
Vector2BC:


        .thumb_func
Vector2C0:
        .thumb_func
Vector2C4:
        .thumb_func
Vector2C8:
        .thumb_func
Vector2CC:
        .thumb_func
Vector2D0:
        .thumb_func
Vector2D4:
        .thumb_func
Vector2D8:
        .thumb_func
Vector2DC:


        .thumb_func
Vector2E0:
        .thumb_func
Vector2E4:
        .thumb_func
Vector2E8:
        .thumb_func
Vector2EC:
        .thumb_func
Vector2F0:
        .thumb_func
Vector2F4:
        .thumb_func
Vector2F8:
        .thumb_func
Vector2FC:


        .thumb_func
Vector300:
        .thumb_func
Vector304:
        .thumb_func
Vector308:
        .thumb_func
Vector30C:
        .thumb_func
Vector310:
        .thumb_func
Vector314:
        .thumb_func
Vector318:
        .thumb_func
Vector31C:


        .thumb_func
Vector320:
        .thumb_func
Vector324:
        .thumb_func
Vector328:
        .thumb_func
Vector32C:
        .thumb_func
Vector330:
        .thumb_func
Vector334:
        .thumb_func
Vector338:
        .thumb_func
Vector33C:


        .thumb_func
Vector340:
        .thumb_func
Vector344:
        .thumb_func
Vector348:
        .thumb_func
Vector34C:
        .thumb_func
Vector350:
        .thumb_func
Vector354:
        .thumb_func
Vector358:
        .thumb_func
Vector35C:


        .thumb_func
Vector360:
        .thumb_func
Vector364:
        .thumb_func
Vector368:
        .thumb_func
Vector36C:
        .thumb_func
Vector370:
        .thumb_func
Vector374:
        .thumb_func
Vector378:
        .thumb_func
Vector37C:


        .thumb_func
Vector380:
        .thumb_func
Vector384:
        .thumb_func
Vector388:
        .thumb_func
Vector38C:
        .thumb_func
Vector390:
        .thumb_func
Vector394:
        .thumb_func
Vector398:
        .thumb_func
Vector39C:


        .thumb_func
Vector3A0:
        .thumb_func
Vector3A4:
        .thumb_func
Vector3A8:
        .thumb_func
Vector3AC:
        .thumb_func
Vector3B0:
        .thumb_func
Vector3B4:
        .thumb_func
Vector3B8:
        .thumb_func
Vector3BC:


        .thumb_func
Vector3C0:
        .thumb_func
Vector3C4:
        .thumb_func
Vector3C8:
        .thumb_func
Vector3CC:
        .thumb_func
Vector3D0:
        .thumb_func
Vector3D4:
        .thumb_func
Vector3D8:
        .thumb_func
Vector3DC:


        .thumb_func
Vector3E0:
        .thumb_func
Vector3E4:
        .thumb_func
Vector3E8:
        .thumb_func
Vector3EC:
        .thumb_func
Vector3F0:
        .thumb_func
Vector3F4:
        .thumb_func
Vector3F8:
        .thumb_func
Vector3FC:

        bl _unhandled_exception

        .thumb_func
        .weak _unhandled_exception
_unhandled_exception:
.stay:
        b .stay
