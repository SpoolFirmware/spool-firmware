# SKR MINI E3 V2.0

## Peripheral Allocation

### Timers

- TIM5(GP): WallClock
- TIM6(B):  Stepper Pulse Timer (Time that x us pulse)
- TIM7(B):  Stepper Execution Callback
- TIM8(ADV): PWM
    - Channel 1: Fan 0
    - Channel 2: Fan 1
    - Channel 3: Heater 0
    - Channel 4: Bed

### UARTs
- USART2: Gcode
- UART4: Communication with TMC2209s
- UART5: Debug Prints
