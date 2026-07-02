// -> esp32-2432S028R
#include "time.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <LittleFS.h>

#include <lvgl.h>
#include "LGFX_Driver.h"

SemaphoreHandle_t file_mutex;

#define RX_PIN 22
#define TX_PIN 27
#define BAUD_RATE 115200

#define BUFFER_SIZE 64
char buffer[BUFFER_SIZE];

volatile float data_weight = 0;
hw_timer_t* timer_data = NULL;
volatile bool flag_read_data = false;

void initial_port() {
  Serial1.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(1000);
}

void IRAM_ATTR change_flag() {
  flag_read_data = true;
}

void initial_timer() {
  timer_data = timerBegin(1000000);
  timerAttachInterrupt(timer_data, &change_flag);
  timerAlarm(timer_data, 10000, true, 0);
  timerStart(timer_data);
}

// -> file_system
  /**
    Данные на esp32-2432S028R будут хранится в папке expirements в течении 3-х дней, 
    после этого все эксперименты будут удалены.
  */
  #define EXP_DIR "/experiments" // -> папка, где будут хранится все файлы с данными об экспериментах
  #define EXP_LIFETIME 3 * 24 * 3600 // -> 3 дня в секундах

  File expirement_file;
  String current_expirement;
  bool expirement_active = false;

  void initial_file_system() {
    if (!LittleFS.begin(true)) {
      Serial.println("LOGGER_SYSTEM: Error create File System");
      return;
    }
    if (!LittleFS.exists(EXP_DIR)) {
      LittleFS.mkdir(EXP_DIR);
    }
    Serial.println("LOGGER_SYSTEM: Successfuly create Diraction");
  }

  /**
    каждый файл будет иметь расширение .csv
    каждый файл будет назван так, чтобы его имя соответствовало дате проведения опыта
  */
  String create_expirement_name() { // -> создание нового эксперимента
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      return "/experiments/unknown.csv";
    }

    char filename[64];
    sprintf(
      filename, 
      "/experiments/exp_%04d%02d%02d_%02d%02d%02d.csv", 
      (timeinfo.tm_year + 1900),
      (timeinfo.tm_mon + 1),
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    );
    return String(filename);
  }

  /**
    как было ранее сказано: файлы будут хранится в течении 3-х дней,
    а после они будут удалены
  */
  void delate_expirements() { // -> удаление данных об эксперименте
    File root = LittleFS.open(EXP_DIR);
    if (!root) {
      return;
    }

    time_t now;
    time(&now);
    File file = root.openNextFile();

    while (file) {
      String name = file.name();
      if (name.endsWith(".csv")) {
        struct tm tm_file = {0};
        int year, month, day;
        int hour, minute, second;

        sscanf(
          name.c_str(), 
          "/experiments/exp_%4d%2d%2d_%2d%2d%2d.csv",
          &year, &month, &day,
          &hour, &minute, &second 
        );

        tm_file.tm_year = year - 1900;
        tm_file.tm_mon = month - 1;
        tm_file.tm_mday = day;
        tm_file.tm_hour = hour;
        tm_file.tm_min = minute;
        tm_file.tm_sec = second;

        time_t file_time = mktime(&tm_file);
        if (difftime(now, file_time) > EXP_LIFETIME) {
          file.close();
          Serial.print("DELETE: ");
          Serial.println(name);

          LittleFS.remove(name);
        }
      }
      file = root.openNextFile();
    }
  }
// -> file_system

// -> initial_server
  // bool flag_expirement = false;
  String current_datetime = "0000-00-00 00:00:00";

  IPAddress ip(192, 168, 2, 1);
  IPAddress getaway(192, 168, 2, 1);
  IPAddress subnet(255, 255, 255, 0);

  AsyncWebServer server(80);

  const char* ssid = "Torex";
  const char* password = "Torex12345";

  const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="ru">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Torex Logger</title>
      <style>
        * {
          margin: 0;
          padding: 0;
          box-sizing: border-box;
          font-family: Arial, Helvetica, sans-serif;
        }
        body {
          background: #f5f5f5;
          padding: 20px;
        }
        .container {
          width: 100%;
          max-width: 1200px;
          margin: 0 auto;
        }
        h1 {
          text-align: center;
          margin-bottom: 20px;
        }
        .card {
          background: white;
          border-radius: 10px;
          padding: 18px;
          margin-bottom: 20px;
          box-shadow: 0 2px 10px rgba(0, 0, 0, .15);
        }
        #chart {
          width: 100%;
          height: 320px;
          display: block;
        }
        .info {
          display: grid;
          grid-template-columns:repeat(3, 1fr);
          gap: 20px;
        }
        .item {
          text-align: center;
        }
        .label {
          color: #666;
          font-size: 14px;
        }
        .value {
          margin-top: 8px;
          font-size: 32px;
          font-weight: bold;
        }
        .buttons {
          display: flex;
          flex-wrap:wrap;
          justify-content: center;
          gap: 15px;
        }
        button {
          flex:1;
          min-width:170px;
          max-width:250px;
          height:50px;
        }
        .start {
          background: #2ecc71;
          color: white;
        }
        .stop {
          background: #e74c3c;
          color: white;
        }
        .download {
          background: #3498db;
          color: white;
        }
        .table-wrapper {
          overflow-x:auto;
        }
        table {
          width: 100%;
          border-collapse: collapse;
        }
        th, td {
          padding: 10px;
          border-bottom: 1px solid #ddd;
          text-align: center;
        }
        .statusRun {
          color: green;
        }
        .statusStop {
          color: red;
        }

        @media (max-width: 900px) {
          .info {
            grid-template-columns: 1fr;
            gap: 15px;
          }
          .buttons {
            flex-direction: column;
            align-items: stretch;
          }
          button {
            width: 100%;
            max-width: none;
          }
          .value {
            font-size: 28px;
          }
          h1 {
            font-size: 28px;
          }
          #chart {
            height: 260px;
          }
        }

        @media (max-width: 600px) {
          body {
            padding: 10px;
          }
          .card {
            padding: 14px;
          }
          h1 {
            font-size: 22px;
          }
          .value {
            font-size: 22px;
          }
          .label {
            font-size: 13px;
          }
          button {
            height: 45px;
            font-size: 16px;
          }
          #chart {
            height: 220px;
          }
          th,td {
            padding: 8px;
            font-size: 14px;
          }
        }
      </style>
    </head>
    <body>
      <div class="container">
        <h1>TOREX DATA LOGGER</h1>
        <div class="card">
          <div class="info">
            <div class="item">
              <div class="label">Время</div>
              <div id="time" class="value">--:--:--</div>
            </div>
            <div class="item">
              <div class="label">Вес</div>
              <div id="weight" class="value">0.000</div>
            </div>
            <div class="item">
              <div class="label">Статус</div>
              <div id="status" class="value statusStop">STOP</div>
            </div>
          </div>
      </div>
      <div class="card">
        <div class="buttons">
          <button class="start" onclick="startExperiment()">
            Старт
          </button>
          <button class="stop" onclick="stopExperiment()">
            Стоп
          </button>
          <button class="download" onclick="downloadData()">
            Скачать
          </button>
        </div>
      </div>
      <div class="card">
        <canvas id="chart"></canvas>
      </div>
      <div class="card">
        <h2 style="margin-bottom:15px;">Последние измерения</h2>
        <div class="table-wrapper">
          <table>
            <thead>
              <tr>
                <th>Время</th>
                <th>Вес</th>
              </tr>
            </thead>
            <tbody id="table">
            </tbody>
          </table>
        </div>
      </div>
      </div>
      <script>
        const canvas = document.getElementById("chart");
        const ctx = canvas.getContext("2d");
        const MAX_POINTS = 100;

        let values = [];
        let labels = [];

        function resizeCanvas() {
          canvas.width = canvas.clientWidth;
          canvas.height = canvas.clientHeight;
          drawGraph();
        }
  
        function drawGraph() {
          ctx.clearRect(0, 0, canvas.width, canvas.height);

          if (values.length < 2)
            return;

          let minValue = Math.min(...values);
          let maxValue = Math.max(...values);

          if (maxValue === minValue) {
            maxValue += 1;
            minValue -= 1;
          }

          const left = 60;
          const right = 20;
          const top = 20;
          const bottom = 50;

          const width = canvas.width - left - right;
          const height = canvas.height - top - bottom;

          ctx.beginPath();

          for (let i = 0; i <= 5; i++) {
            const y = top + i * height / 5;
            ctx.moveTo(left, y);
            ctx.lineTo(canvas.width - right, y);
          }

          ctx.strokeStyle = "#dddddd";
          ctx.lineWidth = 1;
          ctx.stroke();

          ctx.beginPath();

          ctx.moveTo(left, top);
          ctx.lineTo(left, canvas.height - bottom);

          ctx.lineTo(canvas.width - right, canvas.height - bottom);

          ctx.strokeStyle = "#000";
          ctx.lineWidth = 2;
          ctx.stroke();

          ctx.beginPath();

          values.forEach((value, index) => {
            const x = left + index * width / (MAX_POINTS - 1);
            const y = top + height - ((value - minValue) / (maxValue - minValue)) * height;

            if (index === 0)
              ctx.moveTo(x, y);
            else
              ctx.lineTo(x, y);
          });

          ctx.strokeStyle = "#0066ff";
          ctx.lineWidth = 2;
          ctx.stroke();

          values.forEach((value, index) => {
            const x = left + index * width / (MAX_POINTS - 1);
            const y = top + height - ((value - minValue) / (maxValue - minValue)) * height;
            
            ctx.beginPath();
            ctx.arc(x, y, 3, 0, Math.PI * 2);
            ctx.fillStyle = "#0066ff";
            ctx.fill();
          });

          ctx.fillStyle = "#000";
          ctx.font = "13px Arial";

          for (let i = 0; i <= 5; i++) {
            const value = maxValue - i * (maxValue - minValue) / 5;
            const y = top + i * height / 5;
            ctx.fillText(value.toFixed(2), 5, y + 4);
          }

          const step = Math.max(1, Math.floor(labels.length / 5));
          for (let i = 0; i < labels.length; i += step) {
            const x = left + i * width / (MAX_POINTS - 1);
            ctx.fillText(labels[i], x - 18, canvas.height - 20);
          }

          ctx.save();

          ctx.translate(18, canvas.height / 2);
          ctx.rotate(-Math.PI / 2);

          ctx.font = "bold 15px Arial";
          // ctx.fillText("Масса, г", 0, 0);
          ctx.restore();
          ctx.font = "bold 15px Arial";

          // ctx.fillText("Время", canvas.width / 2 - 25, canvas.height - 5);
        }

        function addPoint(time,weight) {
          weight = parseFloat(weight);

          if(isNaN(weight))
            return;

          document.getElementById("time").innerHTML=time;
          document.getElementById("weight").innerHTML = weight.toFixed(3);

          values.push(weight);
          labels.push(time);

          if(values.length > MAX_POINTS) {
            values.shift();
            labels.shift();
          }

          drawGraph();
          const table = document.getElementById("table");
          const row = table.insertRow(0);

          row.insertCell(0).innerHTML = time;
          row.insertCell(1).innerHTML = weight.toFixed(3);
          
          if(table.rows.length > 15)
            table.deleteRow(15);
        }
  
        async function updateWeight() {
          try {
            const response = await fetch("/getData");
            const text = await response.text();
            const weight = parseFloat(text);
            const now =new Date();
            const time = now.toLocaleTimeString();
            addPoint(time,weight);
          } catch(error) {
            console.log(error);
          }
        }

        async function sendTime(){
          const now = new Date();
          const date = now.getFullYear() + "-" +
          String(now.getMonth()+1).padStart(2,'0') + "-" +
          String(now.getDate()).padStart(2,'0');

          const time = String(now.getHours()).padStart(2,'0') + ":" + 
          String(now.getMinutes()).padStart(2,'0') + ":" +
          String(now.getSeconds()).padStart(2,'0');

          const datetime = date + " " + time;
          await fetch("/getDate?datetime=" + datetime);
        }
  
        async function startExperiment() {
          await fetch("/start");
          const status = document.getElementById("status");
          status.innerHTML="RUN";
          status.className = "value statusRun";
        } 
        
        async function stopExperiment() {
          await fetch("/stop");
          const status = document.getElementById("status");
          status.innerHTML="STOP";
          status.className = "value statusStop";
        }

        function downloadData() {
          window.location = "/downloadData";
        }
        window.addEventListener("resize", resizeCanvas);
        window.onload = () => {
          resizeCanvas();
          sendTime();
        };
        setInterval(updateWeight, 500);
      </script>
    </body>
    </html>
  )rawliteral";

  void set_rtc_data(String time) { // -> установка времени для rtc-модуля esp32
    struct tm timeinfo = {0};
    
    int year;
    int months;
    int day;
    int hour;
    int minute;
    int second;
    sscanf(time.c_str(), "%d-%d-%d %d:%d:%d", &year, &months, &day, &hour, &minute, &second);

    // -> date
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = months - 1;
    timeinfo.tm_mday = day;

    // -> time
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;

    time_t times = mktime(&timeinfo);
    struct timeval now;

    now.tv_sec = times;
    now.tv_usec = 0;
    settimeofday(&now, nullptr);
  }

  String get_current_datetime() { // -> получение времени из модуля rtc
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      return "0000-00-00 00:00:00";
    }
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
  }

  void intial_wifi_ap() {
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(ip, getaway, subnet);
    delay(1000);
  }

  void start_server() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      request -> send_P(200, "text/html", index_html);
    });

    server.on("/getDate", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (request -> hasParam("datetime")) {
        current_datetime = request -> getParam("datetime") -> value();
        set_rtc_data(current_datetime);
        request -> send(200, "text/plain", "Ok");
        return;
      }
      request -> send(500, "text/plain", "Error get Data Date.");
    });

    server.on("/start", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (xSemaphoreTake(file_mutex, portMAX_DELAY)) {
        delate_expirements();
        if (expirement_file) {
          expirement_file.close();
        }

        current_expirement = create_expirement_name();
        expirement_file = LittleFS.open(current_expirement,FILE_WRITE);
        if (!expirement_file) {
          xSemaphoreGive(file_mutex);
          request->send(500, "text/plain","File Error");
          return;
        }

        expirement_file.println("time,weight");
        expirement_file.flush();

        expirement_active = true;
        Serial.print("START ");
        Serial.println(current_expirement);
        xSemaphoreGive(file_mutex);
      }
      request->send(200, "text/plain", "START_EXPERIMENT");
    });

    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (xSemaphoreTake(file_mutex, portMAX_DELAY)) {
        expirement_active = false;
        if (expirement_file) {
          expirement_file.close();
        }
        xSemaphoreGive(file_mutex);
      }

      Serial.println("STOP");
      // flag_expirement = false;
      request -> send(200, "text/plain", "STOP_EXPIREMENT");
    });

    server.on("/getData", HTTP_GET, [](AsyncWebServerRequest* request) {
      request -> send(200, "text/plain", String(data_weight));
    });

    server.on("/downloadData", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (current_expirement == "") {
        request -> send(404, "text/plain", "Not Found Experiment");
        return;
      }

      if (!LittleFS.exists(current_expirement)) {
        request -> send(404, "text/plain", "File Not Found");
        return;
      }

      File download_file = LittleFS.open(current_expirement, FILE_READ); 
      if (!download_file) {
        request->send(404, "text/plain", "File open error");
        return;
      }
      request -> send(download_file, current_expirement, "text/csv", true);
    });
    server.begin();
  }
// -> initial_server

// -> lvgl
  static lv_disp_draw_buf_t draw_buf;
  static lv_color_t buf[320 * 10];
  static lv_disp_drv_t disp_drv;

  void my_flush_disp(lv_disp_drv_t* disp,
                     const lv_area_t* area,
                     lv_color_t* color_p) { // -> инициализация буфера
    uint32_t width = area -> x2 - area -> x1 + 1;
    uint32_t height = area -> y2 - area -> y1 + 1;

    lcd.startWrite();
    lcd.setAddrWindow(area -> x1, area -> y1, width, height);
    lcd.writePixels(reinterpret_cast<lgfx::rgb565_t *>(color_p), width * height);
    lcd.endWrite();
    lv_disp_flush_ready(disp);
  }

  // -> GUI
    lv_obj_t* header_panel;
    lv_obj_t* label_main;
    lv_obj_t* label_date;
    lv_obj_t* label_date_value;
    lv_obj_t* label_time;
    lv_obj_t* label_time_value;

    lv_obj_t* main_panel;
    lv_obj_t* weight_label;
    lv_obj_t* weight_label_value;

    lv_obj_t* status_main_panel;
    lv_obj_t* status_label;
    lv_obj_t* icon_status_label;

    lv_obj_t* chart_panel;
    lv_obj_t* chart;

    lv_obj_t* footer_panel;
    lv_obj_t* btn_start;
    lv_obj_t* btn_stop;
    lv_obj_t* btn_save;

    lv_obj_t* create_panel(lv_obj_t* parent, 
                           int width, 
                           int height,
                           lv_align_t align, 
                           int x_position, 
                           int y_position) {
      lv_obj_t* new_panel = lv_obj_create(parent);
      lv_obj_set_size(new_panel, width, height);
      lv_obj_align(new_panel, align, x_position, y_position);
      return new_panel;
    }

    lv_obj_t* create_label(lv_obj_t* parent, 
                           const char* text, 
                           lv_align_t align, 
                           int x_position, 
                           int y_position) {
      lv_obj_t* new_label  = lv_label_create(parent);
      lv_label_set_text(new_label, text);
      lv_obj_align(new_label, align, x_position, y_position);
      return new_label;
    }

    void create_header_part_window(lv_obj_t* scr) {
      header_panel = create_panel(scr, 320, 60, LV_ALIGN_TOP_MID, 0, 0);
      label_date = create_label(header_panel, "DATE:", LV_ALIGN_TOP_LEFT, 0, -6);
      label_time = create_label(header_panel, "TIME:", LV_ALIGN_BOTTOM_LEFT, 0, 4);
      label_main = create_label(header_panel, "TOREX LOGGER", LV_ALIGN_TOP_RIGHT, 0, 5);
      label_date_value = create_label(header_panel, "0000.00.00", LV_ALIGN_TOP_LEFT, 45, -6);
      label_time_value = create_label(header_panel, "00:00:00", LV_ALIGN_BOTTOM_LEFT, 45, 4);
    }

    lv_obj_t* create_indicator(lv_obj_t* scr, int width, int height) {
      lv_obj_t* icon_indicator = lv_obj_create(scr);
      lv_obj_remove_style_all(icon_indicator);
      lv_obj_set_size(icon_indicator, width, height);
      lv_obj_set_style_radius(icon_indicator, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_bg_color(icon_indicator, lv_palette_main(LV_PALETTE_RED), 0);
      lv_obj_set_style_bg_opa(icon_indicator, LV_OPA_COVER, 0);
      lv_obj_set_style_border_width(icon_indicator, 0, 0);
      lv_obj_set_style_shadow_width(icon_indicator, 0, 0);
      lv_obj_align(icon_indicator, LV_ALIGN_TOP_MID, 0, 0);
      return icon_indicator;
    }

    void create_main_part_window(lv_obj_t* scr) {
      main_panel = create_panel(scr, 200, 65, LV_ALIGN_TOP_LEFT, 0, 60);
      weight_label = create_label(main_panel, "Weight", LV_ALIGN_BOTTOM_MID, 0, 5);
      weight_label_value = create_label(main_panel, "000.000 g", LV_ALIGN_TOP_MID, 0, 0);
      status_main_panel = create_panel(scr, 120, 65, LV_ALIGN_TOP_RIGHT, 0, 60);
      status_label = create_label(status_main_panel, "STOP", LV_ALIGN_BOTTOM_MID, 0, 5);
      icon_status_label = create_indicator(status_main_panel, 20, 20);
    }

    lv_obj_t* create_chart(lv_obj_t* scr, lv_align_t align) {
      lv_obj_t* chart = lv_chart_create(scr);
      lv_obj_set_size(chart, 300, 60);
      lv_obj_set_style_line_color(chart, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
      lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
      lv_obj_set_style_line_opa(chart, LV_OPA_COVER, LV_PART_MAIN);
      lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
      lv_chart_set_point_count(chart, 100);
      lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
      lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_align(chart, align, 0, -10);
      return chart;
    }

    void create_chart_panel(lv_obj_t* scr) {
      chart_panel = create_panel(scr, 320, 80, LV_ALIGN_TOP_MID, 0, 125);
      chart = create_chart(chart_panel, LV_ALIGN_TOP_MID);
    }

    lv_obj_t* create_btn(lv_obj_t* scr, 
                         lv_color_t color,
                         const char* txt, 
                         int width, 
                         int height,
                         lv_align_t align,
                         int x_position,
                         int y_position) {
      lv_obj_t* btn = lv_btn_create(scr);
      lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN);
      lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_size(btn, width, height);
      lv_obj_align(btn, align, x_position, y_position);
      lv_obj_t* label_btn = create_label(btn, txt, LV_ALIGN_CENTER, 0, 0);
      return btn;
    }

    void create_footer_panel(lv_obj_t* scr) {
      footer_panel = create_panel(scr, 320, 35, LV_ALIGN_BOTTOM_MID, 0, 0);
      btn_start = create_btn(footer_panel, lv_palette_main(LV_PALETTE_GREEN), "START", 80, 28, LV_ALIGN_TOP_LEFT, 0, -11);
      btn_stop = create_btn(footer_panel, lv_palette_main(LV_PALETTE_RED), "STOP", 80, 28, LV_ALIGN_TOP_MID, 0, -11);
      btn_save = create_btn(footer_panel, lv_palette_main(LV_PALETTE_BLUE), "STOP", 80, 28, LV_ALIGN_TOP_RIGHT, 0, -11);
    }

    void create_window_logger() {
      lv_obj_t* scr = lv_scr_act();
      create_header_part_window(scr);
      create_main_part_window(scr);
      create_chart_panel(scr);
      create_footer_panel(scr);
    }
  // -> GUI

  void initial_display() { // -> инициализация дисплея
    lcd.init();
    lcd.setRotation(1);

    lv_init();
    lv_disp_draw_buf_init(
      &draw_buf,
      buf,
      NULL,
      320 * 10
    );

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;

    disp_drv.flush_cb = my_flush_disp;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
  }
// -> lvgl

void setup() {
  Serial.begin(115200);
  delay(1000);

  file_mutex = xSemaphoreCreateMutex();

  initial_port();
  initial_timer();

  initial_file_system();

  intial_wifi_ap();
  start_server();

  initial_display();
  create_window_logger();
}

void loop() {
  if (flag_read_data) {
    flag_read_data = false;
    if (Serial1.available()) {
      size_t read_bytes = Serial1.readBytesUntil('\n', buffer, BUFFER_SIZE - 1);
      if (read_bytes > 0) {
        buffer[read_bytes] = '\0';
        char *p = buffer;
        while (*p && !isdigit(*p) && *p != '-' && *p != '+') {
          p++;
        }
        if (*p != '\0') {
          data_weight = atof(p);
          Serial.print("Weight: ");
          Serial.println(data_weight, 3);
          if (expirement_active && expirement_file) {
            if (xSemaphoreTake(file_mutex, portMAX_DELAY)) {
              expirement_file.print(get_current_datetime());
              expirement_file.print(",");
              expirement_file.println(data_weight, 3);
              expirement_file.flush();
              xSemaphoreGive(file_mutex);
            }
          }
        } else {
          Serial.println("Error: weight not found!");
        }
      }
    }
  }

  lv_timer_handler();
  delay(5);
}
