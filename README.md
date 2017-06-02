

**ATmega_DS18x20_Tester**
=====================

**Firmware for Sensor Tester by FSA**

This repo contains the firmware for test equipment brought to us by FSA in spring 2017. The project for this hardware was announced and is documented at "Deutsches Raspberry Pi Forum" (http://www.forum-raspberrypi.de/Thread-projekt-ds18b20-tester-mit-lcd).

The firmware supports several ways to check 1 wires sensors e.g. DS18B20 (https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) by Dallas Semiconductors (maybe better known as "Maxim Integrated" https://www.maximintegrated.com/en.html).

The software has "grown up" in conjunction to the hardware. First board was poor equiped with a button, a 1602 resp. 2004 LCD and an ATMega8 as µController.
Current hardware has a rotary encoder (dig) instead of the button, an ATMega328P controller and accessible UART pins, thus there are more features possible.
**Note:**   because at this time the firmware is a "work in progress" project at this time it is **not** possible to compile for an ATMega8.
I'll try to work on the code to reduce size of resulting flash file to fit in an ATMega8.
But this may last some weeks or months because, on the other hand, I want to increase functionality of the firmware.

**Firmware features:**

 - supports either 1602 or 2004 LCD, selectable by soldered jumper
 - has a shortcut for "blind" adjusting LCD contrast
 - multilevel menu on LCD or over UART connection 
 - store settings in EEPROM

**Description:**
*Testmode:*
  When powering the testboard the idle mode is entered. Next two pictures show the appearance of the idle text on a 2004 LCD:
  
![alt tag](http://dreamshader.bplaced.net/Images/github/idle1.png) 

First text that is displayed in idle mode. One click starts a test run.

![alt tag](http://dreamshader.bplaced.net/Images/github/idle2.png) 

Second text. Click twice to enter menu mode.

To start a testrun place a DS18x20 sensor to the board and click the dig once. Zhe LCD will show some information of the current sensor:
![alt tag](http://dreamshader.bplaced.net/Images/github/test.png) 



![alt tag](http://dreamshader.bplaced.net/Images/github/main.png) 











> Written with [StackEdit](https://stackedit.io/).
