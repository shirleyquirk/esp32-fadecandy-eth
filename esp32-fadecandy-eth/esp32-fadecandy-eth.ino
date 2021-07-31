#define FASTLED_ESP32_I2S true
//#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#define PIN1 25
#define N_PIXELS 64
#define Server EthernetServer
//#define Client EthernetClient
#include <EthernetENC.h>
Server opc_server(7890);
CRGB leds[N_PIXELS];


const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


#include <EthernetUdp.h>
#include <EthernetENC.h>
#define LOG_DEST "192.168.0.10"
#define LOG_PORT 5556
//Listen to logging with nc -kluw 1 LOG_PORT
#ifdef LOG_DEST
  #define __log(NAME,...) do{\
    udp.beginPacket(LOG_DEST,LOG_PORT);\
    udp.NAME(__VA_ARGS__);\
    udp.endPacket();\
    }while(0)
#else
  #define __log(NAME,...) do{\
    udp.beginPacket(broadcast_ip,LOG_PORT);\
    udp.NAME(__VA_ARGS__);\
    udp.endPacket();\
    }while(0)
#endif/*LOG_DEST*/

#define log_printf(...) __log(printf,__VA_ARGS__)
#define log_println(...) __log(println,__VA_ARGS__)
#define log_print(...) __log(print,__VA_ARGS__)

#define UDP EthernetUDP
UDP udp;

void setup() {
  // put your setup code here, to run once:
  uint8_t mac[6] = {0,1,2,3,4,5};
  Ethernet.begin(mac,IPAddress(192,168,0,6));
  opc_server.begin();
  LEDS.addLeds<WS2812,PIN1,GRB>(leds,0,N_PIXELS);
  LEDS.setBrightness(128);
}

void loop() {
  // put your main code here, to run repeatedly:
  EthernetClient client = opc_server.available();
  if (client){
    log_println("New OPC Client Connected");
    while (client.connected()){
        int frms=0;
        while(client.available()){
          readFrame(client,leds);
          frms++;
        }
        if (frms==1){
          FastLED.show();
        }else if(frms>1){
          log_printf("received %d frames\n",frms);
        }
        //FastLED.delay(5);
        yield();
    }
    client.stop();
    log_println("Client Stopped");
  }
  FastLED.delay(20);
  yield();
}
int blockingRead (Client &client) {
  int b;
  while ((b = client.read()) < 0) {
    yield();//vTaskDelay(1);?
    if (! client.connected())
      return -1;
  }
  return b;
}
int readFrame(EthernetClient client,CRGB *buf){
  uint8_t buf4[4];
  uint8_t cmd;
  size_t payload_length, leds_in_payload, i;
  client.read(buf4,sizeof(buf4));
  cmd = buf4[1];
  payload_length = (((size_t) buf4[2]) << 8) + (size_t)buf4[3];
  leds_in_payload = payload_length / 3;
  if (leds_in_payload > N_PIXELS) {
    log_printf("expected %d, received %d\n",N_PIXELS,leds_in_payload);
    leds_in_payload = N_PIXELS;
  }
  if (cmd != 0) leds_in_payload = 0;
  for (int i=0;i<leds_in_payload;i++){
    if (client.read(buf4,3)<0){
      return -3;
    }else{
      leds[i] = CRGB(gamma8[buf4[0]],gamma8[buf4[1]],gamma8[buf4[2]]);
    }
  }
  
  //flush remaining leds
  
  
  //FastLED.show();
  return 0;
}
