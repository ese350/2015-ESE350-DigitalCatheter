#include "mbed.h"
#include "MRF24J40.h"
#include <string>
 
// RF tranceiver to link with handheld.
MRF24J40 mrf(p11, p12, p13, p14, p21);
 
// LEDs you can treat these as variables (led2 = 1 will turn led2 on!)
DigitalOut led1(LED1); //Indicates top valve is open (ON) or closed (OFF)
DigitalOut led2(LED2); //Indicates top valve is powered (ON) or idle (OFF)
DigitalOut led3(LED3); //Indicates bottom valve is open (ON) or closed (OFF)
DigitalOut led4(LED4); //Indicates bottom valve is powered (ON) or idle (OFF)
 
// Input Shock
DigitalIn shock(p18);

// Inputs to the H-Bridge
DigitalOut INA1(p7);
DigitalOut INA2(p6);
DigitalOut ENA(p5);

DigitalOut INB1(p28);
DigitalOut INB2(p29);
DigitalOut ENB(p30);

// Button Inputs from the valve
DigitalIn top_open(p19);
DigitalIn top_closed(p20);

DigitalIn bottom_open(p16);
DigitalIn bottom_closed(p17);
 
// Timer
Timer timer;
 
// Used for sending and receiving
char txBuffer[128];
char rxBuffer[128];
char str[128];
int rxLen;
enum Valve_Comm {OPENTOP, CLOSETOP, OPENBOTTOM, CLOSEBOTTOM};
 
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
 
 
void sendMessage() {
    rf_send(txBuffer, strlen(txBuffer) + 1);
}

bool valve(Valve_Comm vc) {
    bool ERROR_FLAG = false;
    int ERROR_THRESH = 2000;
    mbed::DigitalIn* STATUSPTR;

    led1 = led2 = led3 = led4 = 0;

    timer.reset();
    timer.start();

    switch(vc) {
        case OPENTOP:
            led1 = 1;

            INA1 = 1;
            INA2 = 0;
            ENA = 1;

            STATUSPTR = &top_open;
            break;
        case CLOSETOP:
            led2 = 1;

            INA1 = 0;
            INA2 = 1;
            ENA = 1;

            STATUSPTR = &top_closed;
            break;
        case OPENBOTTOM:
            led3 = 1;

            INB1 = 1;
            INB2 = 0;
            ENB = 1;

            STATUSPTR = &bottom_open;
            break;
        case CLOSEBOTTOM:
            led4 = 1;

            INB1 = 0;
            INB2 = 1;
            ENB = 1;

            STATUSPTR = &bottom_closed;
            break;
    }

    while(!(*STATUSPTR) && !ERROR_FLAG) {
        ERROR_FLAG = (timer.read_ms() < ERROR_THRESH);
        sendMessage();    
    }

    ENA = ENB = 0;
    led1 = led2 = led3 = led4 = 0;
    timer.stop();

    return ERROR_FLAG;
}

void error_handle(bool ERROR_FLAG) {
    while(ERROR_FLAG) {
        //blinking indicates distress signal
        led1 = led2 = led3 = led4 = 1;
        wait(0.2);
        led1 = led2 = led3 = led4 = 0;
        wait(0.2);
    }
}

void toggleMessage() {
    if(!strcmp(txBuffer,"A")) strcpy(txBuffer, "B");
    else strcpy(txBuffer, "A");
}

int main (void) {
    //set the radio channel. 0 is default, 15 is max.
    uint8_t channel = 1;
    mrf.SetChannel(channel);
    //set the default message to the computer reciever
    strcpy(txBuffer, "A");
    
    while(true) {
        // set valves to collection position
        error_handle(valve(CLOSEBOTTOM));
        error_handle(valve(OPENTOP));
        
        led1 = led2 = led3 = led4 = 1;

        while(!shock) sendMessage();
        toggleMessage();

        led1 = led2 = led3 = led4 = 0;
        
        // sets valves to drainage position
        error_handle(valve(CLOSETOP));
        error_handle(valve(OPENBOTTOM));
  
        //stall until the valve empties
        timer.reset();
        timer.start();
        while(timer.read() < 2) sendMessage();
        timer.stop();
    }
}