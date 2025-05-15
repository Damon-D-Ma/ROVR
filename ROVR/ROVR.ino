// attributions to: Rui Santos with tutorial https://randomnerdtutorials.com/esp32-cam-car-robot-web-server/ for starter code


#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"             // disable brownout problems
#include "soc/rtc_cntl_reg.h"    // disable brownout problems
#include "esp_http_server.h"

//servos to control cams
#define SERVO1_PIN 14
#define SERVO2_PIN 15

// control speed of the wheels via PWM
#define SPEED_PIN 2


#define NETWORK_NAME "Mlink-007"
#define NETWORK_PWD "SP9239O!"


#define TL_MOTOR    14
#define BL_MOTOR    15
#define TR_MOTOR    13
#define BR_MOTOR    12

httpd_handle_t camera_httpd = NULL;


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



// speed of the motor
int motor_speed = 1000; // TODO: change this initial value???
int speed_resolution = 8; //TODO: need to change this too???


//Servo left_right_servo;
//Servo up_down_servo;



//****************** WEBPAGE CODE BELOW **************************

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<html>
  <head>
    <title>ESP32-CAM Robot</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
    <h1>YOUR WEBSERVER WORKS CONGRATS!!!</h1>
  </body>
</html>
)rawliteral";
//******************END OF WEBPAGE CODE **************************



void setGPIO(){
  // nop for now, need to finalize circuit before filling out
  return;
  /*
  //for the camera-control servos
  left_right_servo.attach(SERVO1_PIN);
  up_down_servo.attach(SERVO2_PIN);

  //set up the PWM for the wheels
  ledcsetup(SPEED_PIN, motor_speed, speed_resolution);

  // TODO: Finish  this function, leave unimplemented for now until 
  // I get camera and webpage setup....
  */

}



void loop(){
  //nop
}


void setup(){

  Serial.begin(115200);

  // HARDWARE SETUP/INIT HERE
  setGPIO();
  //more to come...


  // CONNECT TO WIFI
  WiFi.begin(NETWORK_NAME, NETWORK_PWD);
  //repeatedly try to connect until success
  Serial.print("Connecting to network...");
  while (WiFi.status() != WL_CONNECTED){
    delay(500); // re-attempt connection every 500 ms
    Serial.print(".");
  }
  Serial.println("\nConnection Sucessful!");
  Serial.print("Use your browser to open:  http://");
  Serial.println(WiFi.localIP());

  //setup done, we can allow for connections now
  startServer();

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

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
  }


}






static esp_err_t index_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}









