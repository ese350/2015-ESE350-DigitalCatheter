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

// Used for sending and receiving
char txBuffer[128];
char rxBuffer[128];
int rxLen;

//Used for controlling solenoid
PwmOut pwm(p26);
char ch0[128];
char ch1[128];
char ch2[128];

Serial pc(USBTX, USBRX);

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
    uint8_t channel = 1;
    mrf.SetChannel(channel);

    //pwm settings
    float prev_width = 0.0f;
    float width = 0.0f;
    pwm.period(1.0f);
    pwm.pulsewidth(0.0f);

    int pch0;
    int pch1;
    int pch2;

    while(true) {
        strcpy(rxBuffer, "");
        
        //Try to receive some data from the radio    
        rxLen = 0;
        rxLen = rf_receive(rxBuffer, 128);

        strncpy(ch0, rxBuffer + 0, 1);
        strncpy(ch1, rxBuffer + 1, 1);
        strncpy(ch2, rxBuffer + 2, 1);

        pch0 = strcspn("0123456789", ch0);
        pch1 = strcspn("0123456789", ch1);
        pch2 = strcspn("0123456789", ch2);

        //check if signal is a number
        if(pch0 != 10 && pch1 != 10 && pch2 != 10) {
            width = pch0 + (pch1 / 10.0f) + (pch2 / 100.0f);

            //only modify the pwm when the signal changes
            if(width != prev_width) {
                pwm.pulsewidth(width);
                prev_width = width;   
            }
        }
    }
}