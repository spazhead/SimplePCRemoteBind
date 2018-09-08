#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#include <IRremoteESP8266.h>
#include "ESP8266WiFi.h"
#include <IRrecv.h>
#include <IRutils.h>
#include <EEPROM.h>

uint16_t RECV_PIN = 5;
int SEND_PIN = 4;
int BUTTON_PIN = 14;
int LED_PIN = 2;

IRrecv irrecv(RECV_PIN);

decode_results results;
uint64_t searchval = 0x0;


void setup() {
    WiFi.forceSleepBegin();
    delay(100);
    Serial.begin(115200);
    EEPROM.begin(512);
    irrecv.enableIRIn();
    
    pinMode(SEND_PIN, OUTPUT);
    digitalWrite(SEND_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    pinMode(BUTTON_PIN, INPUT);
    
    while (!Serial) {}  
    delay(50);
    
    Serial.println();
    Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
    Serial.println(RECV_PIN);
    serialPrintUint64(searchval, HEX);

    // Load searchval from EEPROM
    for (int i = 7; i >= 0; i--)
    {
        int shift = i*8;
        int val = EEPROM.read(i);
        Serial.println(val, HEX);
        searchval = searchval | val<<shift;
        Serial.println(val<<shift, HEX);
    }
    Serial.println(byte(searchval>>56), HEX);
    Serial.print("Got ");
    serialPrintUint64(searchval, HEX);
    Serial.println(" from EEPROM");

    // IF button is held at startup
    if (digitalRead(BUTTON_PIN))
    {
        digitalWrite(LED_PIN, LOW);
        // Enter one-time program for searchval
        Serial.println("PROGRAM MODE");
        
        boolean searching = true;
        while (searching)
        {
            // When an input is received
            if (irrecv.decode(&results))
            {
                searchval = results.value;
                
                // Store in EEPROM
                for (int i = 7; i >= 0; i--)
                {
                    int shift = i*8;
                    Serial.print(byte((searchval>>shift)&0xFF), HEX);
                    EEPROM.write(i, byte((searchval>>shift)&0xFF));
                    searching = false;
                }
                EEPROM.commit();
            }
            delay(100);
        }
        digitalWrite(LED_PIN, HIGH);
    }
    
    Serial.println("READY");
}

void loop() {
    // Physical button press
    if (digitalRead(BUTTON_PIN))
    {
        Serial.println("BUTTON PRESS");
        digitalWrite(SEND_PIN, HIGH);
        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(SEND_PIN, LOW);
        digitalWrite(LED_PIN, HIGH);
        delay(500);
    }

    // Detect remote button press
    if (irrecv.decode(&results)) 
    {
        Serial.print("Remote button press with value ");
        serialPrintUint64(results.value, HEX);
        Serial.print(" (looking for ");
        serialPrintUint64(searchval, HEX);
        Serial.println(")");

        // If value (keypress) is right
        if (results.value == searchval)
        {
            // Pulse the transistor/LED
            Serial.println("KEY DOWN");
            digitalWrite(SEND_PIN, HIGH);
            digitalWrite(LED_PIN, LOW);
            delay(100);
            Serial.println("KEY UP");
            digitalWrite(SEND_PIN, LOW);
            digitalWrite(LED_PIN, HIGH);
            // Delay re-enable to slow down double click
            delay(1000);
        }
        irrecv.resume();
    }
    delay(100);
}
