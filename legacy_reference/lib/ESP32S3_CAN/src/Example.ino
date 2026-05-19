
#include <CAN.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    if (!CAN.begin(500000)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    Serial.println("CAN Started");
}

void loop() {
    CANMessage msg;

    if (CAN.available()) {
        msg = CAN.receive();
        Serial.print("Received ID: ");
        Serial.println(msg.id, HEX);

        for (int i = 0; i < msg.length; i++) {
            Serial.print(msg.data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    msg.id = 0x123;
    msg.length = 2;
    msg.data[0] = 0xAB;
    msg.data[1] = 0xCD;

    if (CAN.send(msg)) {
        Serial.println("Message Sent");
    } else {
        Serial.println("Error Sending");
    }

    delay(1000);
}
