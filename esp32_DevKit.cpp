// -> esp32 DevKit
#include "time.h"

#define RX_PIN_ESP32_2432S028R 25
#define TX_PIN_ESP32_2432S028R 26
#define BOAD_RATE_ESP32_2432S028R 115200

void initial_main_port() {
  Serial1.begin(
    BOAD_RATE_ESP32_2432S028R, 
    SERIAL_8N1, 
    RX_PIN_ESP32_2432S028R, 
    TX_PIN_ESP32_2432S028R
  );
  delay(1000);
}

// -> logger_weight
  #define RX_PIN 16
  #define TX_PIN 17
  #define BOAD_RATE_WEIGHT 9600

  #define BUFFER_SIZE 64
  char buffers[64];
  String output_buffer = "";

  void initial_port_weight() { // -> инициализация порта для чтения данных с весов
    Serial2.begin(
      BOAD_RATE_WEIGHT, 
      SERIAL_8N1, 
      RX_PIN, 
      TX_PIN
    );
    delay(1000);
  }

  volatile float data_weight = 0;
  hw_timer_t* timer_weight = NULL;
  volatile bool flag_read_weight = false;

  void IRAM_ATTR change_flag_weight() {
    flag_read_weight = true;
  }

  void initial_timer_weight() {
    timer_weight = timerBegin(1000000);
    timerAttachInterrupt(timer_weight, &change_flag_weight);
    timerAlarm(timer_weight, 10000, true, 0);
    timerStart(timer_weight);
  }
// -> logger_weight

void setup() {
  Serial.begin(115200);
  delay(1000);

  initial_main_port();
  initial_port_weight();
  initial_timer_weight();
}

void loop() {
  if (flag_read_weight) {
    flag_read_weight = false;
    if (Serial2.available()) {
      size_t read_bytes = Serial2.readBytesUntil('\n', buffers, BUFFER_SIZE - 1);
      buffers[read_bytes] = '\0';
      Serial.println(buffers);
      Serial1.println(buffers);
    }
  }
}
