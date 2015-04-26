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
DigitalIn A_open(p19);
DigitalIn A_closed(p20);

DigitalIn B_open(p16);
DigitalIn B_closed(p17);
 
// Timer
Timer timer;
 
// Serial port for showing RX data.
Serial pc(USBTX, USBRX);
 
// Used for sending and receiving
char txBuffer[128];
char rxBuffer[128];
char str[128];
int rxLen;
 
//***************** Do not change these methods (please) *****************//
 
/**
* Receive data from the MRF24J40.
*
* @param data A pointer to a char array to hold the data
* @param maxLength The max amount of data to read.
*/
int rf_receive(char *data, uint8_t maxLength)
{
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
void rf_send(char *data, uint8_t len)
{
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
int main (void)
{
    
    uint8_t channel = 6;
    //Set the Channel. 0 is default, 15 is max
    mrf.SetChannel(channel);
    
    pc.printf("Starting");
    
    // Set it to default position - open the top valve
    INA1 = 0;
    INA2 = 1;
    ENA = 1;
    
    while(!A_open){
        wait(0.5);    
    }; // PIN 19
    
    ENA = 0;
    
    // Set it to default position - close the bottom valve
    INB1 = 0;
    INB2 = 1;
    ENB = 1;
    
    while(!B_closed){
        wait(0.5);  
    }; // PIN 17
    ENB = 0;
    
    // Turn on all LEDs.
    led1 = 1;
    led2 = 1;
    led3 = 1;
    led4 = 1;
    
    wait(0.5);    
    
    while(true){
        
        led1 = 0;
        led2 = 0;
        led3 = 0;
        led4 = 1;
        
        ENA = 0;
        
        wait(0.5);
        
        // Send the null message           
        strcpy(txBuffer, "null");            
        rf_send(txBuffer, strlen(txBuffer) + 1);
                
        if(shock) {       

            /***********************/
            // SEND THE VOLUME INFO//
            /***********************/
                 
                // Turn on all LEDs.
                led1 = 1;
                led2 = 1;
                led3 = 1;
                led4 = 1;
                
                //Add to the buffer. You may want to check out sprintf
                strcpy(txBuffer, "shockedddddddddddddddddddddddddddd");     
                rf_send(txBuffer, strlen(txBuffer) + 1);
                
                wait(0.5);
            
                // Send the null message           
                strcpy(txBuffer, "shockedddddddddddddddddddddd");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                        
            /***********************/
            // CLOSE THE TOP VALVE //
            /***********************/
                
                // Turn off all LEDs.
                led1 = 0;
                led2 = 0;
                led3 = 0;
                led4 = 0;
                
                // Reverse direction
                INA1 = 1;
                INA2 = 0;
                ENA = 1;
                
                led1 = 1;
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "null");            
                rf_send(txBuffer, strlen(txBuffer) + 1);  
                
                while(!A_closed){    
                    wait(0.5);
                    // Send the null message           
                    strcpy(txBuffer, "null");            
                    rf_send(txBuffer, strlen(txBuffer) + 1);     
                }; // PIN 20
                
                ENA = 0;
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1);  
            

                
            /*************************/
            // OPEN THE BOTTOM VALVE //
            /*************************/
                
                // Turn on all LEDs.
                led1 = 0;
                led2 = 1;
                led3 = 0;
                led4 = 0;
                
                // Open the bottom valve
                INB1 = 1;
                INB2 = 0;
                ENB = 1;
                
                while(!B_open){
                    wait(0.5);
                    // Send the null message           
                    strcpy(txBuffer, "0");            
                    rf_send(txBuffer, strlen(txBuffer) + 1);      
                }; // PIN 16
                
                // Turn on all LEDs.
                led1 = 0;
                led2 = 1;
                led3 = 0;
                led4 = 1;
                
                ENB = 0;
                              
                // WAIT FOR 2 SECONDS:
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                
                
            /*************************/
            // CLOSE THE BOTTOM VALVE //
            /*************************/ 
            
                INB1 = 0;
                INB2 = 1;
                ENB = 1;
                
                while(!B_closed){
                    wait(0.5);
                    // Send the null message           
                    strcpy(txBuffer, "0");            
                    rf_send(txBuffer, strlen(txBuffer) + 1);  
                }; // PIN 17
                ENB = 0;
                
                // Turn on all LEDs.
                led1 = 1;
                led2 = 1;
                led3 = 1;
                led4 = 1;
                
                wait(0.5);
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1); 
                
                
            /***********************/
            // OPEN THE TOP VALVE //
            /***********************/
            
                INA1 = 0;
                INA2 = 1;
                ENA = 1;
                
                while(!A_open){
                    wait(0.5);
                    // Send the null message           
                    strcpy(txBuffer, "0");            
                    rf_send(txBuffer, strlen(txBuffer) + 1);      
                }; // PIN 19
                
                ENA = 0;
                
                wait(0.5); 
                
                // Send the null message           
                strcpy(txBuffer, "0");            
                rf_send(txBuffer, strlen(txBuffer) + 1);    
        }
    }
}