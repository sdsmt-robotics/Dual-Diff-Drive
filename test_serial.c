#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>
#include <signal.h>

size_t bitpackNumber( uint8_t* buffer, int32_t number);
uint16_t crc14( const uint8_t* data, size_t length );
size_t writeKangarooCommand(uint8_t address, uint8_t command, const uint8_t* data, uint8_t length, uint8_t* buffer);
size_t writeKangarooPositionCommand(uint8_t address, char channel, int32_t position, int32_t speedLimit, uint8_t* buffer);
size_t writeKangarooStartCommand(uint8_t address, char channel, char flags, char sequence_code, uint8_t* buffer);
size_t writeKangarooGetCommand(uint8_t address, char channel, char flags, char echo_code, char parameter, uint8_t* buffer);
size_t decodecrc14(char channel, char flags, char echo_code, char sequence_code, char parameter, uint32_t value);
size_t writeKangarooSpeedCommand(uint8_t address, char channel,int flags, int speed, uint8_t*buffer);
void sig_handler(int signo);
int flag;
int main()
{
    uint8_t buffer[30];
    size_t command_return;

    const char* front_port= "/dev/robot/front";
    const char* back_port = "/dev/robot/back";

    int front_fd = open( front_port, O_RDWR | O_NOCTTY | O_NDELAY );
    if( front_fd < 0 )
        printf( "FRONT FAILED\n");

    int back_fd = open( back_port, O_RDWR| O_NOCTTY | O_NDELAY);
    if( back_fd < 0 )
        printf( "BACK FAILED" );
    
    if(signal(SIGINT,sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGINT\n");
    
    //Send Start Command to Channels 1 and 2 for front motor controller
    command_return = writeKangarooStartCommand( 128, 1, 32, 0, buffer);
    if( write( front_fd, buffer, command_return ) < 0 )
        printf("Error writing start to front channel 1");
    
    command_return = writeKangarooStartCommand( 128, 2, 32, 0, buffer);
    if( write( front_fd, buffer, command_return ) < 0 )
        printf("Error writing start to front channel 2"); 

    //Send Start Command to Channels 1 and 2 for back motor controller
    command_return = writeKangarooStartCommand( 128, 1, 32, 0, buffer);
    if( write( back_fd, buffer, command_return) < 0 )
        printf("Error writing startto back Channel 1");

    command_return = writeKangarooStartCommand( 128, 2, 32, 0, buffer);
    if( write( back_fd, buffer, command_return) < 0 )
        printf("Error writing start to back Channel 2");
 while( flag != 1 )
 {
    //Send Speed Command to Front Motor
    command_return = writeKangarooSpeedCommand( 128, 1, 32, 255, buffer);
    if( write( front_fd, buffer, command_return) < 0 )
        printf("Error writing position to front channel 1");
    
    command_return = writeKangarooSpeedCommand( 128, 2, 32, 255, buffer);
    if( write( front_fd, buffer, command_return) < 0 )
        printf("Error writing position to front Channel 2");
    //Send Speed Command to Back Motor 
    command_return = writeKangarooSpeedCommand( 128, 1, 32, 255, buffer);
    if( write( back_fd, buffer, command_return) < 0 )
        printf("Error writing position to back Channel 1");

    command_return = writeKangarooSpeedCommand( 128, 2, 32, 255, buffer);
    if( write( back_fd, buffer, command_return) < 0 )
        printf("Error writing position to back Channel 2");

 }
 //Send Speed Command to Front Motor
 command_return = writeKangarooSpeedCommand( 128, 1, 32, 0, buffer);
 if( write( front_fd, buffer, command_return) < 0 )
     printf("Error writing position to front channel 1");

 command_return = writeKangarooSpeedCommand( 128, 2, 32, 0, buffer);
 if( write( front_fd, buffer, command_return) < 0 )
     printf("Error writing position to front Channel 2");
 //Send Speed Command to Back Motor 
 command_return = writeKangarooSpeedCommand( 128, 1, 32, 0, buffer);
 if( write( back_fd, buffer, command_return) < 0 )
     printf("Error writing position to back Channel 1");

 command_return = writeKangarooSpeedCommand( 128, 2, 32, 0, buffer);
 if( write( back_fd, buffer, command_return) < 0 )
     printf("Error writing position to back Channel 2");



 close(front_fd);
 close(back_fd);


    return 0;



}


/*! Bit-packs a number.
  \param buffer The buffer to write into.
  \param number The number to bit-pack. Should be between -(2^29-1) and 2^29-1.
  \return How many bytes were written (1 to 5). */
size_t bitpackNumber(uint8_t* buffer, int32_t number)
{
    size_t i = 0;
    if( number < 0 ) 
    { 
        number = -number; 
        number <<= 1; 
        number |= 1; 
    }
    else
    { 
        number <<= 1; 
    }
    while(i < 5)
    {
        buffer[i ++] = (number & 0x3f) | (number >= 0x40 ? 0x40 : 0x00);
        number >>= 6;
        if(!number) 
        {
            break;
        }
    }
    return i;
}


/*! Computes a 14-bit CRC. CRC is a check sum
  \param data The buffer to compute the CRC of.
  \param length The length of the data.
  \return The CRC. */
uint16_t crc14( const uint8_t* data, size_t length )
{
    uint16_t crc = 0x3fff; //0011111111111111
    size_t i, bit;
    for( i = 0; i < length; i++ )
    {
        crc ^= data[i] & 0x7f; //01111111
        for( bit = 0; bit < 7; bit++ )
        {
            if(crc & 1) 
            { 
                crc >>= 1; 
                crc ^= 0x22f0; //0010001011110000
            }
            else
            { 
                crc >>= 1; 
            }
        }
    }
    return crc ^ 0x3fff; //0011111111111111
}

size_t decodecrc14( char channel, char flags, char echo_code, char sequence_code, char parameter, uint32_t value)
{

}

/*! Writes a Packet Serial command into a buffer.
  \param address The address of the Kangaroo. By default, this is 128.
  \param command The command number.
  \param data The command data.
  \param length The number of bytes of data.
  \param buffer The buffer to write into.
  \return How many bytes were written. This always equals 4 + length. */
size_t writeKangarooCommand(uint8_t address, uint8_t command, const uint8_t* data, uint8_t length, uint8_t* buffer)
{
    size_t i; uint16_t crc;
    buffer[0] = address; 
    buffer[1] = command;
    buffer[2] = length;
    for(i = 0; i < length; i ++) 
    { 
        buffer[3 + i] = data[i]; 
    }
    crc = crc14(buffer, 3 + length);
    buffer[3 + length] = crc & 0x7f;
    buffer[4 + length] = (crc >> 7) & 0x7f;
    return 5 + length;
}



/*! Writes a Move command for Position into a buffer.
  This could have many, many more options, but I've kept it basic
  to make the example easier to read.
  \param address The address of the Kangaroo. By default, this is 128.
  \param channel The channel name.
  By default, for Independent Mode, these are '1' and '2'.
  For Mixed Mode, these are 'D' and 'T'.
  \param position The position to go to.
  \param speedLimit The speed limit to use. Negative numbers use the default.
  \param buffer The buffer to write into.
  \return How many bytes were written (at most 18). */
size_t writeKangarooPositionCommand(uint8_t address, char channel, int32_t position, int32_t speedLimit, uint8_t* buffer)
{
    uint8_t data[14]; 
    size_t length = 0;
    data[length ++] = (uint8_t)channel;
    // move flags
    data[length ++] = 1;
    // Position
    length += bitpackNumber(&data[length], position);
    if(speedLimit >= 0)
        {
            data[length ++] = 2;
            // Speed (Limit if combined with Position)
            length += bitpackNumber(&data[length], speedLimit);
        }
    return writeKangarooCommand(address, 36, data, length, buffer);
}

/*! Tells the Motor Controller channel to start
    \param address The address of the Kangarro. By default, this i s 128.
    \param channel The channel name.
    \param flags 0 for no options and 64 if sequence code
    \param sequence_code 
    \param buffer The buffer to write into.
    \return How many bytes were written ( at most 18). */
size_t writeKangarooStartCommand(uint8_t address, char channel, char flags, char sequence_code, uint8_t *buffer)
{
    uint8_t data[3];
    size_t length = 0;
    data[length++] = (uint8_t)channel;
    //flags
    data[length++] = (uint8_t) flags;

    //sequence_code optional
    if( flags == 64)
        data[length++] = (uint8_t) sequence_code;

    return writeKangarooCommand( address, 32, data, length, buffer );
}


/*! Tells the Motor Controller channel to return a message
    \param address The address of the Kangarro. By default, this i s 128.
    \param channel The channel name.
    \param flags 0 for no options and 64 if sequence code
    \param echo_code to echo the get command with return message
    \param parameter sets what will be returned by the get command
    \param buffer The buffer to write into.
    \return How many bytes were written ( at most 18). */

size_t writeKangarooGetCommand( uint8_t address, char channel, char flags, char echo_code, char parameter, uint8_t* buffer)
{
    uint8_t data[14];
    size_t length = 0;
    data[length++] = (uint8_t) channel;
    //flags
    data[length++] = (uint8_t) flags;

    //If an echo code
    if( flags == 16)
        data[length++] = (uint8_t) echo_code;

    data[length++] = (uint8_t) parameter;

    return writeKangarooCommand( address, 35, data, length, buffer);
}

size_t writeKangarooSpeedCommand(uint8_t address, char channel,int flags, int speed, uint8_t*buffer)
{
    uint8_t data[9] = { 0 };
    size_t length = 0;
    data[length++] = (uint8_t) channel;

    data[length++] = flags;

    data[length++] = 2;

    length+= bitpackNumber(&data[length], speed);
    return writeKangarooCommand( address, 36, data, length, buffer);
}

void sig_handler(int signo)
{
    if( signo == SIGINT)
    {
        flag = 1;   
    }
}






