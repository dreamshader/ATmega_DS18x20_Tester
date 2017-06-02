

**ATmega_DS18x20_Tester**
=====================

**Firmware for Sensor Tester by FSA**

This repo contains the firmware for test equipment brought to us by FSA in spring 2017. The project for this hardware was announced and is documented at "Deutsches Raspberry Pi Forum" (http://www.forum-raspberrypi.de/Thread-projekt-ds18b20-tester-mit-lcd).

The firmware supports several ways to check 1 wires sensors e.g. DS18B20 (https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) by Dallas Semiconductors (maybe better known as "Maxim Integrated" https://www.maximintegrated.com/en.html).

The software has "grown up" in conjunction to the hardware. First board was poor equiped with a button, a 1602 resp. 2004 LCD and an ATMega8 as µController.
Current hardware has a rotary encoder (dig) instead of the button, an ATMega328P controller and accessible UART pins, thus there are more features possible.
**Note**:   because at this time the firmware is a "work in progress" project at this time it is **not** possible to compile for an ATMega8.
I'll try to work on the code to reduce size of resulting flash file to fit in an ATMega8.
But this may last some weeks or months because, on the other hand, I want to increase functionality of the firmware.

**Firmware features:**

 - supports either 1602 or 2004 LCD, selectable by soldered jumper
 - has a shortcut for "blind" adjusting LCD contrast
 - multilevel menu on LCD or over UART connection 
 - store settings in EEPROM

**Testmode:**
When powering the testboard the idle mode is entered. Next two pictures show the appearance of the idle text on a 2004 LCD:
  
![alt tag](http://dreamshader.bplaced.net/Images/github/idle1.png) 

First text that is displayed in idle mode. One click starts a test run.

![alt tag](http://dreamshader.bplaced.net/Images/github/idle2.png) 

Second text. Click twice to enter menu mode.

To start a testrun place a DS18x20 sensor to the board and click the dig once. The LCD will show some information of the current sensor e.g. it's ID, temperature, ...

![alt tag](http://dreamshader.bplaced.net/Images/github/test.png) 

Clicking twice in idle mode will take you in menu mode:

![alt tag](http://dreamshader.bplaced.net/Images/github/main.png) 

**Note:** from this point there are no more pictures, yet. The following information is subject of change while enhancing the software.

**LCD menu mode:**
In the LCD menu you may choose on of the following options:
 - Brightness
 - Contrast
 - Swap dig A/B
 - 1W bus
 - Power safe mode
 - Device scan
 - Save settings
 - Set to defaults
 - Exit menu

***Brightness:*** by turning the dig right or left the brightnes of the LCD is increased resp. decreased. This setting may be stored in EEPROM.

***Contrast:*** by turning the dig right or left the contrast of the LCD is increased resp. decreased. This setting may be stored in EEPROM.
*hint: by holding the button for about 5 seconds in idle mode this menu item is entered automatically to give you a chance to adjust contrast even if nothing is readable on the LCD.*

***Swap dig A/B:*** this is useful because there seems to be no standard which pins of the dig are A and B. So it may occur, that these pins are swapped. This setting swaps the pins in the software, too. This setting may be stored in EEPROM.
hint: you have to restart the tester module to take effect on this setting. Don't forget to save the setting first.

 ***1W bus:*** this choice enters a small submenu with options "power on" and "power off" the bus.  


> Written with [StackEdit](https://stackedit.io/).
