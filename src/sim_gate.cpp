#include <Arduino.h>

#define LED_PIN 21 

// UART設定
#define UART_RX_PIN 44  // UART受信ピン
#define UART_TX_PIN 43  // UART送信ピン
#define UART_BAUD_RATE 115200

// Serial2

void setup() {
    Serial.begin(115200);
    Serial2.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); 
}

void loop() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '1') {
            Serial2.println('1');
        } else if (c == '2') {
            Serial2.println('2');
        } else if (c == '3') {
            Serial2.println('3');
        } else if (c == '4') {
            Serial2.println('4');
        }
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
    }
}