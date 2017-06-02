//
// ************************************************************************
//
// uart_api (c) 2017 Dirk Schanz aka dreamshader
//    add on for: atmega ds18x20 tester (c) 2017 by fsa
// 
//
// ************************************************************************
//
// At this point a "thank you very much" to all the authors sharing
// their work, knowledge and all the very useful stuff.
// You did a great job!
//
// ************************************************************************
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ************************************************************************
//
//-------- brief description ---------------------------------------------
//
//
//
//-------- History -------------------------------------------------------
//
// 1st version: 05/22/17
//         basic function
// update:
//
//
// ************************************************************************
//

// 
// -------------------------- INCLUDE SECTION ---------------------------
// 


#include <stdint.h>
#include <stdio.h>

#ifndef __i386__
#include <Arduino.h>
#else // __i386__
#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#endif // __i386__


// #include "auto_tester.h"

#include "uart_api.h"


// 
// ------------------------ VERSION INFORMATION -------------------------
//
static byte softwareMajorRelease = 0;
static byte softwareMinorRelease = 4;

static byte protocolMajorRelease = 0;
static byte protocolMinorRelease = 1;



// 
// ---------------------------- GLOBAL STUFF ----------------------------
//


struct _err_status _uart_error[] = {
{ UART_CTL_E_PROTOCOL, "" },
{ UART_CTL_E_NULLP,    "" },
{ UART_CTL_E_STATUS,   "" },
{ UART_CTL_E_ARGCNT,   "" },
{ UART_CTL_E_OPCODE,   "" },
{ UART_CTL_E_CRC,      "" },
{ UART_CTL_E_TIMEOUT,  "" },
{ UART_CTL_E_OVERFLOW, "" },
{ UART_CTL_E_OK,       "" },
{ UART_CTL_E_UNSPEC,   "" },
{ 0,                 0L }
};

uint8_t _opcode[] = {
//
// system telegrams below 0x30
//
OPCODE_FIRMWARE_VERSION,
OPCODE_HARDWARE_VERSION,
OPCODE_PROTOCOL_VERSION,
OPCODE_RESPONSE,
OPCODE_HANGUP,
OPCODE_RESEND,
//
// control telegrams from 0x30
//
OPCODE_CMD_1ST_SENSOR_ID,
OPCODE_CMD_NEXT_SENSOR_ID,
OPCODE_CMD_1ST_SENSOR_TEMPERATURE,
OPCODE_CMD_NEXT_SENSOR_TEMPERATURE,
OPCODE_CMD_SENSOR_TEMPERATURE,
OPCODE_CMD_1ST_SENSOR_DATA,
OPCODE_CMD_NEXT_SENSOR_DATA,
OPCODE_CMD_SENSOR_DATA,
OPCODE_CMD_1ST_SENSOR_GET_RESOLUTION,
OPCODE_CMD_NEXT_SENSOR_GET_RESOLUTION,
OPCODE_CMD_SENSOR_GET_RESOLUTION,
OPCODE_CMD_1ST_SENSOR_SET_RESOLUTION,
OPCODE_CMD_NEXT_SENSOR_SET_RESOLUTION,
OPCODE_CMD_SENSOR_SET_RESOLUTION,
OPCODE_CMD_1WBUS_POWER_ON,
OPCODE_CMD_1WBUS_POWER_OFF,
OPCODE_CMD_1WBUS_RESET,
OPCODE_CMD_1WBUS_RESET_SEARCH,
OPCODE_CMD_1WBUS_SELECT_ID,
OPCODE_CMD_PARASITIC_CONVERSION,
OPCODE_CMD_NO_PARASITIC_CONVERSION,
OPCODE_CMD_READ_SCRATCHPAD,
OPCODE_CMD_RUN_VERBOSE,
OPCODE_CMD_RUN_SUMMARY,
OPCODE_CMD_RUN_QUIET,
//
END_OF_OPCODES_MARKER  // MUST STAY AT THIS POS!
};

int8_t _uartErrorCode;
uint8_t currentSequence;

char uartRdBuffer[128];


byte makeVersion( byte major, byte minor )
{
  return( (major << 4) | minor  );
}

byte getSoftwareVersion( void )
{
  return( makeVersion( softwareMajorRelease, softwareMinorRelease ));
}

byte getProtocolVersion( void )
{
  return( makeVersion( protocolMajorRelease, protocolMinorRelease ));
}


// ----------------------------------------------------------------------
// byte CRC8(const byte *data, byte len)
//
// CRC-8 - based on the CRC8 formulas by Dallas/Maxim
// code released under the therms of the GNU GPL 3.0 license
//
// ----------------------------------------------------------------------
byte CRC8(const byte *data, byte len) 
{
  byte crc = 0x00;
  while (len--) 
  {
    byte extract = *data++;
    for (byte tempI = 8; tempI; tempI--) 
    {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) 
      {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}




bool uartCompleteTelegram( struct _uart_telegram_ *pTelegram )
{
  bool retVal = false;

  if( pTelegram != NULL )
  {
    if( pTelegram->_arg_cnt > 0 )
    {
      if( pTelegram->_arg_cnt > REMOTE_COMMAND_MAX_ARGS )
      {
        pTelegram->_arg_cnt = REMOTE_COMMAND_MAX_ARGS;
        _uartErrorCode = UART_CTL_W_DATA_DROPPED;
      }
      pTelegram->_crc8 = CRC8((const byte *) pTelegram->_args, pTelegram->_arg_cnt);
    }
    else
    {
      pTelegram->_crc8 = UART_PROTO_CRC_IGNORE;
    }

    pTelegram->_sequence = currentSequence++;

    retVal = true;
  }
  return( retVal );
}


#ifndef __i386__

void dumpTelegram( struct _uart_telegram_ *p_telegram )
{

  if( p_telegram != NULL )
  {
    Serial.println();
    Serial.println("Telegram:");

    Serial.print("p_telegram->_opcode: ");
    Serial.println(p_telegram->_opcode, HEX);

    Serial.print("p_telegram->_crc8: ");
    Serial.println(p_telegram->_crc8, HEX);

    Serial.print("p_telegram->_sequence: ");
    Serial.println(p_telegram->_sequence, HEX);

    Serial.print("p_telegram->_status: ");
    Serial.println(p_telegram->_status, HEX);

    Serial.print("p_telegram->_arg_cnt: ");
    Serial.println(p_telegram->_arg_cnt, HEX);

    Serial.print("p_telegram->_args: ");
    for( int i = 0; i < p_telegram->_arg_cnt && 
                    i < REMOTE_COMMAND_MAX_ARGS; i++ )
    {
      Serial.print(p_telegram->_args[i], HEX);
    }

    Serial.println();
  }
}


#else

void dumpTelegram( struct _uart_telegram_ *p_telegram )
{

  if( p_telegram != NULL )
  {
    printf("\n\nTelegram:\n");
    printf("p_telegram->_opcode .: 0x%02x\n", p_telegram->_opcode);
    printf("p_telegram->_crc8 ...: 0x%02x\n", p_telegram->_crc8);
    printf("p_telegram->_sequence: 0x%02x\n", p_telegram->_sequence);
    printf("p_telegram->_status .: 0x%02x\n", p_telegram->_status);
    printf("p_telegram->_arg_cnt : 0x%02x\n", p_telegram->_arg_cnt);
    printf("p_telegram->_args ...: ");

    for( int i = 0; i < p_telegram->_arg_cnt && 
                    i < REMOTE_COMMAND_MAX_ARGS; i++ )
    {
      printf("0x%02x ", p_telegram->_args[i]);
    }
    printf("\n");

    if( p_telegram->_arg_cnt > 0 )
    {
      switch( p_telegram->_args[0] )
      {
        case OPCODE_CMD_1ST_SENSOR_ID:
        case OPCODE_CMD_NEXT_SENSOR_ID:
          printf(" --- Sensor-ID: ");
          printf( "%02x-", p_telegram->_args[1] );
          for ( int i = 7; i > 1; i--)
          {
            printf( "%02x", p_telegram->_args[i] );
          }
          printf("\n");
          break;
        default:
          break;
      }
    }
  }
}

#endif // __i386__





// ----------------------------------------------------------------------
// void clearTelegram( struct _uart_telegram_ *p_telegram )
//
// set all telegram elements to 0
// ----------------------------------------------------------------------
void clearTelegram( struct _uart_telegram_ *p_telegram )
{

  if( p_telegram != NULL )
  {
    p_telegram->_opcode = 0;
    p_telegram->_crc8 = 0;
    p_telegram->_sequence = 0;
    p_telegram->_status = 0;
    p_telegram->_arg_cnt = 0;

    memset(p_telegram->_args, '\0', REMOTE_COMMAND_MAX_ARGS);
  }
}



bool uartSendResponse( struct _uart_telegram_ *p_command,
                       struct _uart_telegram_ *p_response )
{
  bool retVal = false;

  return( retVal );
}

#ifndef __i386__

extern byte getFirstSensorID( byte addr[]  );
extern byte getNextSensorID( byte addr[]  );

void uartMakeDummyResponse( struct _uart_telegram_ *p_command,
                       struct _uart_telegram_ *p_response )
{
  p_response->_opcode =   OPCODE_RESPONSE;
  p_response->_args[0] = p_command->_opcode;
  p_response->_arg_cnt = 1;
}


void uartMakeAddrResponse( byte opSuccess, byte sensorID[],
                           struct _uart_telegram_ *p_command,
                           struct _uart_telegram_ *p_response )
{
  p_response->_opcode =   OPCODE_RESPONSE;
  p_response->_status = opSuccess;
  p_response->_args[0] = p_command->_opcode;
  p_response->_arg_cnt = 1;

  for( int i = 0; i < 8; i++ )
  {
    p_response->_args[i+1] = sensorID[i];
    p_response->_arg_cnt++;
  }

}





bool uartControlRunCommand( struct _uart_telegram_ *p_command,
                            struct _uart_telegram_ *p_response )
{

  static byte W1Address[8];
  static byte data[12];
  float celsius;
  byte resolution;
  long conversionTime;
  byte opSuccess;

  bool retVal = false;

  if( p_command != NULL )
  {
    switch( p_command->_opcode )
    {
      //
      // system telegrams below 0x30
      //
      case OPCODE_FIRMWARE_VERSION:                // get firmware version
      case OPCODE_HARDWARE_VERSION:                // get hardware version
      case OPCODE_PROTOCOL_VERSION:                // get protocol version
      case OPCODE_RESPONSE:                        // telegram contains response data
      case OPCODE_HANGUP:                          // quit connection (hangup)
      case OPCODE_RESEND:                          // resend telegram 
        uartMakeDummyResponse( p_command, p_response );
        uartCompleteTelegram( p_response );
        uartSendTelegram( p_response );
        break;
      //
      // control telegrams from 0x30
      //
      case OPCODE_CMD_1ST_SENSOR_ID:               // get 1st sensor id
        opSuccess = getFirstSensorID(W1Address);
        uartMakeAddrResponse( opSuccess, W1Address, p_command, p_response );
        uartCompleteTelegram( p_response );
        uartSendTelegram( p_response );
        break;
      case OPCODE_CMD_NEXT_SENSOR_ID:              // get next sensor id
        opSuccess = getNextSensorID(W1Address);
        uartMakeAddrResponse( opSuccess, W1Address, p_command, p_response );
        uartCompleteTelegram( p_response );
        uartSendTelegram( p_response );
        break;
      case OPCODE_CMD_1ST_SENSOR_TEMPERATURE:      // get temp for 1st sensor
      case OPCODE_CMD_NEXT_SENSOR_TEMPERATURE:     // get temp for next sensor
      case OPCODE_CMD_SENSOR_TEMPERATURE:          // get temp for sensor with id
      case OPCODE_CMD_1ST_SENSOR_DATA:             // get data block for 1st sensor
      case OPCODE_CMD_NEXT_SENSOR_DATA:            // get data block for next sensor
      case OPCODE_CMD_SENSOR_DATA:                 // get data block for sensor with id
      case OPCODE_CMD_1ST_SENSOR_GET_RESOLUTION:   // get resolution for 1st sensor
      case OPCODE_CMD_NEXT_SENSOR_GET_RESOLUTION:  // get resolution for next sensor
      case OPCODE_CMD_SENSOR_GET_RESOLUTION:       // get resolution for sensor with id
      case OPCODE_CMD_1ST_SENSOR_SET_RESOLUTION:   // set resolution for 1st sensor
      case OPCODE_CMD_NEXT_SENSOR_SET_RESOLUTION:  // set resolution for next sensor
      case OPCODE_CMD_SENSOR_SET_RESOLUTION:       // set resolution for sensor with id
      case OPCODE_CMD_1WBUS_POWER_ON:              // power on 1w bus
      case OPCODE_CMD_1WBUS_POWER_OFF:             // power off 1w bus
      case OPCODE_CMD_1WBUS_RESET:                 // reset 1w bus
      case OPCODE_CMD_1WBUS_RESET_SEARCH:          // reset_search 1w bus
      case OPCODE_CMD_1WBUS_SELECT_ID:             // select id on 1W bus

      case OPCODE_CMD_PARASITIC_CONVERSION:        // start conversion parasitic power
      case OPCODE_CMD_NO_PARASITIC_CONVERSION:     // start conversion no parasitic power
      case OPCODE_CMD_READ_SCRATCHPAD:             // read scratchpad
      case OPCODE_CMD_RUN_VERBOSE:                 // run testsequence send results
      case OPCODE_CMD_RUN_SUMMARY:                 // run testsequence send summary
      case OPCODE_CMD_RUN_QUIET:                   // run testsequence discard output
      //
        uartMakeDummyResponse( p_command, p_response );
        uartCompleteTelegram( p_response );
        uartSendTelegram( p_response );
        retVal = true;
        break;
      default:
        _uartErrorCode = UART_CTL_E_OPCODE;
        uartMakeDummyResponse( p_command, p_response );
        uartCompleteTelegram( p_response );
        uartSendTelegram( p_response );
        break;
    }
  }
  else
  {
    _uartErrorCode = UART_CTL_E_NULLP;
  }

  return( retVal );

}


#endif // __i386__


#ifndef __i386__


int uartSendTelegram( struct _uart_telegram_ *pTelegram )
{
  int retVal = -1;

  if( pTelegram != NULL )
  {
    Serial.write(pTelegram->_opcode);
    Serial.write(pTelegram->_crc8);
    Serial.write(pTelegram->_sequence);
    Serial.write(pTelegram->_status);
    Serial.write(pTelegram->_arg_cnt);
    for( int i = 0; i < pTelegram->_arg_cnt;i++ )
    {
      Serial.write(pTelegram->_args[i]);
    }
  }

  return( retVal );

}

#else

int uartSendTelegram( int fd, struct _uart_telegram_ *pTelegram )
{
  int retVal = -1;
  _uartErrorCode = UART_CTL_E_OK;

  if( pTelegram != NULL )
  {
    write(fd, &pTelegram->_opcode, 1);
    write(fd, &pTelegram->_crc8, 1);
    write(fd, &pTelegram->_sequence, 1);
    write(fd, &pTelegram->_status, 1);
    write(fd, &pTelegram->_arg_cnt, 1);
    if( pTelegram->_arg_cnt > 0 )
    {
      write(fd, pTelegram->_args, pTelegram->_arg_cnt);
    }
    retVal = 0;
  }
  else
  {
    _uartErrorCode = UART_CTL_E_NULLP;
  }

  return( retVal );

}

#endif // __i386__

#ifndef __i386__

void uartConnectionResponse( void )
{
  struct _uart_telegram_ response;

  response._opcode =   OPCODE_RESPONSE;
  response._sequence = currentSequence++;
  response._status =  _uartErrorCode;
  response._arg_cnt =  0x02;

  response._args[0] = makeVersion( softwareMajorRelease, softwareMinorRelease );
  response._args[1] = makeVersion( protocolMajorRelease, protocolMinorRelease );

  response._crc8 = CRC8(response._args, response._arg_cnt);

  uartSendTelegram( &response );
}

#endif // __i386__




