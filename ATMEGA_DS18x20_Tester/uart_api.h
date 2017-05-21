#ifndef _UART_API_
#define _UART_API_

#ifdef __cplusplus
extern "C" {
#endif


#ifndef byte
  typedef uint8_t byte;
#endif // byte

#ifndef __i386__
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif // __i386__


//
// ---------------------------- UART REMOTE CONTROL HANDLING ---------------------------
//

#define UART_CTL_E_PROTOCOL        -9      // protocol mismatch
#define UART_CTL_E_NULLP           -8      // null pointer
#define UART_CTL_E_STATUS          -7      // status field out of range
#define UART_CTL_E_ARGCNT          -6      // invalid buffer size
#define UART_CTL_E_OPCODE          -5      // unknown opcode for telegram
#define UART_CTL_E_CRC             -4      // crc fail for telegram
#define UART_CTL_E_TIMEOUT         -3      // uart timeout
#define UART_CTL_E_OVERFLOW        -2      // buffer overflow
#define UART_CTL_E_UNSPEC          -1      // unspecific error / no add. information
#define UART_CTL_E_OK               0      // no error
#define UART_CTL_W_DATA_DROPPED     1      // warning: dropped some payload

#define UART_CTL_TIMEOUT          500      // 500 ms timeout to read from UART

#define UART_PROTO_STATUS_IGNORE    0
#define UART_PROTO_CRC_IGNORE       0


#define REMOTE_COMMAND_MAX_ARGS    20      // size of arg buffer
#define UART_CTLBUF_SIZE           32      // 32 byte for UART remote control buffer
#define REMOTE_COMMAND_HDR_LENGTH   5


struct _err_status {
int8_t status;
const char *errmsg;
};

struct _uart_telegram_ {
uint8_t _opcode;
uint8_t _crc8;
uint8_t _sequence;
int8_t _status;
uint8_t _arg_cnt;
uint8_t _args[REMOTE_COMMAND_MAX_ARGS];
};
// uart_telegram_t, *p_uart_telegram_t;

//
// system telegrams below 0x30
//
#define OPCODE_FIRMWARE_VERSION               0x01   // get firmware version
#define OPCODE_HARDWARE_VERSION               0x02   // get hardware version
#define OPCODE_PROTOCOL_VERSION               0x03   // get protocol version
#define OPCODE_RESPONSE                       0x04   // telegram contains response data
#define OPCODE_HANGUP                         0x05   // quit connection (hangup)
#define OPCODE_RESEND                         0x06   // resend telegram 
//
// control telegrams from 0x30
//
#define OPCODE_CMD_1ST_SENSOR_ID              0x30   // get 1st sensor id
#define OPCODE_CMD_NEXT_SENSOR_ID             0x31   // get next sensor id
#define OPCODE_CMD_1ST_SENSOR_TEMPERATURE     0x32   // get temp for 1st sensor
#define OPCODE_CMD_NEXT_SENSOR_TEMPERATURE    0x33   // get temp for next sensor
#define OPCODE_CMD_SENSOR_TEMPERATURE         0x34   // get temp for sensor with id
#define OPCODE_CMD_1ST_SENSOR_DATA            0x35   // get data block for 1st sensor
#define OPCODE_CMD_NEXT_SENSOR_DATA           0x36   // get data block for next sensor
#define OPCODE_CMD_SENSOR_DATA                0x37   // get data block for sensor with id
#define OPCODE_CMD_1ST_SENSOR_GET_RESOLUTION  0x38   // get resolution for 1st sensor
#define OPCODE_CMD_NEXT_SENSOR_GET_RESOLUTION 0x39   // get resolution for next sensor
#define OPCODE_CMD_SENSOR_GET_RESOLUTION      0x3a   // get resolution for sensor with id
#define OPCODE_CMD_1ST_SENSOR_SET_RESOLUTION  0x3b   // set resolution for 1st sensor
#define OPCODE_CMD_NEXT_SENSOR_SET_RESOLUTION 0x3c   // set resolution for next sensor
#define OPCODE_CMD_SENSOR_SET_RESOLUTION      0x3d   // set resolution for sensor with id
#define OPCODE_CMD_1WBUS_POWER_ON             0x4e   // power on 1w bus
#define OPCODE_CMD_1WBUS_POWER_OFF            0x4f   // power off 1w bus
#define OPCODE_CMD_1WBUS_RESET                0x40   // reset 1w bus
#define OPCODE_CMD_1WBUS_RESET_SEARCH         0x41   // reset_search 1w bus
#define OPCODE_CMD_1WBUS_SELECT_ID            0x42   // select id on 1W bus

#define OPCODE_CMD_PARASITIC_CONVERSION       0x43   // start conversion parasitic power
#define OPCODE_CMD_NO_PARASITIC_CONVERSION    0x44   // start conversion no parasitic power
#define OPCODE_CMD_READ_SCRATCHPAD            0x45   // read scratchpad
#define OPCODE_CMD_RUN_VERBOSE                0x46   // run testsequence send results
#define OPCODE_CMD_RUN_SUMMARY                0x47   // run testsequence send summary
#define OPCODE_CMD_RUN_QUIET                  0x48   // run testsequence discard output
//
#define END_OF_OPCODES_MARKER                 0xff    // end of opcodes indicator

//
// ----------------------------------------------------------------------
//

extern struct _err_status _uart_error[];
extern uint8_t _opcode[];
extern char uartRdBuffer[];

extern int8_t _uartErrorCode;
extern uint8_t currentSequence;

//
// ----------------------------------------------------------------------
//

extern void uartFlush( void );

extern bool uartSendResponse( struct _uart_telegram_ *p_command,
                       struct _uart_telegram_ *p_response );

extern byte CRC8(const byte *data, byte len) ;

extern byte makeVersion( byte major, byte minor );

extern bool uartCompleteTelegram( struct _uart_telegram_ *pTelegram );

extern void dumpTelegram( struct _uart_telegram_ *p_telegram );

extern void clearTelegram( struct _uart_telegram_ *p_telegram );

extern bool uartSendResponse( struct _uart_telegram_ *p_command,
                       struct _uart_telegram_ *p_response );

extern bool uartControlRunCommand( struct _uart_telegram_ *p_command,
                            struct _uart_telegram_ *p_response );

extern void uartControlRun( bool reset );

#ifndef __i386__
extern int uartSendTelegram( struct _uart_telegram_ *pTelegram );
#else
extern int uartSendTelegram( int fd, struct _uart_telegram_ *pTelegram );
#endif // __i386__

extern void uartConnectionResponse( void );



//
// ----------------------------------------------------------------------
//

#ifdef __cplusplus
}
#endif

#endif // _UART_API_









