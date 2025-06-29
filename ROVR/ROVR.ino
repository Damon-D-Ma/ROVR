/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-cam-projects-ebook/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

// Code modified by Damon Ma

// Open browser with http://myesp32.local/ after turning on robot (assuming mdns is supported on your client device)


#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"             // disable brownout problems
#include "soc/rtc_cntl_reg.h"    // disable brownout problems
#include "esp_http_server.h"
#include <ESPmDNS.h>

//servos to control cams
#define SERVO1_PIN 2
#define PWM_FREQ 50         // 50Hz for servo
#define PWM_RES 16          // 16-bit resolution

#define PWM_TIMER LEDC_TIMER_3  // Use timer 3 (avoids camera conflict)
#define PWM_CHANNEL LEDC_CHANNEL_4  // Use channel 4 (any free channel)
int s1_pos = 90;


#define SERVO2_PIN 1
#define PWM_CHANNEL2 LEDC_CHANNEL_5  // Use a different channel
int s2_pos = 90;

#define FLASHLIGHT_PIN 4
bool flashlight_on = false;


//TODO: PUT NETWORK CREDENTIALS HERE
#define NETWORK_NAME ""
#define NETWORK_PWD ""

#define MOTOR_1_PIN_1 14
#define MOTOR_1_PIN_2 15
#define MOTOR_2_PIN_1 13
#define MOTOR_2_PIN_2 12

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

// NOTE THAT THIS CODE IS FOR THE ESP32 CAM AI THINKER ONLY
#define PART_BOUNDARY "123456789000000000000987654321"

  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22


static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";


//Servo left_right_servo;
//Servo up_down_servo;

void setServo1Angle(int angle) {
  angle = constrain(angle, 0, 180);
  
  // Map angle to pulse width in microseconds (typical servo pulse: 500-2400us)
  int pulseWidth = map(angle, 0, 180, 500, 2400);
  
  // Calculate duty based on pulse width and PWM resolution
  // Duty cycle = pulseWidth / 20000us * (2^PWM_RES - 1)
  uint32_t maxDuty = (1 << PWM_RES) - 1;  // 65535 for 16-bit
  uint32_t duty = (uint32_t)((pulseWidth / 20000.0) * maxDuty);
  
  ledcWrite(PWM_CHANNEL, duty);
}


void setServo2Angle(int angle) {
  angle = constrain(angle, 0, 180);
  int pulseWidth = map(angle, 0, 180, 500, 2400);
  uint32_t maxDuty = (1 << PWM_RES) - 1;
  uint32_t duty = (uint32_t)((pulseWidth / 20000.0) * maxDuty);
  ledcWrite(PWM_CHANNEL2, duty);
}



//****************** WEBPAGE CODE BELOW **************************
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<html>
  <head>
    <title>ESP32-CAM</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://fonts.googleapis.com/css2?family=Roboto&display=swap" rel="stylesheet">
    <style>
      body {
        font-family: 'Roboto', sans-serif;
        margin: 0;
        padding: 0;
        text-align: center;
        background-color: #f2f2f2;
      }
      h1 {
        margin-top: 20px;
      }
      img {
        width: auto;
        max-width: 90%;
        height: auto;
        margin: 20px 0;
        border-radius: 10px;
        box-shadow: 0 4px 10px rgba(0,0,0,0.1);
      }
      .container {
        display: flex;
        justify-content: center;
        flex-wrap: wrap;
        gap: 40px;
        padding: 20px;
      }
      .section {
        background-color: #ffffff;
        padding: 20px;
        border-radius: 12px;
        box-shadow: 0 4px 10px rgba(0,0,0,0.1);
        min-width: 280px;
      }
      table {
        margin: 0 auto;
      }
      td {
        padding: 8px;
      }
      .button {
        background-color: #2f4468;
        border: none;
        color: white;
        padding: 10px 20px;
        text-align: center;
        text-decoration: none;
        display: inline-block;
        font-size: 18px;
        margin: 6px 3px;
        cursor: pointer;
        border-radius: 6px;
        user-select: none;
      }
    </style>
  </head>
  <body>
    <h1>ESP32-CAM</h1>
    <img src="" id="photo">

    <div class="container">
      <div class="section">
        <h2>Motor Control</h2>
        <table>
            <tr><td colspan="3" align="center"><button class="button" onmousedown="toggleCheckbox('forward');" ontouchstart="toggleCheckbox('forward');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">Forward</button></td></tr>
            <tr>
              <td align="center"><button class="button" onmousedown="toggleCheckbox('left');" ontouchstart="toggleCheckbox('left');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">Left</button></td>
              <td align="center"><button class="button" onmousedown="toggleCheckbox('stop');" ontouchstart="toggleCheckbox('stop');">Stop</button></td>
              <td align="center"><button class="button" onmousedown="toggleCheckbox('right');" ontouchstart="toggleCheckbox('right');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">Right</button></td>
            </tr>
            <tr><td colspan="3" align="center"><button class="button" onmousedown="toggleCheckbox('backward');" ontouchstart="toggleCheckbox('backward');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">Backward</button></td></tr>
        </table>
      </div>

      <div class="section">
        <h2>Camera Control</h2>
        <table>
          <tr><td colspan="3" align="center"><button class="button" onmousedown="toggleCheckbox('sup');" ontouchstart="toggleCheckbox('sup');">Up</button></td></tr>
          <tr>
            <td align="center"><button class="button" onmousedown="toggleCheckbox('sleft');" ontouchstart="toggleCheckbox('sleft');">Left</button></td>
            <td align="center"><button class="button" onclick="toggleCheckbox('flash')">Flash</button></td>
            <td align="center"><button class="button" onmousedown="toggleCheckbox('sright');" ontouchstart="toggleCheckbox('sright');">Right</button></td>
          </tr>
          <tr><td colspan="3" align="center"><button class="button" onmousedown="toggleCheckbox('sdown');" ontouchstart="toggleCheckbox('sdown');">Down</button></td></tr>
        </table>
      </div>
    </div>

    <script>
      function toggleCheckbox(x) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/action?go=" + x, true);
        xhr.send();
      }

      window.onload = function() {
        document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/stream";
      };
    </script>
  </body>
</html>
)rawliteral";

//******************END OF WEBPAGE CODE **************************



void setGPIO(){
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(SERVO1_PIN, PWM_CHANNEL);
  setServo1Angle(90);

  ledcSetup(PWM_CHANNEL2, PWM_FREQ, PWM_RES);
  ledcAttachPin(SERVO2_PIN, PWM_CHANNEL2);
  setServo2Angle(90);

  pinMode(FLASHLIGHT_PIN, OUTPUT);
  digitalWrite(FLASHLIGHT_PIN, LOW);

  pinMode(MOTOR_1_PIN_1, OUTPUT);
  pinMode(MOTOR_1_PIN_2, OUTPUT);
  pinMode(MOTOR_2_PIN_1, OUTPUT);
  pinMode(MOTOR_2_PIN_2, OUTPUT);
}


static esp_err_t index_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}


static esp_err_t stream_handler(httpd_req_t * req){
  camera_fb_t* fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t* _jpg_buf = NULL;
  char* part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

 while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      //Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            //Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
  }
  return res;
}


static esp_err_t cmd_handler(httpd_req_t *req){
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};
  
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if(!buf){
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "go", variable, sizeof(variable)) == ESP_OK) {
      } else {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  sensor_t * s = esp_camera_sensor_get();
  int res = 0;
  


  //Motor Controls
  if(!strcmp(variable, "forward")) {
    //Serial.println("Forward");
    digitalWrite(MOTOR_1_PIN_1, 1);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 1);
    digitalWrite(MOTOR_2_PIN_2, 0);
  }
  else if(!strcmp(variable, "left")) {
    //Serial.println("Left");
    digitalWrite(MOTOR_1_PIN_1, 0);
    digitalWrite(MOTOR_1_PIN_2, 1);
    digitalWrite(MOTOR_2_PIN_1, 1);
    digitalWrite(MOTOR_2_PIN_2, 0);
  }
  else if(!strcmp(variable, "right")) {
    //Serial.println("Right");
    digitalWrite(MOTOR_1_PIN_1, 1);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 0);
    digitalWrite(MOTOR_2_PIN_2, 1);
  }
  else if(!strcmp(variable, "backward")) {
    //Serial.println("Backward");
    digitalWrite(MOTOR_1_PIN_1, 0);
    digitalWrite(MOTOR_1_PIN_2, 1);
    digitalWrite(MOTOR_2_PIN_1, 0);
    digitalWrite(MOTOR_2_PIN_2, 1);
  }else if(!strcmp(variable, "stop")) {
    //Serial.println("Stop");
    digitalWrite(MOTOR_1_PIN_1, 0);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 0);
    digitalWrite(MOTOR_2_PIN_2, 0);
 }else if (!strcmp(variable, "sdown")){
  if(s1_pos <= 170) {
    s1_pos += 10;
    setServo1Angle(s1_pos);
    delay(200);
  }
}else if (!strcmp(variable, "sup")){
  if(s1_pos >= 10) {
    s1_pos -= 10;
    setServo1Angle(s1_pos);
    delay(200);
  }
}else if (!strcmp(variable, "sright")) {
  if(s2_pos >= 10) {
    s2_pos -= 10;
    setServo2Angle(s2_pos);
    delay(200);
  }
} else if (!strcmp(variable, "sleft")) {
  if(s2_pos <= 170) {
    s2_pos += 10;
    setServo2Angle(s2_pos);
    delay(200);
  }
}else if (!strcmp(variable, "flash")) {
  flashlight_on = !flashlight_on;
  digitalWrite(FLASHLIGHT_PIN, flashlight_on ? HIGH : LOW);
}else {
    res = -1;
  }

  if(res){
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}


void startServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t cmd_uri = {
    .uri       = "/action",
    .method    = HTTP_GET,
    .handler   = cmd_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
  }
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }

}


void loop(){
  //nop
}


void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.begin(115200);

  //configure camera settings
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 

  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }


  // init camera

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    //Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1);    // Flip vertically
  s->set_hmirror(s, 1);  // (Optional) horizontal mirror

  // CONNECT TO WIFI
  WiFi.begin(NETWORK_NAME, NETWORK_PWD);
  //repeatedly try to connect until success
  //Serial.print("Connecting to network...");
  while (WiFi.status() != WL_CONNECTED){
    delay(500); // re-attempt connection every 500 ms
    //Serial.print(".");
  }

  if (!MDNS.begin("myesp32")) {
    Serial.println("Error starting mDNS");
    return;
  }

  //Serial.println("\nConnection Sucessful!");
  //Serial.print("Use your browser to open:  http://");
  //Serial.println(WiFi.localIP());


  // HARDWARE SETUP/INIT HERE
  setGPIO();

  //setup done, we can allow for connections now
  startServer();

}

// EOF