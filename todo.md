# ToDo:

- [x] AP mode
- [ ] Exclusive ssid using mac
- [x] Control output (acceleration and dirrection)
- [x] Turn output values to PWM (and servo)
- [x] HW prototyp
- [ ] Schematic and some other docu
- [x] ADC accu voltage measurement
- [ ] Send data allways (to ensure motor off)

## HW:

ADC accu voltage measurement:

- volt. div. 1k0, 4k7
- pin ADC1_0, GPIO36
- ADC2_x can't be used when WiFi is ON (viz https://lastminuteengineers.com/esp32-basics-adc)
- filter capacitor 100nF

PWM motor outputs:

- MX1508 driver (2 x full bridge)
- GPIO16 -> IN4
- GPIO17 -> IN3
- GPIO18 -> IN2
- GPIO19 -> IN1

PWM servos:

- GPIO13, GPIO14

OC lights:

- GPIO26, GPIO27

Beeper:

- GPIO32
