#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

#define SD_CS PA4

SdFat32 sd;
File32 file;

void printSdError(const char *msg) {
    Serial.println(msg);
    sd.errorPrint(&Serial);

    Serial.print("Error code = 0x");
    Serial.println(sd.sdErrorCode(), HEX);

    Serial.print("Error data = 0x");
    Serial.println(sd.sdErrorData(), HEX);
}

void setup() {
    Serial.setTx(PA9);
    Serial.setRx(PA10);
    Serial.begin(115200);

    delay(5000);

    Serial.println("===== SD WRITE/READ SAME FILE TEST =====");

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    delay(200);

    SPI.begin();
    delay(200);

    // SPI chậm để ổn định khi test breadboard
    SdSpiConfig config(SD_CS, DEDICATED_SPI, SD_SCK_HZ(125000));

    if (!sd.begin(config)) {
        Serial.println("SD begin FAILED");
        sd.initErrorPrint(&Serial);
        return;
    }

    Serial.println("SD begin OK");

    // Xóa file cũ để test sạch
    if (sd.exists("test.txt")) {
        sd.remove("test.txt");
        Serial.println("Old test.txt removed");
    }

    file = sd.open("test.txt", O_RDWR | O_CREAT);

    if (!file) {
        printSdError("Open test.txt FAILED");
        return;
    }

    Serial.println("Writing...");
    file.println("HELLO STM32 FAT32");
    file.println("WRITE READ TEST");

    if (!file.sync()) {
        printSdError("file.sync() FAILED");
        file.close();
        return;
    }

    Serial.println("Write + sync OK");

    // Quay về đầu file để đọc lại
    if (!file.seekSet(0)) {
        printSdError("seekSet(0) FAILED");
        file.close();
        return;
    }

    Serial.println("Reading test.txt:");

    while (file.available()) {
        Serial.write(file.read());
    }

    file.close();

    Serial.println();
    Serial.println("Read OK");
}

void loop() {
    Serial.println("Loop running...");
    delay(2000);
}