#include "mbed.h"
#include "MRF24J40.h"
#include <string>
 
// RF tranceiver to link with handheld.
MRF24J40 mrf(p11, p12, p13, p14, p21);
 
// LEDs you can treat these as variables (led2 = 1 will turn led2 on!)
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// Timer
Timer timer;
 
// Serial port for showing RX data.
Serial pc(USBTX, USBRX);
 
// Used for sending and receiving
char txBuffer[128];
char rxBuffer[128];
int rxLen;

//***************** Do not change these methods (please) *****************//
 
/**
* Receive data from the MRF24J40.
*
* @param data A pointer to a char array to hold the data
* @param maxLength The max amount of data to read.
*/
int rf_receive(char *data, uint8_t maxLength) {
    uint8_t len = mrf.Receive((uint8_t *)data, maxLength);
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};
 
    if(len > 10) {
        //Remove the header and footer of the message
        for(uint8_t i = 0; i < len-2; i++) {
            if(i<8) {
                //Make sure our header is valid first
                if(data[i] != header[i])
                    return 0;
            } else {
                data[i-8] = data[i];
            }
        }
 
        //pc.printf("Received: %s length:%d\r\n", data, ((int)len)-10);
    }
    return ((int)len)-10;
}
 
/**
* Send data to another MRF24J40.
*
* @param data The string to send
* @param maxLength The length of the data to send.
*                  If you are sending a null-terminated string you can pass strlen(data)+1
*/
void rf_send(char *data, uint8_t len) {
    //We need to prepend the message with a valid ZigBee header
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};
    uint8_t *send_buf = (uint8_t *) malloc( sizeof(uint8_t) * (len+8) );
 
    for(uint8_t i = 0; i < len+8; i++) {
        //prepend the 8-byte header
        send_buf[i] = (i<8) ? header[i] : data[i-8];
    }
    //pc.printf("Sent: %s\r\n", send_buf+8);
 
    mrf.Send(send_buf, len+8);
    free(send_buf);
}
 
 
//***************** You can start coding here *****************//
int main (void) {
    //set the radio channel. 0 is default, 15 is max.
    uint8_t channel1 = 6;
    uint8_t channel2 = 7;

    while(true) {
        strcpy(rxBuffer, "");
        strcpy(txBuffer, "");
        strcpy(pcBuffer, "");
        
        //Try to receive some data from the radio
        mrf.SetChannel(channel);     
        rxLen = 0;
        rxLen = rf_receive(rxBuffer, 128);

        //Relay the message to serial
        pc.printf(rxBuffer);

        //Try to recieve some data from the serial
        while(!pc.readable()) pc.gets(txBuffer, 128);

        //Relays the message to radio
        mrf.setChannel(channel2);
        rf_send(txBuffer, strlen(txBuffer) + 1);
    }
    
}