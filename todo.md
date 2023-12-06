# ToDo:

- [ ] AP mode
- [x] control output (acceleration and dirrection)
- [ ] turn output values to PWM (and servo)
- [ ] HW prototyp, schema
- [x] ADC měření napětí akumulátoru

## HW:

ADC měření napětí akumulátoru:

- dělič 1k0, 4k7
- pin ADC1_0, GPIO36
- ADC2_x nelze použít když je WiFi (viz https://lastminuteengineers.com/esp32-basics-adc)
- filtr 100nF kondenzátor

PWM motory:

- MX1508 driver (2 x full bridge)
- GPIO16 -> IN4
- GPIO17 -> IN3
- GPIO18 -> IN2
- GPIO19 -> IN1

PWM serva:

- GPIO13, GPIO14

OC světla:

- GPIO26, GPIO27

Bzučák:

- GPIO32
