

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
When powering the testboard the idle mode is entered. Next two pictures show the appearance of the idle text on a 2004 LCD. First text that is displayed in idle mode. One click starts a test run.
  
![alt tag](http://dreamshader.bplaced.net/Images/github/idle1.png) 

Second text. Click twice to enter menu mode.

![alt tag](http://dreamshader.bplaced.net/Images/github/idle2.png) 


To start a testrun place a DS18x20 sensor to the board and click the dig once. The LCD will show some information of the current sensor e.g. it's ID, temperature, ...

![alt tag](http://dreamshader.bplaced.net/Images/github/test.png) 

Clicking twice in idle mode will take you in menu mode:

![alt tag](http://dreamshader.bplaced.net/Images/github/main.png) 

**Note:** from this point there are no more pictures, yet. The following information is subject of change while enhancing the software.

**LCD menu mode:**
In the LCD menu you may choose one of the following options:
 - Brightness
 - Contrast
 - Swap dig A/B
 - 1W bus
 - Power safe mode
 - Device scan
 - Save settings
 - Set to defaults

***Brightness:*** by turning the dig right or left the brightnes of the LCD is increased resp. decreased. This setting may be stored in the EEPROM by "Save setting".

***Contrast:*** by turning the dig right or left the contrast of the LCD is increased resp. decreased. This setting may be stored in the EEPROM by "Save setting".
*hint: by holding the button for about 5 seconds in idle mode this menu item is entered automatically to give you a chance to adjust contrast even if nothing is readable on the LCD.*

***Swap dig A/B:*** this is useful because there seems to be no standard which pins of the dig are A and B. So it may occur, that these pins are swapped. This setting swaps the pins in the software, too. This setting may be stored in the EEPROM by "Save setting".
hint: to take effect for this setting a powercycle of the module is required. Don't forget to save the setting first.

 ***1W bus:*** this choice enters a small submenu with options "power on" and "power off" the bus. You may notice that the LED indicating bus power will change its status.

***Power safe mode:*** this has no effect, yet. It's an idea for further enhancement.

***Device scan:*** searches the first Device on the 1 wire bus and display information about it.

***Save settings:*** store settings to the EEPROM to make them permanent.
Note: all changes, except the contrast settings if entered in immediate mode (by holding the button for about 5 seconds) are only used until powering off the module. It's recommended you save changes made to EEPROM. 

***Set to defaults:*** all settings are set to factory defaults. The values are used only until next powercycle. To make them permanent you have to select "Save setting" before powerdown the module. 

**UART menu mode:**
If you connect the UART pins, that are accessible on the module, to e.g. an USB<->PL2303 Adapter, you will get the same menu items in your terminal program. Connection parameters are 38400 8N1.
Because there are no size limits of any display, some selections give more information.
Note, that all input has to be done by a keyboard. Encoder changes have no effect to the UART Menu.
hint: the idea was to have access to all settings and options even no display is connected or text on the LCD is not readable for some reasons.

**Future features:**

 - setup sensor resolution via menu (serial & LCD)
 - reset 1W bus via menu (serial & LCD)
 - reset search via menu (serial & LCD)
 - output crc and crc-info in device scan/sensor test
 - "click" hint in brightness and contrast submenu
 - display message if no sensor in testrun  found
 - display message that dig pins are swapped
 - serial output degree sign in minicom

**Creating flash file:**

You need an arduino IDE with "Open hardware.ro" bords extension. This is fount at http://openhardware.ro/boards/package_openhardwarero_index.json
Arduino IDE should work from Version 1.6.9 and up.
An additional library for encoder handling, the Click-Encoder-Lib, is needed: https://github.com/0xPIT/encoder
Now you can create and flash the image.

Have fun,
-ds-


> Written with [StackEdit](https://stackedit.io/).
