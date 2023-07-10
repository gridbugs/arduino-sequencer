# Arduino Nano LED Chaser

Basic Arduino project that displays a rotating pattern on some LEDs.

## Useful Commands

### Flash an Arduino Nano
```
make && sudo avrdude -P /dev/ttyUSB0 -c arduino -p m328p -U flash:w:main.elf
```

### Connect USB serial console
```
sudo picocom -b9600 /dev/ttyUSB0
```

## Circuit Diagram
![Circuit Diagram](/circuit.jpg)
