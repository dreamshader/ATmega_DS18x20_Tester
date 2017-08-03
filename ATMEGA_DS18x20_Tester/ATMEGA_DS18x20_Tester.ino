/*
   01.01.2017

   Erstellt mit Arduino IDE 1.69 auf ATmega8A, 8Mhz

   Programmablauf:

   Einschalten
   Text mit Aufforderung zum Tastendruck anzeigen
   Wird der Taster gedrückt, wird die Versorgungsspannung für den 
     DS18x20 aktiviert und es werden die "Vitaldaten" ausgelesen
     und auf dem LCD angezeigt

   Nach 5 Sekunden wird die Versorgungsspannung abgeschaltet und es 
     erscheint wieder der Text mit Aufforderung Tastendruck.

*/

//
// ----------------------------------------------------------------------
// extended version (c) 2017 Dirk Schanz aka dreamshader
// 
// needs the click-encoder lib:
//   https://github.com/0xPIT/encoder
// for rotary encoder handling
// 
//
// brief description:
//
// 
//
//   future enhancements:
//     setup sensor resolution via menu (serial & LCD)
//     reset 1W bus via menu (serial & LCD)
//     reset search via menu (serial & LCD)
//     output crc and crc-info in device scan/sensor test
//     "click" hint in brightness and contrast submenu
//     dieplay message if no sensor in testrun  found
//     display message that dig pins are swapped
//     serial output degree sign in minicom
//
//
// history:
//  in may 2017:
//  -- collect all strings, make unique as much of them as possible
//  -- place strings into flash instead of SRAM
//  -- do output using central function
//  -- added: some more helper
//  -- added: support for dig encoder
//  -- added: menu support using UART
//  -- added: menu support on lcd
//
//  -- fixed: immediate menu called on button press, too
//  -- fixed: save contrast setting in immediate menu
//  -- added: support for 20x4 lcd
//  -- fixed: center toggling text on 20x4 LCD
//  -- fixed: brightness - setting has currently no effect
//  -- fixed: SRAM - store remaining strings in flash
//  -- fixed: bug in resolution
//  -- fixed: serial output of sensor id in same manner as to LCD
//  -- added: device scan with output to LCD
//  -- fixed: DIG change A and B ... additional menu item
//  -- added: output conversion time in device scan/sensor test
//  -- fixed: mostly must click twice
//  -- fixed: lighter/darker doesn't work properly
//  -- fixed: 2004 LCD - correct output handling to 2004 LCD 
//  -- fixed: higher value causes lower brightness
//  -- fixed: higher value causes lower contrast
//  -- fixed: device scan fails every first time, second try is ok
//  -- checked: generate firmware for ATmega8/ATmega88
//  -- added: device scan on LCD 
//
// ----------------------------------------------------------------------
//

#ifdef __AVR_ATmega328P__
  #define USE_SERIAL
  #define USE_EEPROM
  #define USE_MENU
  #define USE_DIG_ENCODER
  #define UART_REMOTE_CONTROL
#else
  #ifdef __AVR_ATmega168__
    // first we'll see whether tis mc makes sense ...
    // give error at this time
    #error ATmega168 NOT YET SUPPORTED
  #else
    #ifdef __AVR_ATmega8__
      // quite small item
//      #undef USE_SERIAL
      #define USE_SERIAL
      #undef USE_EEPROM
      #undef USE_MENU
      #undef USE_DIG_ENCODER
      #undef UART_REMOTE_CONTROL
      #undef F(a)
      #define F(a) a
    #else
      #ifdef __AVR_ATmega88P__
        // quite small item
//        #undef USE_SERIAL
        #define USE_SERIAL
        #undef USE_EEPROM
        #undef USE_MENU
        #undef USE_DIG_ENCODER
        #undef UART_REMOTE_CONTROL
        #undef F(a)
        #define F(a) a        
      #else      
        #error UNSUPPORTED TARGET!
      #endif // __AVR_ATmega88P__
    #endif // __AVR_ATmega8__
  #endif // __AVR_ATmega168__
#endif // __AVR_ATmega328P__


// 
// -------------------------- INCLUDE SECTION ---------------------------
// 

#include <OneWire.h>
#include <LiquidCrystal.h>

#ifdef USE_EEPROM
#include <EEPROM.h>
#endif // USE_EEPROM

#ifdef USE_DIG_ENCODER
#include <ClickEncoder.h>
#endif // USE_DIG_ENCODER

#ifdef UART_REMOTE_CONTROL
#include "uart_api.h"
#endif // UART_REMOTE_CONTROL

// 
// ------------------------ VERSION INFORMATION -------------------------
//
static byte firmwareMajorRelease = 0;
static byte firmwareMinorRelease = 4;
static byte firmwarePatchLevel   = 0;

static byte protocolMajorRelease = 0;
static byte protocolMinorRelease = 1;

static byte hardwareMajorRelease = 1;
static byte hardwareMinorRelease = 1;

//
// ------------------------- BASIC DEFINITIONS --------------------------
//
// define arduino/atmega pins for
// LCD data display and
#define PIN_LCD_RS                  2      // LCD pin RS
#define PIN_LCD_RW                  3      // LCD pin RW
#define PIN_LCD_EN                  4      // LCD pin EN
#define PIN_LCD_D4                  5      // LCD pin D4
#define PIN_LCD_D5                  6      // LCD pin D5
#define PIN_LCD_D6                  7      // LCD pin D6
#define PIN_LCD_D7                  8      // LCD pin D7
//
// LCD display control
#define PIN_BRIGHTNESS              9      // LCD Helligkeit
#define PIN_CONTRAST               10      // LCD Kontrast

//
// old version
// - push button only
#define PIN_PUSH_BUTTON            11      // Startknopf
//
// power pin sensor and last but not least
#define PIN_SENSOR_POWER           12      // Spannungsversorgung DS18x20
//
// 1W bus sensor is connected to
#define PIN_1WIRE_BUS              13      // Pin # 1Wire Bus DS18x20 
//
// define CHIP IDs of valid DS18x2x chips
// add more valid ids here ...
#define CHIP_ID_DS18S20          0x10      // device is a DS18S20
#define CHIP_ID_DS18B20          0x28      // device is a DS18B20
#define CHIP_ID_DS1822           0x22      // device is a DS1B22
//
// tell how many tries to read from 1W bus
#define MAX_READ_TRIES              5
//
// define ms to wait after power on/off bus
#define POWER_OFF_DELAY           200
#define POWER_ON_DELAY            200
#define BUS_SETTLE_DELAY          100
//
#ifdef USE_SERIAL
// serial settings
#define SERIAL_BAUD             38400      // serial console/debug
#endif // USE_SERIAL

#define PIN_LCD_TYPE               A0      // soldering jumper to select type of LCD
                                           // open = LOW = LCD type 2004
                                           // closed = HIGH = LCD type 1602

#define LCD_1602_NUM_COLUMNS       16      // columns and
#define LCD_1602_NUM_ROWS           2      // rows for 1602 type

#define LCD_2004_NUM_COLUMNS       20      // columns and
#define LCD_2004_NUM_ROWS           4      // rows for 2004 type

#define LCD_TYPE_1602            1602
#define LCD_TYPE_2004            2004

#define LCD_DEFAULT_BRIGHTNESS    150      // intial default for brightness and
#define LCD_DEFAULT_CONTRAST       90      // contrast

#ifdef USE_EEPROM
#define LCD_HIGHLIGHT_BRIGHTNESS    0      // brighter - active
#define LCD_DIMMED_BRIGHTNESS       0      // dimmed - idle mode
#else
#define LCD_HIGHLIGHT_BRIGHTNESS    0      // brighter - active
#define LCD_DIMMED_BRIGHTNESS       0      // dimmed - idle mode
#endif // USE_EEPROM

//
// some global stuff - makes life easier
//   some current settings - may be changed
//   using the menu
int currentBrightness;                     // current brightness
int currentContrast;                       // current value for contrast
int lcdType;                               // type of LCD ... to make life easier
bool currentPowerSafeMode;                 // power safe mode = switch off Vcc of sensor 
                                           // in idle mode
bool current1WBusPower;                    // indicates whether 1W bus is powered
bool swapDigPins;                          // swap A and B ... necessary for some DIGs

//
// and these two globals we need to be flexible 
//   cannot be changed via software
int lcdNumColumns;
int lcdNumRows;

#ifdef USE_DIG_ENCODER
#define NUM_TOGGLE_TEXT             4
#else
#define NUM_TOGGLE_TEXT             2
#endif // USE_DIG_ENCODER

//
// details for idle display
#define LCD_16X2_LINE_TOGGLE_TEXT   0
#define LCD_20X4_LINE_1_TOGGLE_TEXT 0
#define LCD_20X4_LINE_2_TOGGLE_TEXT 1

#define LCD_REFRESH_TOGGLE_LINE  1500
#define TOGGLE_STRING_1             "    To start    "
#define TOGGLE_STRING_2             "  press button  "
#define TOGGLE_STRING_3             "  Double-Click  "
#define TOGGLE_STRING_4             "    for Menu    "

#define LCD_16X2_LINE_SCROLL_TEXT   1
#define LCD_20X4_LINE_1_SCROLL_TEXT 2
#define LCD_20X4_LINE_2_SCROLL_TEXT 3

#define LCD_REFRESH_SCROLL_LINE   700
#define SCROLL_TEXT_PART_1          " DS18x20 Tester"
#define SCROLL_TEXT_PART_2          " (c)FSA 04/2017"
#define SCROLL_TEXT_LEN            30

#define TEXT_SCANNING                  " Scanning ... "
#define TEXT_TESTING                   " Test ... "
#define TEXT_DONE                      "done! "
#define TEXT_FAIL                      "FAIL! "

//
// details for info display
#define INFO_DISPLAY_TIME        5000 // 5 sec. display
#define LCD_20x4_LINE_SENDOR_ID     1
#define LCD_20x4_LINE_SENDOR_DATA_1 2
#define LCD_20x4_LINE_SENDOR_DATA_2 3
#define LCD_16x2_LINE_SENDOR_ID     0
#define LCD_16x2_LINE_SENDOR_DATA   1

#define LCD_16x2_LINE_SELECT_ITEM   1
#define LCD_16x2_LINE_DISPLAY_ITEM  0
#define LCD_20x4_LINE_SELECT_ITEM   2
#define LCD_20x4_LINE_DISPLAY_ITEM  1

// 
// create instances of 1W Bus 
OneWire  oneWireBus(PIN_1WIRE_BUS);     // DS18x20
//
// and LCD
LiquidCrystal lcd( PIN_LCD_RS, PIN_LCD_RW, PIN_LCD_EN, 
                   PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7 );
//
//
// ------------------------------ ROTARY ENCODER SECTION -------------------------------
//
// Rotary encoder stuff. We need an encoder with push button.
//
#ifdef USE_DIG_ENCODER
// new version of testboard
// - rtr encoder replaces push button
#define PIN_ENCODER_A              A1
#define PIN_ENCODER_B              A2
#define PIN_ENCODER_PUSH           A3   // encoder C - click switch
//
#define ENCODER_STEP                4   // values per step
//
#define TIME_HOLD_FOR_CONTRAST   3000   // 3 sec. hold to enter immediate contrast setup

ClickEncoder *encoder;      // encoder with click button

#endif // USE_DIG_ENCODER
//





// #ifdef USE_MENU
//
// --------------------------------- COMMON MENU STUFF ---------------------------------
//
//
// common menu stuff ... 
//

int menuStatus;
static int encoderMenuStatus;

#define DO_MAIN_MENU                   0
#define DO_MAIN_CHOICE                 1
#define DO_UART_CONTROL                2
#define DO_1WBUS_MENU                  3
#define DO_POWERSAFE_MENU              4
#define DO_BRIGHTNESS_MENU             5
#define DO_CONTRAST_MENU               6
#define DO_DIG_PINSWAP_MENU            7

#ifdef USE_MENU


#define MENU_MAIN_HEADER_TEXT          "  Main-Settings "     // header line
#define MENU_1WBUS_HEADER_TEXT         "1W-Bus-Settings "
#define MENU_POWERSAFE_HEADER_TEXT     "Power-Safe-Mode "
#define MENU_DIG_PINS_HEADER_TEXT      " Swap A/B pins  "
#define MENU_BRIGHTNESS_HEADER_TEXT    "Adj. Brightness "
#define MENU_CONTRAST_HEADER_TEXT      " Adj. Contrast  "

#define MENU_MAIN_INPUT_PROMPT         "your choice (A-H): "    // UART prompt
#define LCD_CHANGE_VALUE_PROMPT        "- <   click  > +"       // LCD prompt
#define LCD_SELECT_ITEM_PROMPT         "< click=select >"       // LCD prompt
#define MENU_019_INPUT_PROMPT          "your choice (0.1.9): "  // UART prompt

#define CURRENT_VALUE_TEXT             ": Current value is "
#define NEW_VALUE_PROMPT               "Enter new value (0-255,x): "

#define MENU_BRIGHTNESS_SELECTION      "Brightness"           // selection A
#define MENU_CONTRAST_SELECTION        "Contrast"             // selection B
#define MENU_SWAP_DIG_PINS_SELECTION   "Swap dig A/B"         // selection C
#define MENU_1WBUS_SELECTION           "1W bus"               // selection D
#define MENU_POWERSAFE_SELECTION       "Power safe mode"      // selection E
#define MENU_DEVICE_SCAN_SELECTION     "Device scan"          // selection F
#define MENU_SAVE_SETTINGS_SELECTION   "Save settings"        // selection G
#define MENU_DEFAULTS_SELECTION        "Set to defaults"      // selection H
#define MENU_EXIT_MENU_SELECTION       "Exit menu"            // selection I
#define MENU_NUMBER_ITEMS              9

#define MENU_1WBUS_POWER_ON_SELECTION  "1W-Bus ON"
#define MENU_1WBUS_POWER_OFF_SELECTION "1W-Bus OFF"
#define MENU_1WBUS_POWER_NUMBER_ITEMS  2

#define MENU_POWERSAFE_ON_SELECTION    "Power-Safe ON"
#define MENU_POWERSAFE_OFF_SELECTION   "Power-Safe OFF"
#define MENU_POWERSAFE_NUMBER_ITEMS    2

#define MENU_DIG_PINSWAP_SELECTION     "pin A/B = B/A"
#define MENU_DIG_NO_PINSWAP_SELECTION  "pin A/B = A/B"
#define MENU_DIG_PINSWAP_NUMBER_ITEMS  2

#define TEXT_STORE                     "storing ... "
#define TEXT_RESET                     "resetting ... "
#define TEXT_CANCEL_ACTION             "cancel"
#define TEXT_VALUE_CHANGED             " set to "
#define TEXT_BACK_ACTION               "back to main"

#define ITEM_MAIN_HEADER_TEXT          1
#define ITEM_HORIZONTAL_RULER          2
#define ITEM_MAIN_INPUT_PROMPT         3
#define ITEM_LCD_CHANGE_VALUE_PROMPT   4
#define ITEM_LCD_SELECT_ITEM_PROMPT    5
#define ITEM_NEW_BRIGHTNESS_VALUE      6
#define ITEM_NEW_CONTRAST_VALUE        7
#define ITEM_019_INPUT_PROMPT          8
#define ITEM_1WBUS_HEADER_TEXT         9
#define ITEM_POWERSAFE_HEADER_TEXT    10
#define ITEM_BRIGHTNESS_HEADER_TEXT   11
#define ITEM_CONTRAST_HEADER_TEXT     12

#define ITEM_BRIGHTNESS               13
#define ITEM_CONTRAST                 14
#define ITEM_SWAP_DIG_PINS            15
#define ITEM_1WBUS                    16
#define ITEM_POWERSAFE                17
#define ITEM_DEVICE_SCAN              18
#define ITEM_SAVE_SETTINGS            19
#define ITEM_DEFAULTS                 20
#define ITEM_EXIT_MENU                21

#define ITEM_1WBUS_POWER_ON           22
#define ITEM_1WBUS_POWER_OFF          23
#define ITEM_POWERSAFE_ON             24
#define ITEM_POWERSAFE_OFF            25
#define ITEM_BRIGHTNESS_CHANGED       26
#define ITEM_CONTRAST_CHANGED         27
#define ITEM_MENU_DIG_PINSWAP         28
#define ITEM_MENU_DIG_NO_PINSWAP      29

#define ITEM_DIG_PINS_HEADER_TEXT     30

#define ITEM_TEXT_STORE               40
#define ITEM_TEXT_DONE                41
#define ITEM_TEXT_RESET               42
#define ITEM_TEXT_CANCEL_ACTION       43
#define ITEM_TEXT_TESTING             44
#define ITEM_TEXT_BACK_ACTION         45
#define ITEM_TEXT_SCANNING            46
#define ITEM_TEXT_FAIL                47

#define ITEM_NEW_LINE                 90  // pseudo output - NL only
#define ITEM_BLANK                    91  // pseudo output - blank only
#define ITEM_DASH                     92  // pseudo output - dash only
#define ITEM_TAB                      93  // pseudo output - tab only
#define ITEM_CLEAR                    94  // pseudo output - clear e.g. lcd
#define ITEM_HOME                     95  // pseudo output - go to pos 0,0

#define LEN_ITEM_BRIGHTNESS           10
#define LEN_ITEM_CONTRAST              8
#define LEN_ITEM_SWAP_DIG_PINS        12
#define LEN_ITEM_1WBUS                 6
#define LEN_ITEM_POWERSAFE            15
#define LEN_ITEM_DEVICE_SCAN          11
#define LEN_ITEM_SAVE_SETTINGS        13
#define LEN_ITEM_DEFAULTS             15
#define LEN_ITEM_EXIT_MENU             9

#define LEN_ITEM_1WBUS_POWER_ON        9
#define LEN_ITEM_1WBUS_POWER_OFF      10
#define LEN_ITEM_POWERSAFE_ON         13
#define LEN_ITEM_POWERSAFE_OFF        14

#define LEN_ITEM_DIG_PINSWAP          13
#define LEN_ITEM_DIG_NO_PINSWAP       13

#define LEN_ITEM_TEXT_STORE           12
#define LEN_ITEM_TEXT_DONE             5
#define LEN_ITEM_TEXT_RESET           14
#define LEN_ITEM_TEXT_CANCEL_ACTION    6
#define LEN_ITEM_TEXT_TESTING         10
#define LEN_ITEM_TEXT_BACK_ACTION     12
#define LEN_ITEM_TEXT_SCANNING        10
#define LEN_ITEM_TEXT_FAIL             5



#define TEXT_ALIGN_NONE                0
#define TEXT_ALIGN_LEFT                1
#define TEXT_ALIGN_RIGHT               2
#define TEXT_ALIGN_CENTER              3

#define DIGIT_SELECTION                2   // 0 - 9
#define ALPHA_SELECTION                3   // A - Z

#define OUTPUT_DEVICE_UART             1
#define OUTPUT_DEVICE_LCD              2


#define MAX_MENU_WIDTH                24
const char menuHorDelimiter = '-';

//
// ------------------------------- END COMMON MENU STUFF -------------------------------
//
#endif // USE_MENU
//
//
// ---------------------------- END ROTARY ENCODER SECTION -----------------------------
//
#ifdef USE_EEPROM

//
// ---------------------------------- EEPROM SECTION -----------------------------------
// EEPROM part ... only these few lines without any overhead to reduce SRAM consumption
//

#define EE_POS_MAGIC                0
#define EE_POS_BRIGHTNESS           1
#define EE_POS_CONTRAST             2
#define EE_POS_POWERSAFE            3
#define EE_POS_SWAP_DIG_PINS        4

// change this to force reset to defaults
#define EE_MAGIC_BYTE            0x9e

// ---------------------------------------------------------
// void storeSettings ( void )
//
// write values for brightness, contrast and powersafe mode
//    to EEPROM
// ---------------------------------------------------------
void storeSettings( void )
{
  // write to EEPROM these three bytes:
  // currentBrightness
  // currentContrast
  // currentPowerSafeMode
  EEPROM.write( EE_POS_MAGIC, EE_MAGIC_BYTE );

  EEPROM.write( EE_POS_BRIGHTNESS, currentBrightness );
  EEPROM.write( EE_POS_CONTRAST, currentContrast );
  EEPROM.write( EE_POS_POWERSAFE, currentPowerSafeMode );    
  EEPROM.write( EE_POS_SWAP_DIG_PINS, swapDigPins );    
}

// ---------------------------------------------------------
// void restoreSettings ( void )
//
// read values for brightness, contrast and powersafe mode
//    from EEPROM
//    if values haven't previously stored (check MAGIC) set
//    values to defaults and store them
// ---------------------------------------------------------
void restoreSettings( void )
{
  byte magicByte;

  // read from EEPROM these three bytes:
  // currentBrightness
  // currentContrast
  // currentPowerSafeMode
  // swapDigPins

  magicByte = EEPROM.read( EE_POS_MAGIC );

  if( magicByte == EE_MAGIC_BYTE )
  {
    currentBrightness = EEPROM.read( EE_POS_BRIGHTNESS );
    currentContrast = EEPROM.read( EE_POS_CONTRAST );
    currentPowerSafeMode = EEPROM.read( EE_POS_POWERSAFE );
    swapDigPins = EEPROM.read( EE_POS_SWAP_DIG_PINS );
  }
  else
  {
    currentBrightness = LCD_DEFAULT_BRIGHTNESS;
    currentContrast = LCD_DEFAULT_CONTRAST;
    currentPowerSafeMode = true;
    swapDigPins = false;
    storeSettings();
  }
}

//
#endif // USE_EEPROM 
//
// -------------------------------- END EEPROM SECTION ---------------------------------
//

// byte makeVersion( byte major, byte minor )
// {
//   return( (major << 4) | minor  );
// }





//
// ----------------------------------- GENERAL SETUP -----------------------------------
//
//
// ---------------------------------------------------------
// void setup ( void )
//
// here we go ... first at all: setup MCU and peripherials
// ---------------------------------------------------------
void setup(void)
{

#ifdef USE_SERIAL
  Serial.begin(SERIAL_BAUD);
#endif // USE_MENU

#ifdef USE_EEPROM
  EEPROM.begin();
  restoreSettings();
#else
  currentBrightness = LCD_DEFAULT_BRIGHTNESS;
  currentContrast = LCD_DEFAULT_CONTRAST;
  currentPowerSafeMode = false;
#endif // USE_EEPROM

#ifdef USE_DIG_ENCODER
  pinMode(PIN_ENCODER_PUSH, INPUT);
  pinMode(PIN_ENCODER_A,    INPUT);
  pinMode(PIN_ENCODER_B,    INPUT);

  // create encoder instance
  if( swapDigPins )
  {
    encoder = new ClickEncoder(PIN_ENCODER_B, PIN_ENCODER_A, PIN_ENCODER_PUSH, ENCODER_STEP);
  }
  else
  {
    encoder = new ClickEncoder(PIN_ENCODER_A, PIN_ENCODER_B, PIN_ENCODER_PUSH, ENCODER_STEP);
  }

#else
  // old fashioned PCB has no DIG onboard
  pinMode(PIN_PUSH_BUTTON,  INPUT);
#endif // USE_DIG_ENCODER

  // LCD brightness and contrast pins are outputs as well as
  pinMode(PIN_BRIGHTNESS,   OUTPUT);
  pinMode(PIN_CONTRAST,     OUTPUT);

  // power pin for DS18 sensor
  pinMode(PIN_SENSOR_POWER, OUTPUT);

  // first power up possible 1W device
  powerOn1W();

  // this is to detect LCD type
  pinMode(PIN_LCD_TYPE,     INPUT);
  // check for LCD-type
  if( digitalRead(PIN_LCD_TYPE) == LOW )
  {
    // LCD columns and rows for 2004 type
    lcdNumColumns = LCD_2004_NUM_COLUMNS;
    lcdNumRows = LCD_2004_NUM_ROWS;
    lcdType = LCD_TYPE_2004;
  }
  else
  {
    // LCD columns and rows for 1602 type
    lcdNumColumns = LCD_1602_NUM_COLUMNS;
    lcdNumRows = LCD_1602_NUM_ROWS;
    lcdType = LCD_TYPE_1602;
  }

  // initialize LCD
  lcd.begin( lcdNumColumns, lcdNumRows );
  analogWrite(PIN_BRIGHTNESS, 255-currentBrightness);
  analogWrite(PIN_CONTRAST, 255-currentContrast);
  lcd.clear();

  // switch off 1W bus power
  powerOff1W();
}
//
// --------------------------------- END GENERAL SETUP ---------------------------------
//
//
// -------------------------------- SOME USEFUL HELPERS --------------------------------

// ---------------------------------------------------------
// void enablePowerSafe ( void )
//
// set power safe mode to true
// ---------------------------------------------------------
void enablePowerSafe( void )
{
  currentPowerSafeMode = true;
}

// ---------------------------------------------------------
// void disablePowerSafe ( void )
//
// set power safe mode to false
// ---------------------------------------------------------
void disablePowerSafe( void )
{
  currentPowerSafeMode = false;
}

// ---------------------------------------------------------
// void powerOff1W ( void )
//
// set sensor Vcc pin to LOW -> switch Vcc of sensor off
// ---------------------------------------------------------
void powerOff1W( void )
{
  digitalWrite(PIN_SENSOR_POWER, LOW);     // schaltet die Versorgungs-
                                           // spannung des DS18x20 aus
  delay(POWER_OFF_DELAY);
  current1WBusPower = false;
}

// ---------------------------------------------------------
// void powerOn1W ( void )
//
// set sensor Vc pin to HIGH -> switch Vcc of sensor on
// ---------------------------------------------------------
void powerOn1W()
{
  digitalWrite(PIN_SENSOR_POWER, HIGH);    // schaltet die Versorgungs-
                                           // spannung des DS18x20 ein
  delay(POWER_ON_DELAY);
  current1WBusPower = true;
}

// ---------------------------------------------------------
// dimLCD ( void )
//
// write lower value to brightness pin for more brightness
// ---------------------------------------------------------
void dimLCD( void )
{
  if( currentPowerSafeMode )
  {
    analogWrite(PIN_BRIGHTNESS, 255-currentBrightness + LCD_DIMMED_BRIGHTNESS);
  }
}

// ---------------------------------------------------------
// void highlightLCD ( void )
//
// write higher value to brightness pin to lower brightness
// ---------------------------------------------------------
void highlightLCD( void )
{
  if( currentPowerSafeMode )
  {
    analogWrite(PIN_BRIGHTNESS, 255-currentBrightness - LCD_HIGHLIGHT_BRIGHTNESS);
  }
}

// ---------------------------------------------------------
// bool isValidChipId( byte idByte )
//
// check idByte to be in range, return false if no match
//       true if idByte is valid
// ---------------------------------------------------------
bool isValidChipId( byte idByte )
{
  bool retVal;

  switch ( idByte )
  {
    case CHIP_ID_DS18S20:
    case CHIP_ID_DS18B20:
    case CHIP_ID_DS1822:
      retVal = true;
      break;
    default:
      retVal = false;
      break;
  }
  return( retVal );
}

// ---------------------------------------------------------
// bool collect1WInfo( byte W1Address[8], byte data[12], 
//                     float *celsius, byte *resolution, 
//                     long *conversionTime )
//
// read information from a speicific sensor that address is
//      given as an argument. Return true, if sensor data
//      have been read. In that case, celsius will contain 
//      tepmerature in degree celsius and resolution contains
//      the resolution the sensor is configure for.
// ---------------------------------------------------------
bool collect1WInfo( byte W1Address[], byte data[], float *celsius, 
                    byte *resolution, long *conversionTime  )
{

  bool validTemp = false;
  float calcTemp;
  int16_t raw;
  int runs;

  for( runs = 0; !validTemp && runs < MAX_READ_TRIES; runs++ )
  {
    if ( !oneWireBus.search(W1Address))
    {
      oneWireBus.reset_search();
    }
    else
    {
      if (OneWire::crc8(W1Address, 7) != W1Address[7])
      {
        oneWireBus.reset();
      }
      else
      {
        oneWireBus.reset();
 
        // the first ROM byte indicates which chip
        if( isValidChipId( W1Address[0] ) )
        {
          oneWireBus.select(W1Address);
          // oneWireBus.write(0x44, 1);     // start conversion, parasite power on
          oneWireBus.write(0x44, 0);        // start conversion, parasite power off
      
          delay(1000);     // maybe 750ms is enough, maybe not
  
          oneWireBus.reset();
          oneWireBus.select(W1Address);
          oneWireBus.write(0xBE);         // Read Scratchpad
  
          // we need 9 bytes
          for ( int i = 0; i < 9; i++)
          {
            data[i] = oneWireBus.read();
          }
  
          // Convert the data to actual temperature
          // because the result is a 16 bit signed integer, it should
          // be stored to an "int16_t" type, which is always 16 bits
          // even when compiled on a 32 bit processor.
  
          raw = (data[1] << 8) | data[0];
          if( W1Address[0] == CHIP_ID_DS18S20 )
          {
            raw = raw << 3; // 9 bit resolution default
            *resolution = 9;
            *conversionTime = 93750;
            if (data[7] == 0x10)
            {
              // "count remain" gives full 12 bit resolution
              raw = (raw & 0xFFF0) + 12 - data[6];
              *resolution = 12;
              *conversionTime = 750000;
            }
          }
          else
          {
            // at lower res, the low bits are undefined, so let's zero them
            byte cfg = (data[4] & 0x60);
            switch( cfg )
            {
              case 0x00:
                raw = raw & ~7;  // 9 bit resolution, 93.75 ms
                *resolution = 9;
                *conversionTime = 93750;
                break;
              case 0x20:
                raw = raw & ~3; // 10 bit res, 187.5 ms
                *resolution = 10;
                *conversionTime = 187500;
              case 0x40:
                raw = raw & ~1; // 11 bit res, 375 ms
                *resolution = 11;
                *conversionTime = 375000;
                break;
              default:
                // default is 12 bit resolution, 750 ms conversion time
                *resolution = 12;
                *conversionTime = 750000;
              break;
            }
          }
        
          if( (calcTemp = (float)raw / 16.0) != 127.0 && calcTemp != 85.0 )
          {
            validTemp = true;
            *celsius = calcTemp;
          }
        }
      }
    }
  }
  return( validTemp );
}

// ---------------------------------------------------------
// void reset2Defaults( void )
//
// reset brightness, contrast and powesafe to their defaults
// ---------------------------------------------------------
void reset2Defaults( void )
{
  currentBrightness = LCD_DEFAULT_BRIGHTNESS;
  currentContrast = LCD_DEFAULT_CONTRAST;
  currentPowerSafeMode = true;
#ifdef USE_EEPROM
  storeSettings();
#endif // USE_EEPROM
  analogWrite(PIN_CONTRAST,   255-currentContrast);
  analogWrite(PIN_BRIGHTNESS, 255-currentBrightness);
}

// ---------------------------------------------------------
// void saveSettings( void )
//
// just call funtion to save to EEPROM
// ---------------------------------------------------------
void saveSettings( void )
{
  // write to EEPROM these three bytes:
  // currentBrightness
  // currentContrast
  // currentPowerSafeMode
#ifdef USE_EEPROM
  storeSettings();
#endif // USE_EEPROM
}

//
// ------------------------------ END SOME USEFUL HELPERS ------------------------------
//
//
// ------------------------------------ LCD RELATED ------------------------------------
//

// ---------------------------------------------------------
// String ScrollTxt(String txt)
//
// scroll a text left
// this useful function was leaned here:
// https://forum.arduino.cc/index.php?topic=132886.0
// ---------------------------------------------------------
String ScrollTxt(String txt)
{
  return txt.substring(1,txt.length()) + txt.substring(0,1);
}

// ---------------------------------------------------------
// void idleDisplay( bool reset )
// 
// display idle information
// first line of LCD toggles two different textes
// second line is a scrolled information
// ---------------------------------------------------------
void idleDisplay( bool reset )
{
  static String newScrollText = String(F(SCROLL_TEXT_PART_1)) + String(F(SCROLL_TEXT_PART_2));
  static unsigned long lastToggleRefresh;
  static unsigned long lastScrollRefresh;
  static byte toggle;
  static bool staticTextDone = false;

  if( reset )
  {
    lcd.clear();
    lastToggleRefresh = 0;
    lastScrollRefresh = 0;
    toggle = 0;
    staticTextDone = false;
  }

  if( millis() - lastToggleRefresh >= LCD_REFRESH_TOGGLE_LINE )
  {
    if( lcdType == LCD_TYPE_1602 )
    {
      lcd.setCursor(0, LCD_16X2_LINE_TOGGLE_TEXT);
      switch( toggle )
      {
        case 0:
          lcd.print(F(TOGGLE_STRING_1));
          break;
        case 1:
          lcd.print(F(TOGGLE_STRING_2));
          break;
        case 2:
          lcd.print(F(TOGGLE_STRING_3));
          break;
        case 3:
          lcd.print(F(TOGGLE_STRING_4));
          break;
      }
    }
    else
    {
      switch( toggle )
      {
        case 0:
        case 2:
          lcd.setCursor(2, LCD_20X4_LINE_1_TOGGLE_TEXT);
          lcd.print(F(TOGGLE_STRING_1));
          lcd.setCursor(2, LCD_20X4_LINE_2_TOGGLE_TEXT);
          lcd.print(F(TOGGLE_STRING_2));
          break;
        case 1:
        case 3:
          lcd.setCursor(2, LCD_20X4_LINE_1_TOGGLE_TEXT);
          lcd.print(F(TOGGLE_STRING_3));
          lcd.setCursor(2, LCD_20X4_LINE_2_TOGGLE_TEXT);
          lcd.print(F(TOGGLE_STRING_4));
          break;
      }
    }

    if( toggle++ >= NUM_TOGGLE_TEXT )
    {
      toggle = 0;
    }

    lastToggleRefresh = millis();
  }

  // or LCD_TYPE_2004 

  if( lcdType == LCD_TYPE_1602 )
  {
    if( millis() - lastScrollRefresh >= LCD_REFRESH_SCROLL_LINE )
    {
      lcd.setCursor(0, LCD_16X2_LINE_SCROLL_TEXT);
      lcd.print( newScrollText );
      newScrollText = ScrollTxt(newScrollText);
      lastScrollRefresh = millis();
    }
  }
  else
  {
      if( !staticTextDone )
      {
        lcd.setCursor(2, LCD_20X4_LINE_1_SCROLL_TEXT);
        lcd.print( F(SCROLL_TEXT_PART_1) );
        lcd.setCursor(2, LCD_20X4_LINE_2_SCROLL_TEXT);
        lcd.print( F(SCROLL_TEXT_PART_2) );
        staticTextDone = true;
      }
  }

}

// ---------------------------------------------------------
// void infoDisplay( byte addr[], float temp, 
//                   byte resolution, long conversionTime )
//
//   display 1W sensor information 
//   address is displayed in same format as used on Raspberry
//   Pi in virtual filesystem
// ---------------------------------------------------------
void infoDisplay( byte addr[], float temp, byte resolution, 
                  long conversionTime )
{

  if( lcdType == LCD_TYPE_1602 )
  {
    lcd.setCursor(0, LCD_16x2_LINE_SENDOR_ID);
  }
  else
  {
    // lcdType == LCD_TYPE_2004
    lcd.print(F(TEXT_DONE));
    lcd.setCursor(2, LCD_20x4_LINE_SENDOR_ID);
  }
  

  // display address
  // e.g. 10-00080278c4d6  
  //      28-00000629aa92
  lcd.print( addr[0], HEX);
  lcd.print("-");
  for ( int i = 6; i > 0; i--)
  {
    if( addr[i] < 0x10 )
    {
      lcd.print("0");
    }
    lcd.print( addr[i], HEX);
  }

  if( lcdType == LCD_TYPE_1602 )
  {
    lcd.setCursor(0, LCD_16x2_LINE_SENDOR_DATA);
  }
  else
  {
    // lcdType == LCD_TYPE_2004
    lcd.setCursor(2, LCD_20x4_LINE_SENDOR_DATA_1);
  }

  // display sensor type
  switch (addr[0])
  {
    case CHIP_ID_DS18S20:
      lcd.print(F("DS18S20"));
      break;
    case CHIP_ID_DS18B20:
      lcd.print(F("DS18B20"));
      break;
    case CHIP_ID_DS1822:
      lcd.print(F("DS1822"));
      break;
    default:
      lcd.print(F(" NOT a DS18x2x "));
      break;
  }

  if( lcdType == LCD_TYPE_1602 )
  {
    lcd.setCursor(9, LCD_16x2_LINE_SENDOR_DATA);
  }
  else
  {
    // lcdType == LCD_TYPE_2004
    lcd.setCursor(11, LCD_20x4_LINE_SENDOR_DATA_1);
  }

  // display temperature
  lcd.print(temp);
  lcd.print((char)223);   // zeigt das ° Zeichen an.
  // lcd.print("F");
  lcd.print("C");

  if( lcdType == LCD_TYPE_2004 )
  {
    lcd.setCursor(0, LCD_20x4_LINE_SENDOR_DATA_2);
    lcd.print(F("Resolution: "));
    lcd.print(resolution);
    lcd.print(F(" bit."));
  }
}


// ---------------------------------------------------------
// void doTestRun( void )
//
// perform a test run if sensor is found
// ---------------------------------------------------------
void doTestRun( void )
{
  static byte W1Address[8];
  static byte data[12];
  float celsius;
  byte resolution;
  long conversionTime;

  memset( W1Address, '\0', sizeof(W1Address) );
  powerOn1W();

  if( collect1WInfo(W1Address, data, &celsius, &resolution, &conversionTime) )
  {
    if( isValidChipId( W1Address[0] ) )
    {    
      lcd.clear();
      if( lcdType == LCD_TYPE_2004 )
      {
        lcd.print(F(TEXT_TESTING));
      }

      infoDisplay(W1Address, celsius, resolution, conversionTime);
      delay(INFO_DISPLAY_TIME);
    }
  }

  powerOff1W();
  
}




// ---------------------------------------------------------
// void printAddr( byte addr[] )
//
//   display 1W sensor id 
// ---------------------------------------------------------
void printAddr( byte addr[] )
{

  if( lcdType == LCD_TYPE_1602 )
  {
    lcd.setCursor(0, LCD_16x2_LINE_SENDOR_ID);
  }
  else
  {
    // lcdType == LCD_TYPE_2004
    lcd.setCursor(2, LCD_20x4_LINE_SENDOR_ID);
  }
  
  // display address
  // e.g. 10-00080278c4d6  
  //      28-00000629aa92
  lcd.print( addr[0], HEX);
  lcd.print("-");
  for ( int i = 6; i > 0; i--)
  {
    if( addr[i] < 0x10 )
    {
      lcd.print("0");
    }
    lcd.print( addr[i], HEX);
  }
}





// ---------------------------------------------------------
// byte getSensorID( bool first, byte sensorID[] )
//
// get next sensor id on bus, perform a power on
// if first flag is set
// ---------------------------------------------------------
byte getSensorID( bool first, byte sensorID[] )
{

  byte retVal = 0;

  if( first )
  {
    powerOn1W();
  }

  if(oneWireBus.search(sensorID))
  {
    retVal = 1;
  }
  else
  {
    oneWireBus.reset_search();
    powerOff1W();
    retVal = 0;
  }

  return( retVal );

}

// ---------------------------------------------------------
// void doScan( void )
//
// perform a test run if sensor is found
// ---------------------------------------------------------
void doScan( void )
{
  static byte W1Address[8];
  static byte data[12];
  float celsius;
  byte resolution;
  long conversionTime;

  memset( W1Address, '\0', sizeof(W1Address) );

  if( lcdType == LCD_TYPE_2004 )
  {
    lcd.print(F(TEXT_SCANNING));
  }

  powerOn1W();

  if( collect1WInfo(W1Address, data, &celsius, &resolution, &conversionTime) )
  {
    if( lcdType == LCD_TYPE_2004 )
    {
      lcd.print(F(TEXT_DONE));
    }

    if( isValidChipId( W1Address[0] ) )
    {    

      printAddr(W1Address);
    }

  }
  else
  {
    if( lcdType == LCD_TYPE_2004 )
    {
      lcd.print(F(TEXT_FAIL));
    }
  }

  delay(INFO_DISPLAY_TIME);

  powerOff1W();

  lcd.clear();
  
}


//
// ---------------------------------- END LCD RELATED ----------------------------------
//

#ifdef USE_MENU
//
// --------------------------------- COMMON MENU STUFF ---------------------------------
//


// ---------------------------------------------------------
// void displayItem( byte device, byte which, char *pPrefix, 
//                   bool lineFeed, byte alignment, byte maxWidth )
//
//   byte device     device for output e.g. UART, LCD
//   byte which      which item to display
//   char *pPrefix   flexible prefix e.g. "A-". "B-", ...
//   bool lineFeed   perform a linefeed after output
//   byte alignment  item alignement e.g. LEFT. CENTER, ...
//   byte maxWidth   maximum output length
// ---------------------------------------------------------
void displayItem( byte device, byte which, char *pPrefix, 
                  bool lineFeed, byte alignment, byte maxWidth )
{
  const __FlashStringHelper* pItem;              // item to display goes here
  const __FlashStringHelper* pPrompt;            // needed for special case only
  bool noItem = false;
  int leadingSpaces, trailingSpaces;
  int displayLength;

  switch( which )
  {
    case ITEM_MAIN_HEADER_TEXT:
      pItem = F(MENU_MAIN_HEADER_TEXT);
      displayLength = 16;
      break;
    case ITEM_1WBUS_HEADER_TEXT:
      pItem = F(MENU_1WBUS_HEADER_TEXT);
      displayLength = 16;
      break;
    case ITEM_POWERSAFE_HEADER_TEXT:
      pItem = F(MENU_POWERSAFE_HEADER_TEXT);
      displayLength = 16;
      break;
    case ITEM_DIG_PINS_HEADER_TEXT:
      pItem = F(MENU_DIG_PINS_HEADER_TEXT);
      displayLength = 16;
      break;
    case ITEM_BRIGHTNESS_HEADER_TEXT:
      pItem = F(MENU_BRIGHTNESS_HEADER_TEXT);    
      displayLength = 16;
      break;
    case ITEM_CONTRAST_HEADER_TEXT:
      pItem = F(MENU_CONTRAST_HEADER_TEXT);    
      displayLength = 16;
      break;
    case ITEM_MAIN_INPUT_PROMPT:
      pItem = F(MENU_MAIN_INPUT_PROMPT);
      displayLength = 0;
      break;
    case ITEM_LCD_CHANGE_VALUE_PROMPT:
      pItem = F(LCD_CHANGE_VALUE_PROMPT);
      displayLength = 16;
      break;
    case ITEM_LCD_SELECT_ITEM_PROMPT:
      pItem = F(LCD_SELECT_ITEM_PROMPT);
      displayLength = 16;
      break;
    case ITEM_HORIZONTAL_RULER:            // special case
      displayLength = 0;
      break;
    case ITEM_NEW_BRIGHTNESS_VALUE:        // special case
    case ITEM_NEW_CONTRAST_VALUE:          // special case
      displayLength = 16;
      break;
    case ITEM_NEW_LINE:                    // pseudo text -> NL
    case ITEM_BLANK:                       // pseudo output - blank only
    case ITEM_DASH:                        // pseudo output - dash only
    case ITEM_TAB:                         // pseudo output - tab only
    case ITEM_CLEAR:                       // pseudo text clear dieplay e.g. lcd
    case ITEM_HOME:                        // pseudo text go to position 0.0
      displayLength = 0;
      break;
    case ITEM_019_INPUT_PROMPT:
      pItem = F(MENU_019_INPUT_PROMPT);
      displayLength = 0;
      break;
    //
    // selection items from here
    //
    case ITEM_BRIGHTNESS:
      pItem = F(MENU_BRIGHTNESS_SELECTION);
      displayLength = LEN_ITEM_BRIGHTNESS;
      break;
    case ITEM_CONTRAST:
      pItem = F(MENU_CONTRAST_SELECTION);
      displayLength = LEN_ITEM_CONTRAST;
      break;
    case ITEM_SWAP_DIG_PINS:
      pItem = F(MENU_SWAP_DIG_PINS_SELECTION);
      displayLength = LEN_ITEM_SWAP_DIG_PINS;
      break;
    case ITEM_1WBUS:
      pItem = F(MENU_1WBUS_SELECTION);
      displayLength = LEN_ITEM_1WBUS;
      break;
    case ITEM_POWERSAFE:
      pItem = F(MENU_POWERSAFE_SELECTION);
      displayLength = LEN_ITEM_POWERSAFE;
      break;
    case ITEM_DEVICE_SCAN:
      pItem = F(MENU_DEVICE_SCAN_SELECTION);
      displayLength = LEN_ITEM_DEVICE_SCAN;
      break;
    case ITEM_SAVE_SETTINGS:
      pItem = F(MENU_SAVE_SETTINGS_SELECTION);
      displayLength = LEN_ITEM_SAVE_SETTINGS;
      break;
    case ITEM_DEFAULTS:
      pItem = F(MENU_DEFAULTS_SELECTION);
      displayLength = LEN_ITEM_DEFAULTS;
      break;
    case ITEM_EXIT_MENU:
      pItem = F(MENU_EXIT_MENU_SELECTION);
      displayLength = LEN_ITEM_EXIT_MENU;
      break;
    case ITEM_1WBUS_POWER_ON:
      pItem = F(MENU_1WBUS_POWER_ON_SELECTION);
      displayLength = LEN_ITEM_1WBUS_POWER_ON;
      break;
    case ITEM_1WBUS_POWER_OFF:
      pItem = F(MENU_1WBUS_POWER_OFF_SELECTION);
      displayLength = LEN_ITEM_1WBUS_POWER_OFF;
      break;
    case ITEM_POWERSAFE_ON:
      pItem = F(MENU_POWERSAFE_ON_SELECTION);
      displayLength = LEN_ITEM_POWERSAFE_ON;
      break;
    case ITEM_POWERSAFE_OFF:
      pItem = F(MENU_POWERSAFE_OFF_SELECTION);
      displayLength = LEN_ITEM_POWERSAFE_OFF;
      break;
    case ITEM_MENU_DIG_PINSWAP:
      pItem = F(MENU_DIG_PINSWAP_SELECTION);
      displayLength = LEN_ITEM_DIG_PINSWAP;
      break;
    case ITEM_MENU_DIG_NO_PINSWAP:
      pItem = F(MENU_DIG_NO_PINSWAP_SELECTION);
      displayLength = LEN_ITEM_DIG_NO_PINSWAP;
      break;
//
    case ITEM_TEXT_STORE:
      pItem = F(TEXT_STORE);
      displayLength = LEN_ITEM_TEXT_STORE;
      break;
    case ITEM_TEXT_DONE:
      pItem = F(TEXT_DONE);
      displayLength = LEN_ITEM_TEXT_DONE;
      break;
    case ITEM_TEXT_RESET:
      pItem = F(TEXT_RESET);
      displayLength = LEN_ITEM_TEXT_RESET;
      break;
    case ITEM_TEXT_CANCEL_ACTION:
      pItem = F(TEXT_CANCEL_ACTION);
      displayLength = LEN_ITEM_TEXT_CANCEL_ACTION;
      break;
    case ITEM_TEXT_BACK_ACTION:
      pItem = F(TEXT_BACK_ACTION);
      displayLength = LEN_ITEM_TEXT_BACK_ACTION;
      break;
    case ITEM_BRIGHTNESS_CHANGED:
    case ITEM_CONTRAST_CHANGED:
      pItem = F(TEXT_VALUE_CHANGED);
      displayLength = 0;
      break;
    default:
      noItem = true;
      break;
  }

  if( noItem == false ) // have an item to display
  {
    leadingSpaces = trailingSpaces = 0;

    switch( which )
    {
      case ITEM_NEW_BRIGHTNESS_VALUE:
      case ITEM_NEW_CONTRAST_VALUE:
	 // special case - ignore aligenment
	displayLength = 0;
	// special case - we have to display/output an item text
	pItem = F(CURRENT_VALUE_TEXT);
	// and a prompt for a new value
	pPrompt = F(NEW_VALUE_PROMPT);
	break;
      case ITEM_HORIZONTAL_RULER:
	// speical case ... horizontal delimiter
	if( (displayLength = maxWidth) <= 0 )
	{
	  displayLength = MAX_MENU_WIDTH;
	}
	break;
      default:
	// calculate output length
	if( pPrefix != NULL )
	{
	  displayLength += strlen( pPrefix );
	}

	switch( alignment )
	{
	  case TEXT_ALIGN_NONE:
	  case TEXT_ALIGN_LEFT:
	    // nothing to do - leave number of leading/trailing blanks untouched
	    break;
	  case TEXT_ALIGN_RIGHT:
	    // calculate number of leading white spaces
	    if( displayLength > 0 )
	    {
	      if( (leadingSpaces = maxWidth - displayLength) < 0 )
	      {
		leadingSpaces = 0;
	      }
	    }
	    break;
	  case TEXT_ALIGN_CENTER:
	    // calculate number of leading/trailing white spaces
	    if( displayLength > 0 )
	    {
	      if( (leadingSpaces = ((maxWidth - displayLength) / 2)) < 0 )
	      {
		leadingSpaces = 0;
	      }
	      else
	      {
		if( (trailingSpaces = (maxWidth - leadingSpaces - displayLength)) < 0 )
		{
		  trailingSpaces = 0;
		}
	      }
	    }
	    break;
	  default:
	    break;
	}
	break;
    }


    // now we are ready for display/output text
    switch(device)
    {
      case  OUTPUT_DEVICE_UART:
	switch( which )
	{
	  case ITEM_NEW_LINE:                    // pseudo text -> NL
	    Serial.println();
	    if( lineFeed )
	    {
	      Serial.println();
	    }
	    break;
	  case ITEM_BLANK:                       // pseudo output - blank only
	    Serial.print(' ');
	    if( lineFeed )
	    {
	      Serial.println();
	    }
	    break;
	  case ITEM_DASH:                        // pseudo output - dash only
	    Serial.print('-');
	    if( lineFeed )
	    {
	      Serial.println();
	    }
	    break;
	  case ITEM_TAB:                         // pseudo output - tab only
	    Serial.print('\t');
	    if( lineFeed )
	    {
	      Serial.println();
	    }
	    break;
	  case ITEM_CLEAR:                       // pseudo text clear dieplay e.g. lcd
	  case ITEM_HOME:                        // pseudo text go to position 0.0
	    break;
	  case ITEM_BRIGHTNESS_CHANGED:
	    Serial.println();
	    Serial.print(F(MENU_BRIGHTNESS_SELECTION));
	    Serial.print( pItem );
	    Serial.print( currentBrightness );
	    Serial.println(".");
	  case ITEM_CONTRAST_CHANGED:
	    Serial.println();
	    Serial.print(F(MENU_CONTRAST_SELECTION));
	    Serial.print( pItem );
	    Serial.print( currentContrast );
	    Serial.println(".");
	    break;
	  case ITEM_NEW_BRIGHTNESS_VALUE:
	    Serial.println();
	    Serial.print(F(MENU_BRIGHTNESS_SELECTION));
	    Serial.print( pItem );
	    Serial.print( currentBrightness );
	    Serial.println(".");
	    Serial.print( pPrompt );
	    break;
	  case ITEM_NEW_CONTRAST_VALUE:
	    Serial.println();
	    Serial.print(F(MENU_CONTRAST_SELECTION));
	    Serial.print( pItem );
	    Serial.print( currentContrast );
	    Serial.println(".");
	    Serial.print( pPrompt );
	    break;
	  case ITEM_HORIZONTAL_RULER:
	    for( int i=0; i < displayLength; i++ )
	    {
	      Serial.print( menuHorDelimiter );
	    }
	    if( lineFeed )
	    {
	      Serial.println();
	    }
	    break;
	  case ITEM_TEXT_STORE:
	    Serial.print( pItem );
	    if( lineFeed )
	    {
	      Serial.println();
	    }
	    break;
	  case ITEM_TEXT_DONE:
	    Serial.print( pItem );
	    if( lineFeed )
	    {
	      Serial.println();
	    }
	    break;
	  case ITEM_TEXT_RESET:
	    Serial.print( pItem );
	    if( lineFeed )
	    {
	      Serial.println();
	    }
	    break;
	  default:
	    if( leadingSpaces )
	    {
	      for( int i=0; i < leadingSpaces; i++ )
	      {
		Serial.print(" ");
	      }
	    }

	    if( pPrefix != NULL )
	    {
	      Serial.print( pPrefix );
	    }

	    Serial.print( pItem );

	    if( trailingSpaces )
	    {
	      for( int i=0; i < trailingSpaces; i++ )
	      {
		Serial.print(" ");
	      }
	    }

	    if( lineFeed )
	    {
	      Serial.println();
	    }
	    break;
	}
	break;
      case  OUTPUT_DEVICE_LCD:
	switch( which )
	{
	  case ITEM_NEW_LINE:                    // pseudo text -> NL
	  case ITEM_BLANK:                       // pseudo output - blank only
	  case ITEM_DASH:                        // pseudo output - dash only
	  case ITEM_TAB:                         // pseudo output - tab only
	  case ITEM_CLEAR:                       // pseudo text clear dieplay e.g. lcd
	  case ITEM_HOME:                        // pseudo text go to position 0.0
	    break;
	  case ITEM_NEW_BRIGHTNESS_VALUE:
	  case ITEM_NEW_CONTRAST_VALUE:
	    break;
	  case ITEM_HORIZONTAL_RULER:
	    break;
	  default:
	    if( leadingSpaces )
	    {
	      for( int i=0; i < leadingSpaces; i++ )
	      {
		lcd.print(" ");
	      }
	    }

	    if( pPrefix != NULL )
	    {
	      lcd.print( pPrefix );
	    }

	    lcd.print( pItem );

	    if( trailingSpaces )
	    {
	      for( int i=0; i < trailingSpaces; i++ )
	      {
		lcd.print(" ");
	      }
	    }          
	    break;
	}
	break;
      default:
	break;
    }
  }
}

//
// ------------------------------- END COMMON MENU STUFF -------------------------------
//
#endif // USE_MENU

//
//
//

#ifdef USE_SERIAL
//
// --------------------------- SERIAL CONTROL AND MENU STUFF ---------------------------
//

// ---------------------------------------------------------
// void displayUartMenu( void )
//
// use UART to display the main menu
// items are preceded by a prefix e.g. "A - ", "B - ", ...
// that is used as user selection
// ---------------------------------------------------------
void displayUartMenu( void )
{
#ifdef __AVR_ATmega328P__
  displayItem( OUTPUT_DEVICE_UART, ITEM_NEW_LINE,
                   "", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_MAIN_HEADER_TEXT,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER, 
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_BRIGHTNESS,
                   "A - ", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_CONTRAST,
                   "B - ", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_SWAP_DIG_PINS,
                   "C - ", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_1WBUS,
                   "D - ", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_POWERSAFE,
                   "E - ", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_DEVICE_SCAN,
                   "F - ", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_SAVE_SETTINGS,
                   "G - ", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_DEFAULTS,
                   "H - ", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_MAIN_INPUT_PROMPT,
                   "", false, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
#endif // __AVR_ATmega328P__
}

// ---------------------------------------------------------
// void uartSubMenu1WBus( void )
//
// display a sub menu to select switching on or off the
// 1Wire bus 
// ---------------------------------------------------------
void uartSubMenu1WBus( void )
{
#ifdef __AVR_ATmega328P__
  displayItem( OUTPUT_DEVICE_UART, ITEM_NEW_LINE,
                   "", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_1WBUS_HEADER_TEXT,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_1WBUS_POWER_OFF,
                   "0 - ", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_1WBUS_POWER_ON,
                   "1 - ", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_TEXT_CANCEL_ACTION,
                   "9 - ", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_019_INPUT_PROMPT,
                   "", false, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
#endif // __AVR_ATmega328P__
}

// ---------------------------------------------------------
// void uartSubMenuSwapDigPins( void )
//
// display a sub menu to enable or disable power safe mode
// ---------------------------------------------------------
void uartSubMenuSwapDigPins( void )
{
#ifdef __AVR_ATmega328P__
  displayItem( OUTPUT_DEVICE_UART, ITEM_NEW_LINE,
                   "", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_DIG_PINS_HEADER_TEXT,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                  "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_MENU_DIG_NO_PINSWAP,
                   "0 - ", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_MENU_DIG_PINSWAP,
                   "1 - ", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_TEXT_CANCEL_ACTION,
                   "9 - ", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_019_INPUT_PROMPT,
                   "", false, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
#endif // __AVR_ATmega328P__
}

// ---------------------------------------------------------
// void uartSubMenuPowerSafe( void )
//
// display a sub menu to enable or disable power safe mode
// ---------------------------------------------------------
void uartSubMenuPowerSafe( void )
{
#ifdef __AVR_ATmega328P__
  displayItem( OUTPUT_DEVICE_UART, ITEM_NEW_LINE,
                   "", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_POWERSAFE_HEADER_TEXT,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_POWERSAFE_OFF,
                   "0 - ", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_POWERSAFE_ON,
                   "1 - ", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_TEXT_CANCEL_ACTION,
                   "9 - ", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_019_INPUT_PROMPT,
                   "", false, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
#endif // __AVR_ATmega328P__
}

// ---------------------------------------------------------
// void uartSetCurrentContrast( int newValue )
//
// set selected contrast and send some informational
// text to serial connection
// ---------------------------------------------------------
void uartSetCurrentContrast( int newValue )
{
#ifdef __AVR_ATmega328P__
  if( newValue >= 0 && newValue <= 255 )
  {
    currentContrast = newValue;

    displayItem( OUTPUT_DEVICE_UART, ITEM_CONTRAST_CHANGED,
                     "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
    displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                     "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
                         
    analogWrite(PIN_CONTRAST,   255-currentContrast);
  }
#endif // __AVR_ATmega328P__
}

// ---------------------------------------------------------
// void uartSetCurrentBrightness( int newValue )
//
// set selected brightness and send some informational
// text to serial connection
// ---------------------------------------------------------
void uartSetCurrentBrightness( int newValue )
{
#ifdef __AVR_ATmega328P__
  if( newValue >= 0 && newValue <= 255 )
  {
    currentBrightness = newValue;

    displayItem( OUTPUT_DEVICE_UART, ITEM_BRIGHTNESS_CHANGED,
                     "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
    displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                     "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
                   
    analogWrite(PIN_BRIGHTNESS, 255-currentBrightness);
  }
#endif // __AVR_ATmega328P__
}

// ---------------------------------------------------------
// void uartSubMenuBrightness( void )
//
// display a sub menu to change lcd brightness to serial
// connection
// ---------------------------------------------------------
void uartSubMenuBrightness( void )
{
#ifdef __AVR_ATmega328P__
  displayItem( OUTPUT_DEVICE_UART, ITEM_NEW_LINE,
                   "", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_BRIGHTNESS_HEADER_TEXT,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_NEW_BRIGHTNESS_VALUE,
                   "", false, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
#endif // __AVR_ATmega328P__
}

// ---------------------------------------------------------
// void uartSubMenuContrast( void )
//
// display a sub menu to change lcd contrast to serial
// connection
// ---------------------------------------------------------
void uartSubMenuContrast( void )
{
#ifdef __AVR_ATmega328P__
  displayItem( OUTPUT_DEVICE_UART, ITEM_NEW_LINE,
                   "", true, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_CONTRAST_HEADER_TEXT,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  displayItem( OUTPUT_DEVICE_UART, ITEM_HORIZONTAL_RULER,
                   "", true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );

  displayItem( OUTPUT_DEVICE_UART, ITEM_NEW_CONTRAST_VALUE,
                   "", false, TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
#endif // __AVR_ATmega328P__
}


void uartSaveSettings( void )
{

#ifdef __AVR_ATmega328P__
  displayItem( OUTPUT_DEVICE_UART, ITEM_TEXT_STORE, "", 
               false,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  saveSettings();
  displayItem( OUTPUT_DEVICE_UART, ITEM_TEXT_DONE, "", 
               true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  menuStatus = DO_MAIN_MENU;
#endif // __AVR_ATmega328P__

}


void uartReset2Defaults( void )
{

#ifdef __AVR_ATmega328P__
  displayItem( OUTPUT_DEVICE_UART, ITEM_TEXT_RESET, "", 
               false,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  reset2Defaults();
  displayItem( OUTPUT_DEVICE_UART, ITEM_TEXT_DONE, "", 
               true,  TEXT_ALIGN_NONE, MAX_MENU_WIDTH );
  menuStatus = DO_MAIN_MENU;
#endif // __AVR_ATmega328P__

}






// ---------------------------------------------------------
// void uartFlush( void )
//
// read off all remaining character on serial line
// ---------------------------------------------------------
void uartFlush( void )
{
  // flush all bytes in uart buffer
  while( Serial.available() )
  {
    Serial.read();
  }
}

// ---------------------------------------------------------
// void uartScan1W( void )
//
// scan 1Wire bus for devices
// if one found display information to serial connection
// ---------------------------------------------------------
void uartScan1W( void )
{
  static byte W1Address[8];
  static byte data[12];
  float celsius;
  byte resolution;
  long conversionTime;

  memset( W1Address, '\0', sizeof(W1Address) );

  powerOn1W();
 
  Serial.println( "Scan 1W bus ..." );



  while ( oneWireBus.search(W1Address))
  {


    Serial.println();
    Serial.print("Device ");

    // display address
    // e.g. 10-00080278c4d6  
    //      28-00000629aa92
    Serial.print( W1Address[0], HEX);
    Serial.print("-");
    for ( int i = 6; i > 0; i--)
    {
      if( W1Address[i] < 0x10 )
      {
        Serial.print("0");
      }
      Serial.print( W1Address[i], HEX);
    }

  }



 
  collect1WInfo(W1Address, data, &celsius, &resolution, &conversionTime);

  if( W1Address[0] != '\0' )
  {
    Serial.println();
    Serial.print("Device ");

    // display address
    // e.g. 10-00080278c4d6  
    //      28-00000629aa92
    Serial.print( W1Address[0], HEX);
    Serial.print("-");
    for ( int i = 6; i > 0; i--)
    {
      if( W1Address[i] < 0x10 )
      {
        Serial.print("0");
      }
      Serial.print( W1Address[i], HEX);
    }

    Serial.print(" is ");

    switch (W1Address[0])
    {
      case CHIP_ID_DS18S20:
        Serial.print("a DS18S20");
        break;
      case CHIP_ID_DS18B20:
        Serial.print("a DS18B20");
        break;
      case CHIP_ID_DS1822:
        Serial.print("a DS1822");
        break;
      default:
        Serial.print("NOT a DS18x2x");
        break;
    }

    Serial.println();

    if( isValidChipId( W1Address[0] ) )
    {
      Serial.print("Resolution is ");
      Serial.print(resolution);
      Serial.print(" bit, conversion time ");
      Serial.print(conversionTime);

      Serial.println(" usec.");
      Serial.print("Temp is ");
      Serial.print(celsius);
      Serial.print("°C (");
      Serial.print(celsius * 1.8 + 32.0);
      Serial.println("°F).");

      Serial.print("data dump: ");
      for( int i = 0; i < 12; i++ )
      {
        Serial.print(data[i]);
        Serial.print(" ");
      }
    }
    Serial.println();      
  }
  else
  {
    Serial.println("NO 1W device found ...");
  }

  powerOff1W();
  menuStatus = DO_MAIN_MENU;
  uartFlush();  
}

// ---------------------------------------------------------
// void uartPowerOff1W( void )
//
// switch off Vcc of 1Wire bus
// send some information to serial connection
// ---------------------------------------------------------
void uartPowerOff1W( void )
{

  Serial.println( F("Power off 1W bus") );
  if( !currentPowerSafeMode )
  {
    Serial.println( F("Warning! Overriding power safe mode!") );
  }
  
  Serial.print( F("1W bus is ") );

  if( current1WBusPower )
  {
    powerOff1W();
    Serial.print( F("now "));
  }
  else
  {
    Serial.print( F("already ") );
  }

  Serial.println( F("off.") );

  menuStatus = DO_MAIN_MENU;
  uartFlush();  
}

// ---------------------------------------------------------
// void uartSwapDigPins( void )
//
// enable swap A/B pins of dig on next restart
// send some information to serial connection
// ---------------------------------------------------------
void uartSwapDigPins( void )
{

  Serial.println( F("Enabled swap A/B pins of dig,") );
  Serial.print( F("Pin swap is "));

  if( !swapDigPins )
  {
    swapDigPins = true;
    Serial.print( F("now "));
  }
  else
  {
    Serial.print( F("already ") );
  }

  Serial.println( F("enabled.") );

  menuStatus = DO_MAIN_MENU;
  uartFlush();  
}

// ---------------------------------------------------------
// void uartNoSwapDigPins( void )
//
// disable swap A/B pins of dig on next restart
// send some information to serial connection
// ---------------------------------------------------------
void uartNoSwapDigPins( void )
{

  Serial.println( F("Disable swap A/B pins of dig,") );
  Serial.print( F("Pin swap is "));

  if( !swapDigPins )
  {
    Serial.print( F("already "));
  }
  else
  {
    swapDigPins = false;
    Serial.print( F("now ") );
  }

  Serial.println( F("disabled.") );

  menuStatus = DO_MAIN_MENU;
  uartFlush();  
}

// ---------------------------------------------------------
// void uartPowerOn1W( void )
//
// switch on Vcc of 1Wire bus
// send some information to serial connection
// ---------------------------------------------------------
void uartPowerOn1W( void )
{
  
  Serial.println( F("Power on 1W bus") );
  Serial.print( F("1W bus is "));

  if( !current1WBusPower )
  {
    powerOn1W();
    Serial.print( F("now "));
  }
  else
  {
    Serial.print( F("already ") );
  }

  Serial.println( F("on.") );

  menuStatus = DO_MAIN_MENU;
  uartFlush();  
}

// ---------------------------------------------------------
// void uartDisablePowerSafe( void )
//
// disable power safe mode and send informational
// text to serial connection
// ---------------------------------------------------------
void uartDisablePowerSafe( void )
{
  Serial.println( F("disable power safe") );
  Serial.print(F("Power safe is "));

  if( currentPowerSafeMode )
  {
    disablePowerSafe();
    Serial.print(F("now "));
  }
  else
  {
    Serial.print(F("already "));
  }

  Serial.println(F("disabled."));


  menuStatus = DO_MAIN_MENU;
  uartFlush();  
}

// ---------------------------------------------------------
// void uartEnablePowerSafe( void )
//
// enable power safe mode and send informational
// text to serial connection
// ---------------------------------------------------------
void uartEnablePowerSafe( void )
{
  Serial.println( F("enable power safe") );
  Serial.print(F("Power safe is "));

  if( !currentPowerSafeMode )
  {
    enablePowerSafe();
    Serial.print(F("now "));
  }
  else
  {
    Serial.print(F("already "));
  }

  Serial.println(F("enabled."));

  menuStatus = DO_MAIN_MENU;
  uartFlush();  
}
//


#ifdef UART_REMOTE_CONTROL
//
// ---------------------------- UART REMOTE CONTROL HANDLING ---------------------------
//

static byte protocol_version   = 0x04;


void uartControlRun( bool reset )
{
  static byte controlBuf[UART_CTLBUF_SIZE+1];
  static int cmdBufferIndex = 0;
  static int telegramIndex = 0;
  static unsigned long lastReadSuccess;
  static bool cmdCompleted = false;
  static struct _uart_telegram_ command;
  struct _uart_telegram_ response;
  
  if( Serial.available() )
  {
 
    if( cmdBufferIndex < UART_CTLBUF_SIZE )
    {
      controlBuf[cmdBufferIndex] = Serial.read();
      lastReadSuccess = millis();

      switch( cmdBufferIndex + 1 )
      {
        case 1:
          command._opcode = controlBuf[cmdBufferIndex];
          break;
        case 2:
          command._crc8 = controlBuf[cmdBufferIndex];
          break;
        case 3:
          command._sequence = controlBuf[cmdBufferIndex];
          break;
        case 4:
          command._status = controlBuf[cmdBufferIndex];
          break;
        case 5:
          command._arg_cnt = controlBuf[cmdBufferIndex];
          break;
        default:
          if( (telegramIndex < command._arg_cnt) &&
              (telegramIndex < REMOTE_COMMAND_MAX_ARGS) )
          {
            command._args[telegramIndex++] = controlBuf[cmdBufferIndex];
          }
          break;
      }
  
      cmdBufferIndex++;
    }
    else
    {
      byte discardData = Serial.read();
      lastReadSuccess = millis();
    }
  }

  if( cmdBufferIndex >= REMOTE_COMMAND_HDR_LENGTH )
  {
    // we have at least a complete header

    if( telegramIndex == command._arg_cnt )
    {
      cmdCompleted = true;
      _uartErrorCode = UART_CTL_E_OK;
      cmdBufferIndex = 0;
      telegramIndex = 0;
      lastReadSuccess = 0;
      uartFlush();  
      if( uartControlRunCommand( &command, &response ) )
      {
        // successfully done
      }
      else
      {
        // something has gone wrong
      }

      if( uartSendResponse( &command, &response ) )
      {
        // successfully done
      }
      else
      {
        // something has gone wrong
      }

      clearTelegram( &command );
      clearTelegram( &response );
      memset( controlBuf, '\0', sizeof(controlBuf) );

    }
    else
    {
      if( telegramIndex > command._arg_cnt )
      {
        cmdCompleted = true;
        _uartErrorCode = UART_CTL_E_OVERFLOW;
        cmdBufferIndex = 0;
        telegramIndex = 0;
        lastReadSuccess = 0;
        uartFlush();  
        if( uartControlRunCommand( &command, &response ) )
        {
          // successfully done
        }
        else
        {
          // something has gone wrong
        }

        if( uartSendResponse( &command, &response ) )
        {
          // successfully done
        }
        else
        {
          // something has gone wrong
        }

        clearTelegram( &command );
        clearTelegram( &response );
        memset( controlBuf, '\0', sizeof(controlBuf) );

      }
    }
  }
  else
  {
    // incomplete command or single byte action
    if( cmdBufferIndex > 0 )
    {
      // check for single byte command
      if( command._opcode == 0x05 ) // hangup 
      {
        cmdBufferIndex = 0;
        telegramIndex = 0;
        lastReadSuccess = 0;
        cmdCompleted = false;
        _uartErrorCode = UART_CTL_E_OK;
        menuStatus = DO_MAIN_MENU;
        uartFlush();  
        clearTelegram( &command );
        clearTelegram( &response );
        memset( controlBuf, '\0', sizeof(controlBuf) );
      }
    }

  }

  if( lastReadSuccess != 0 )
  {
    if( millis() - lastReadSuccess >= UART_CTL_TIMEOUT*10 )
    {
      cmdBufferIndex = 0;
      telegramIndex = 0;
      lastReadSuccess = 0;
      cmdCompleted = false;
      _uartErrorCode = UART_CTL_E_TIMEOUT;
      menuStatus = DO_MAIN_MENU;
      uartFlush();  
      clearTelegram( &command );
      clearTelegram( &response );
      memset( controlBuf, '\0', sizeof(controlBuf) );
    }
  }
}

// ---------------------------------------------------------
// byte getNextSensorID( byte sensorID[]  )
//
// locate next device on bus
// ---------------------------------------------------------
byte getNextSensorID( byte sensorID[]  )
{
  return( getSensorID( false, sensorID ) );
}

// ---------------------------------------------------------
// byte getFirstSensorID( byte sensorID[] )
//
// locate first device on 1W bus
// ---------------------------------------------------------
byte getFirstSensorID( byte sensorID[] )
{

  return( getSensorID( true, sensorID ) );
}

byte getFirstSensorTemp( byte sensorID[] )
{
return( 0 );
}

byte getNextSensorTemp( byte sensorID[] )
{
return( 0 );
}

byte getSensorTemp( byte sensorID[] )
{
return( 0 );
}





//
// ------------------------- END UART REMOTE CONTROL HANDLING --------------------------
//
#endif // UART_REMOTE_CONTROL

//
// -------------------------------- UART MENU HANDLING ---------------------------------
//

// ---------------------------------------------------------
// void uartMenu( void )
//
// display and handle input of serial menu
// ---------------------------------------------------------
void uartMenu( void )
{
  static int numericInput;
  static int inputLength;
  static bool inputComplete;

  switch( menuStatus )
  {
    case DO_MAIN_MENU:
      // flush all input
      uartFlush();
      displayUartMenu();
      menuStatus = DO_MAIN_CHOICE;
      break;
    case DO_MAIN_CHOICE:
      if( Serial.available() )
      {
        byte choice = Serial.read();
        uartFlush();

        switch( choice )
        {
          // #define MENU_BRIGHTNESS_SELECTION     "Brightness"           // selection A
          case 'a':
          case 'A':
              uartSubMenuBrightness();
              numericInput = 0;
              inputLength = 0;
              inputComplete = false;
              menuStatus = DO_BRIGHTNESS_MENU;
              uartFlush();
            break;
          // #define MENU_CONTRAST_SELECTION       "Contrast"             // selection B
          case 'b':
          case 'B':
              uartSubMenuContrast();
              numericInput = 0;
              inputLength = 0;
              inputComplete = false;
              menuStatus = DO_CONTRAST_MENU;
              uartFlush();
            break;
          // #define MENU_SWAP_DIG_PINS_SELECTION   "Swap dig A/B"         // selection C
          case 'c':
          case 'C':
            uartSubMenuSwapDigPins();
            menuStatus = DO_DIG_PINSWAP_MENU;
            uartFlush();
            break;
          // #define MENU_1WBUS_SELECTION          "1W bus"               // selection D
          case 'd':
          case 'D':
              uartSubMenu1WBus();
              menuStatus = DO_1WBUS_MENU;
              uartFlush();
            break;
          // #define MENU_POWERSAFE_SELECTION      "Power safe mode"      // selection E
          case 'e':
          case 'E':
              uartSubMenuPowerSafe();
              menuStatus = DO_POWERSAFE_MENU;
              uartFlush();
            break;
          // #define MENU_DEVICE_SCAN_SELECTION    "Device scan"          // selection F
          case 'f':
          case 'F':
            uartScan1W();
            break;
          // #define MENU_SAVE_SETTINGS_SELECTION  "Save settings"        // selection G
          case 'g':
          case 'G':
            uartSaveSettings();
            menuStatus = DO_1WBUS_MENU;
            break;
          // #define MENU_DEFAULTS_SELECTION       "Set to defaults"      // selection H
          case 'h':
          case 'H':
            uartReset2Defaults();
            menuStatus = DO_1WBUS_MENU;
            break;
#ifdef UART_REMOTE_CONTROL
          case '%':
            menuStatus = DO_UART_CONTROL;
            uartConnectionResponse();
            break;
#endif // UART_REMOTE_CONTROL
          default:
            displayUartMenu();
            break;
        }
      }
      break;
    case DO_POWERSAFE_MENU:
      if( Serial.available() )
      {
        byte choice = Serial.read();
        uartFlush();

        switch( choice )
        {
          case '0':
            uartDisablePowerSafe();
            menuStatus = DO_MAIN_MENU;
            break;
          case '1':
            uartEnablePowerSafe();
            menuStatus = DO_MAIN_MENU;
            break;
          case '9':
            menuStatus = DO_MAIN_MENU;
            break;
          default:
            break;
        }
      }
      break;
    case DO_1WBUS_MENU:
      if( Serial.available() )
      {
        byte choice = Serial.read();
        uartFlush();

        switch( choice )
        {
          case '0':
            uartPowerOff1W();
            menuStatus = DO_MAIN_MENU;
            break;
          case '1':
            uartPowerOn1W();
            menuStatus = DO_MAIN_MENU;
            break;
          case '9':
            menuStatus = DO_MAIN_MENU;
            break;
          default:
            break;
        }
      }
      break;
    case DO_DIG_PINSWAP_MENU:
      if( Serial.available() )
      {
        byte choice = Serial.read();
        uartFlush();

        switch( choice )
        {
          case '0':
            uartNoSwapDigPins();
            menuStatus = DO_MAIN_MENU;
            break;
          case '1':
            uartSwapDigPins();
            menuStatus = DO_MAIN_MENU;
            break;
          case '9':
            menuStatus = DO_MAIN_MENU;
            break;
          default:
            break;
        }
      }
      break;
    case DO_BRIGHTNESS_MENU:
      while( Serial.available() )
      {
        byte choice = Serial.read();
        switch( choice )
        {
            case '0':            
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              inputLength++;
              if( numericInput > 0 )
              {
                numericInput *= 10;
              }
              numericInput += ( choice - '0' );
              break;
            case 0x0d:
            case 0x0a:
            case 'x':
            case 'X':
              inputComplete = true;
              break;
            default:
              break;
        }

        if( numericInput >= 100 || inputComplete )
        {
          if( inputLength && choice != 'X' && choice != 'x' )
          {
            uartSetCurrentBrightness( numericInput );
          }

          menuStatus = DO_MAIN_MENU;
          uartFlush();
          inputComplete = false;
          numericInput = 0;
          inputLength = 0;
        }
      }
      break;
    case DO_CONTRAST_MENU:
      while( Serial.available() )
      {
        byte choice = Serial.read();
        switch( choice )
        {
            case '0':            
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              inputLength++;
              if( numericInput > 0 )
              {
                numericInput *= 10;
              }
              numericInput += ( choice - '0' );
              break;
            case 0x0d:
            case 0x0a:
            case 'x':
            case 'X':
              inputComplete = true;
              break;
            default:
              break;
        }

        if( numericInput >= 100 || inputComplete )
        {
          if( inputLength && choice != 'X' && choice != 'x' )
          {
            uartSetCurrentContrast( numericInput );
          }

          menuStatus = DO_MAIN_MENU;
          uartFlush();
          inputComplete = false;
          numericInput = 0;
          inputLength = 0;
        }
      }
      break;
#ifdef UART_REMOTE_CONTROL
    case DO_UART_CONTROL:
      uartControlRun(false);
      break;
#endif // UART_REMOTE_CONTROL
    default:
      break;
  }
}
// ------------------------------- END UART MENU HANDLING ------------------------------
//

//
// ------------------------- END SERIAL CONTROL AND MENU STUFF -------------------------
//
#endif // USE_SERIAL



#ifdef USE_DIG_ENCODER
//
// ---------------------------- DIG BRIGHTNESS CHANGE LOOP -----------------------------
//
void encoderChangeBrightness( void )
{
  static int16_t last, value;

  bool brightnessExit = false;

  lcd.clear();

  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 0);

  displayItem( OUTPUT_DEVICE_LCD, ITEM_BRIGHTNESS_HEADER_TEXT,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );

  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 1);

  displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_CHANGE_VALUE_PROMPT,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );

  while( !brightnessExit )
  {

    encoder->service(); 

    value += encoder->getValue();

    if (value != last) 
    {
      if( value > last )
      {
        if( currentBrightness < 255 )
        {
          currentBrightness++;
        }
      }
      else
      {
        if( currentBrightness > 0 )
        {
          currentBrightness--;
        }
      }
      analogWrite(PIN_BRIGHTNESS, 255-currentBrightness);
      last = value;

      if( lcdType == LCD_TYPE_1602 )
      {
        lcd.setCursor(6, 1);
      }
      else
      {
        lcd.setCursor(8, 1);
      }

      lcd.print(" ");
      if( currentBrightness < 100 )
      {
        lcd.print(" ");
      }

      if( currentBrightness < 10 )
      {
        lcd.print(" ");
      }

      lcd.print(currentBrightness);
      lcd.print(" ");

    }

    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open) 
    {
      switch (b) 
      {
        case ClickEncoder::Clicked:
          brightnessExit = true;
          break;
        case ClickEncoder::DoubleClicked:
          brightnessExit = true;
          break;
        default:
          break;
      }
    }
  }
  lcd.clear();
}
//
// -------------------------- END DIG BRIGHTNESS CHANGE LOOP ---------------------------
//
#endif // USE_DIG_ENCODER


#ifdef USE_DIG_ENCODER
//
// ----------------------------- DIG CONTRAST CHANGE LOOP ------------------------------
//
void encoderChangeContrast( void )
{
  static int16_t last, value;

  bool contrastExit = false;

  lcd.clear();
  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 0);

  displayItem( OUTPUT_DEVICE_LCD, ITEM_CONTRAST_HEADER_TEXT,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );

  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 1);

  displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_CHANGE_VALUE_PROMPT,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );

  value = encoder->getValue();
    
  while( !contrastExit )
  {
    encoder->service(); 

    value += encoder->getValue();

    if (value != last) 
    {
      if( value > last )
      {
        if( currentContrast < 255 )
        {
          currentContrast++;
        }
      }
      else
      {
        if( currentContrast > 0 )
        {
          currentContrast--;
        }
      }

      analogWrite(PIN_CONTRAST, 255-currentContrast);
      last = value;

      if( lcdType == LCD_TYPE_1602 )
      {
        lcd.setCursor(6, 1);
      }
      else
      {
        lcd.setCursor(8, 1);
      }

      lcd.print(" ");
      if( currentContrast < 100 )
      {
        lcd.print(" ");
      }

      if( currentContrast < 10 )
      {
        lcd.print(" ");
      }

      lcd.print(currentContrast);
      lcd.print(" ");

    }

    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open) 
    {
      switch (b) 
      {
        case ClickEncoder::Clicked:
          contrastExit = true;
          break;
        case ClickEncoder::DoubleClicked:
          contrastExit = true;
          break;
        default:
          break;
      }
    }
  }
  lcd.clear();
}
//
// --------------------------- END DIG CONTRAST CHANGE LOOP ----------------------------
//
#endif // USE_DIG_ENCODER



#ifdef USE_DIG_ENCODER
//
// ------------------------------- DIG SAVE SETTINGS -----------------------------------
//
void encoderSaveSettings( void )
{
  lcd.setCursor(0, 1);
  displayItem( OUTPUT_DEVICE_LCD, ITEM_TEXT_STORE,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
  delay(1000);

  saveSettings();

  lcd.setCursor(0, 1);
  displayItem( OUTPUT_DEVICE_LCD, ITEM_TEXT_DONE,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
  delay(1000);
}
//
// ------------------------------ END DIG SAVE SETTINGS --------------------------------
//
#endif // USE_DIG_ENCODER


#ifdef USE_DIG_ENCODER
//
// ------------------------------- DIG RESET TO DEAULTS --------------------------------
//
void encoderSetToDefault( void )
{

  lcd.setCursor(0, 1);
  displayItem( OUTPUT_DEVICE_LCD, ITEM_TEXT_RESET,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
  delay(1000);

  reset2Defaults();

  lcd.setCursor(0, 1);
  displayItem( OUTPUT_DEVICE_LCD, ITEM_TEXT_DONE,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
  delay(1000);

}
//
// ----------------------------- END DIG RESET TO DEAULTS ------------------------------
//
#endif // USE_DIG_ENCODER


#ifdef USE_DIG_ENCODER
//
// --------------------------------- DIG SWAP DIG PINS ---------------------------------
//
void encoderSwapDigPins( void )
{

  static int16_t last, value;
  bool swapDigPinsChangeExit = false;
  static int currItem;

  lcd.clear();
  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 0);

  displayItem( OUTPUT_DEVICE_LCD, ITEM_DIG_PINS_HEADER_TEXT,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );

  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 1);

  if( swapDigPins )
  {
    displayItem( OUTPUT_DEVICE_LCD, ITEM_MENU_DIG_PINSWAP,
                     "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
    currItem = 1;
  }
  else
  {
    displayItem( OUTPUT_DEVICE_LCD, ITEM_MENU_DIG_NO_PINSWAP,
                     "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
    currItem = 0;
  }

  if( lcdType == LCD_TYPE_2004 )
  {
    lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);

    displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                     "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
  }

  value = encoder->getValue();

  while( !swapDigPinsChangeExit )
  {
    encoder->service(); 

    value += encoder->getValue();

    if (value != last) 
    {
      if( value > last )
      {
        if( currItem < 2 )
        {
          currItem++;
        }
        else
        {
          currItem = 0;
        }
      }
      else
      {
        if( currItem > 0 )
        {
          currItem--;
        }
        else
        {
          currItem = 2;
        }
      }

      last = value;

      // lcd.setCursor(COLUMN, LINE);
      lcd.setCursor(0, 1);

      switch( currItem )
      {
        case 0:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_MENU_DIG_NO_PINSWAP,
                           "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 1:
        displayItem( OUTPUT_DEVICE_LCD, ITEM_MENU_DIG_PINSWAP,
                         "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 2:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_TEXT_BACK_ACTION,
                           "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        default:
          break;
      }

    }

    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open) 
    {
      switch (b) 
      {
        case ClickEncoder::Clicked:

          switch( currItem )
          {
            case 0:                             // LEN_ITEM_DIG_NO_PINSWAP
              swapDigPins = false;
              break;
            case 1:                             // LEN_ITEM_DIG_PINSWAP
              swapDigPins = true;
              break;
            case 2:                             // ITEM_TEXT_CANCEL_ACTION
              swapDigPinsChangeExit = true;
              break;
            default:
              break;
          }
          break;
        case ClickEncoder::DoubleClicked:
          swapDigPinsChangeExit = true;
          break;
        default:
          break;
      }
    }
  }
  lcd.clear();
}
//
// ------------------------------- END DIG SWAP DIG PINS -------------------------------
//
#endif // USE_DIG_ENCODER


#ifdef USE_DIG_ENCODER
//
// --------------------------------- DIG POWER 1W BUS ----------------------------------
//
void encoderPower1WBus( void )
{

  static int16_t last, value;
  bool power1WBusChangeExit = false;
  static int currItem;

  lcd.clear();
  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 0);

  displayItem( OUTPUT_DEVICE_LCD, ITEM_1WBUS_HEADER_TEXT,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );

  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 1);

  if( current1WBusPower )
  {
    displayItem( OUTPUT_DEVICE_LCD, ITEM_1WBUS_POWER_OFF,
                     "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
    currItem = 0;
  }
  else
  {
    displayItem( OUTPUT_DEVICE_LCD, ITEM_1WBUS_POWER_ON,
                     "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
    currItem = 1;
  }

  if( lcdType == LCD_TYPE_2004 )
  {
    lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);

    displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                     "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
  }

  value = encoder->getValue();

  while( !power1WBusChangeExit )
  {
    encoder->service(); 

    value += encoder->getValue();

    if (value != last) 
    {
      if( value > last )
      {
        if( currItem < 2 )
        {
          currItem++;
        }
        else
        {
          currItem = 0;
        }
      }
      else
      {
        if( currItem > 0 )
        {
          currItem--;
        }
        else
        {
          currItem = 2;
        }
      }

      last = value;

      // lcd.setCursor(COLUMN, LINE);
      lcd.setCursor(0, 1);

      switch( currItem )
      {
        case 0:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_1WBUS_POWER_OFF,
                           "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 1:
        displayItem( OUTPUT_DEVICE_LCD, ITEM_1WBUS_POWER_ON,
                         "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 2:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_TEXT_BACK_ACTION,
                           "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        default:
          break;
      }

    }

    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open) 
    {
      switch (b) 
      {
        case ClickEncoder::Clicked:

          switch( currItem )
          {
            case 0:                             // ITEM_1WBUS_POWER_OFF
              powerOff1W();
              break;
            case 1:                             // ITEM_1WBUS_POWER_ON
              powerOn1W();
              break;
            case 2:                             // ITEM_TEXT_CANCEL_ACTION
              power1WBusChangeExit = true;
              break;
            default:
              break;
          }
          break;
        case ClickEncoder::DoubleClicked:
          power1WBusChangeExit = true;
          break;
        default:
          break;
      }
    }
  }
  lcd.clear();
}
//
// ------------------------------- END DIG POWER 1W BUS --------------------------------
//
#endif // USE_DIG_ENCODER




#ifdef USE_DIG_ENCODER
//
// ---------------------------------- DIG POWER SAFE -----------------------------------
//
void encoderPowerSafe( void )
{
  static int16_t last, value;
  bool powerSafeChangeExit = false;
  static int currItem;

  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 0);

  displayItem( OUTPUT_DEVICE_LCD, ITEM_POWERSAFE_HEADER_TEXT,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );

  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 1);

  if( currentPowerSafeMode )
  {
    displayItem( OUTPUT_DEVICE_LCD, ITEM_POWERSAFE_OFF,
                     "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
    currItem = 0;
  }
  else
  {
    displayItem( OUTPUT_DEVICE_LCD, ITEM_POWERSAFE_ON,
                     "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
    currItem = 1;
  }
  
  if( lcdType == LCD_TYPE_2004 )
  {
    lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);

    displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                     "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
  }

  value = encoder->getValue();

  while( !powerSafeChangeExit )
  {
    encoder->service(); 

    value += encoder->getValue();

    if (value != last) 
    {
      if( value > last )
      {
        if( currItem < 2 )
        {
          currItem++;
        }
        else
        {
          currItem = 0;
        }
      }
      else
      {
        if( currItem > 0 )
        {
          currItem--;
        }
        else
        {
          currItem = 2;
        }
      }

      last = value;

      // lcd.setCursor(COLUMN, LINE);
      lcd.setCursor(0, 1);

      switch( currItem )
      {
        case 0:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_POWERSAFE_OFF,
                           "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 1:
        displayItem( OUTPUT_DEVICE_LCD, ITEM_POWERSAFE_ON,
                         "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 2:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_TEXT_BACK_ACTION,
                           "", false, TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        default:
          break;
      }
    }

    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open) 
    {
      switch (b) 
      {
        case ClickEncoder::Clicked:

          switch( currItem )
          {
            case 0:                             // ITEM_POWERSAFE_OFF
              disablePowerSafe();
              break;
            case 1:                             // ITEM_POWERSAFE_ON
              enablePowerSafe();
              break;
            case 2:                             // ITEM_TEXT_CANCEL_ACTION
              powerSafeChangeExit = true;
              break;
            default:
              break;
          }
          break;
        case ClickEncoder::DoubleClicked:
          powerSafeChangeExit = true;
          break;
        default:
          break;
      }
    }
  }
  lcd.clear();
}
//
// -------------------------------- END DIG POWER SAFE ---------------------------------
//
#endif // USE_DIG_ENCODER




#ifdef USE_DIG_ENCODER
//
// ----------------------------------- DIG MENU LOOP -----------------------------------
//
void encoderMenu( void )
{

  static int16_t last, value;
  int currMenuItem = 0;
  bool menuExit = false;
  bool entryCall = true;

  lcd.clear();
  // lcd.setCursor(COLUMN, LINE);
  lcd.setCursor(0, 0);

  displayItem( OUTPUT_DEVICE_LCD, ITEM_MAIN_HEADER_TEXT,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );

  if( lcdType == LCD_TYPE_1602 )
  {
    lcd.setCursor(0, LCD_16x2_LINE_SELECT_ITEM);
  }
  else
  {
    lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);
  }

  displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                   "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );

  if( lcdType == LCD_TYPE_1602 )
  {
    delay(2000);
  }
  
  value = encoder->getValue();
    
  while( !menuExit )
  {
    encoder->service(); 

    value += encoder->getValue();

    if (entryCall || (value != last) ) 
    {
      if( value < last )
      {
        // left
        if( currMenuItem > 0 )
        {
          currMenuItem--;
        }
        else
        {
          currMenuItem = MENU_NUMBER_ITEMS - 1;
        }
      }
      else
      {
        if( value > last )
        {
          // right
          if( currMenuItem < (MENU_NUMBER_ITEMS-1) )
          {
            currMenuItem++;
          }
          else
          {
            currMenuItem = 0;
          }
        }
      }
      entryCall = false;

// display Item
      // lcd.setCursor(COLUMN, LINE);
      if( lcdType == LCD_TYPE_1602 )
      {
        lcd.setCursor(0, LCD_16x2_LINE_DISPLAY_ITEM);
      }
      else
      {
        lcd.setCursor(0, LCD_20x4_LINE_DISPLAY_ITEM);
      }

      switch(currMenuItem)
      {
        // perform action depending on currMenuItem
        case 0:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_BRIGHTNESS,
                           "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 1:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_CONTRAST,
                           "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 2:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_SWAP_DIG_PINS,
                           "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 3:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_1WBUS,
                           "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 4:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_POWERSAFE,
                           "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 5:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_DEVICE_SCAN,
                           "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 6:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_SAVE_SETTINGS,
                           "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 7:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_DEFAULTS,
                           "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        case 8:
          displayItem( OUTPUT_DEVICE_LCD, ITEM_EXIT_MENU,
                           "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
          break;
        default:
          break;
      }

      last = value;

    }

    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open) 
    {
      switch (b) 
      {
        case ClickEncoder::Clicked:

          switch(currMenuItem)
          {
            // perform action depending on currMenuItem
            case 0:                // ITEM_BRIGHTNESS
              encoderChangeBrightness();
              if( lcdType == LCD_TYPE_1602 )
              {
                lcd.setCursor(0, LCD_16x2_LINE_SELECT_ITEM);
              }
              else
              {
                lcd.setCursor(0, 0);
                displayItem( OUTPUT_DEVICE_LCD, ITEM_MAIN_HEADER_TEXT,
                             "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
                lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);
              }
              displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                               "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
              entryCall = true;
              break;
            case 1:                // ITEM_CONTRAST
              encoderChangeContrast();
              if( lcdType == LCD_TYPE_1602 )
              {
                lcd.setCursor(0, LCD_16x2_LINE_SELECT_ITEM);
              }
              else
              {
                lcd.setCursor(0, 0);
                displayItem( OUTPUT_DEVICE_LCD, ITEM_MAIN_HEADER_TEXT,
                             "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
                lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);
              }
              displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                               "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
              entryCall = true;
              break;
            case 2:                // ITEM_CONTRAST
              encoderSwapDigPins();
              if( lcdType == LCD_TYPE_1602 )
              {
                lcd.setCursor(0, LCD_16x2_LINE_SELECT_ITEM);
              }
              else
              {
                lcd.setCursor(0, 0);
                displayItem( OUTPUT_DEVICE_LCD, ITEM_MAIN_HEADER_TEXT,
                             "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
                lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);
              }
              displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                               "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
              entryCall = true;
              break;
            case 3:                // ITEM_1WBUS
              encoderPower1WBus();
              if( lcdType == LCD_TYPE_1602 )
              {
                lcd.setCursor(0, LCD_16x2_LINE_SELECT_ITEM);
              }
              else
              {
                lcd.setCursor(0, 0);
                displayItem( OUTPUT_DEVICE_LCD, ITEM_MAIN_HEADER_TEXT,
                             "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
                lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);
              }
              displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                               "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
              entryCall = true;
              break;
            case 4:                // ITEM_POWERSAFE
              encoderPowerSafe();
              if( lcdType == LCD_TYPE_1602 )
              {
                lcd.setCursor(0, LCD_16x2_LINE_SELECT_ITEM);
              }
              else
              {
                lcd.setCursor(0, 0);
                displayItem( OUTPUT_DEVICE_LCD, ITEM_MAIN_HEADER_TEXT,
                             "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
                lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);
              }
              displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                               "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
              entryCall = true;
              break;
            case 5:                // ITEM_DEVICE_SCAN
              doScan();
              if( lcdType == LCD_TYPE_1602 )
              {
                lcd.setCursor(0, LCD_16x2_LINE_SELECT_ITEM);
              }
              else
              {
                lcd.setCursor(0, 0);
                displayItem( OUTPUT_DEVICE_LCD, ITEM_MAIN_HEADER_TEXT,
                             "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
                lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);
              }
              displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                               "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
              entryCall = true;
              break;
            case 6:                // ITEM_SAVE_SETTINGS
              encoderSaveSettings();
              if( lcdType == LCD_TYPE_1602 )
              {
                lcd.setCursor(0, LCD_16x2_LINE_SELECT_ITEM);
              }
              else
              {
                lcd.setCursor(0, 0);
                displayItem( OUTPUT_DEVICE_LCD, ITEM_MAIN_HEADER_TEXT,
                             "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
                lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);
              }
              displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                               "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
              entryCall = true;
              break;
            case 7:                // ITEM_DEFAULTS
              encoderSetToDefault();
              if( lcdType == LCD_TYPE_1602 )
              {
                lcd.setCursor(0, LCD_16x2_LINE_SELECT_ITEM);
              }
              else
              {
                lcd.setCursor(0, 0);
                displayItem( OUTPUT_DEVICE_LCD, ITEM_MAIN_HEADER_TEXT,
                             "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
                lcd.setCursor(0, LCD_20x4_LINE_SELECT_ITEM);
              }
              displayItem( OUTPUT_DEVICE_LCD, ITEM_LCD_SELECT_ITEM_PROMPT,
                               "", false,  TEXT_ALIGN_CENTER, lcdNumColumns );
              entryCall = true;
              break;
            case 8:                // ITEM_EXIT_MENU
              menuExit = true;
              break;
            default:
              break;
          }

          break;
        case ClickEncoder::DoubleClicked:
          menuExit = true;
          break;
        case ClickEncoder::Held:
          break;
        case ClickEncoder::Released:
          break;
      }
    }
  }

  lcd.clear();
}


void encoderCheck (void )
{

  static long beginHold;

  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) 
  {
    switch (b) 
    {
      case ClickEncoder::Clicked:
        doTestRun();
        idleDisplay(true);
        break;
      case ClickEncoder::DoubleClicked:
        encoderMenu();
        idleDisplay(true);
        break;
      case ClickEncoder::Held:
        if( !beginHold )
        {
          beginHold = millis();
        }
        else
        {
          if( millis() - beginHold >= TIME_HOLD_FOR_CONTRAST )
          {
            encoderChangeContrast();
            encoderSaveSettings();
            idleDisplay(true);
            beginHold = 0;
          }
        }
        break;
      case ClickEncoder::Released:
        if( beginHold != 0 )
        {
          if( millis() - beginHold >= TIME_HOLD_FOR_CONTRAST )
          {
            encoderChangeContrast();
            encoderSaveSettings();
            idleDisplay(true);
          }
          beginHold = 0;
        }
        break;
      default:
        break;
    }
  }
}

//
// --------------------------------- END DIG MENU LOOP ---------------------------------
//
#endif // USE_DIG_ENCODER

//
// ------------------------------------- MAIN LOOP -------------------------------------
//
void loop(void)
{

#ifdef USE_DIG_ENCODER
  encoder->service(); 
#endif // USE_DIG_ENCODER

#ifdef USE_MENU
  #ifdef USE_SERIAL

  uartMenu();

  #endif // USE_SERIAL
#endif // USE_MENU

#ifdef USE_DIG_ENCODER

  encoderCheck();

#else  // pervious hardware w/o dig

  if( digitalRead(PIN_PUSH_BUTTON) == LOW )
  {
    doTestRun();
  }
#endif // USE_DIG_ENCODER

  idleDisplay(false);
}

/* ------------------------- no needed stuff behind this line -------------------------- */

