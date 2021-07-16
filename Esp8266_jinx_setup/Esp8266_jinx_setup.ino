#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define WSout 5  // Pin D1
#define WSbit (1<<WSout)

// ARTNET CODES
#define ARTNET_DATA 0x50
#define ARTNET_POLL 0x20
#define ARTNET_POLL_REPLY 0x21
#define ARTNET_PORT 6454
#define ARTNET_HEADER 17

WiFiUDP udp;

uint8_t uniData[514];
uint16_t uniSize;
uint8_t hData[ARTNET_HEADER + 1];
uint8_t net = 0;
uint8_t universe = 0;
uint8_t subnet = 0;

const char* ssid     = "Playmedia";
const char* password = "12345678";

IPAddress local_ip(192, 168, 1, 200);
IPAddress gateway_ip(192, 168, 1, 1);
IPAddress subnet_ip(255, 255, 255, 0);

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.config(local_ip, gateway_ip, subnet_ip);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  udp.begin(ARTNET_PORT); // Open ArtNet port

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(WSout, OUTPUT);
}



void ICACHE_FLASH_ATTR sendWS() {
  uint32_t writeBits;
  uint8_t  bitMask, time;
  os_intr_lock();
  for (uint16_t t = 0; t < uniSize; t++) { // outer loop counting bytes
    bitMask = 0x80;
    while (bitMask) {
      // time=0ns : start by setting bit on
      time = 4;
      while (time--) {
        WRITE_PERI_REG( 0x60000304, WSbit );  // do ON bits // T=0
      }
      if ( uniData[t] & bitMask ) {
        writeBits = 0;  // if this is a '1' keep the on time on for longer, so dont write an off bit
      }
      else {
        writeBits = WSbit;  // else it must be a zero, so write the off bit !
      }
      time = 4;
      while (time--) {
        WRITE_PERI_REG( 0x60000308, writeBits );  // do OFF bits // T='0' time 350ns
      }
      time = 6;
      while (time--) {
        WRITE_PERI_REG( 0x60000308, WSbit );  // switch all bits off  T='1' time 700ns
      }
      // end of bite write time=1250ns
      bitMask >>= 1;
    }
  }
  os_intr_unlock();
}

void loop() {
  if (udp.parsePacket()) {
    udp.read(hData, ARTNET_HEADER + 1);
    if ( hData[0] == 'A' && hData[1] == 'r' && hData[2] == 't' && hData[3] == '-' && hData[4] == 'N' && hData[5] == 'e' && hData[6] == 't') {
      if ( hData[8] == 0x00 && hData[9] == ARTNET_DATA && hData[15] == net ) {
        if ( hData[14] == (subnet << 4) + universe ) { // UNIVERSE
          uniSize = (hData[16] << 8) + (hData[17]);
          udp.read(uniData, uniSize);
          //Serial.print("ArtNet packet RX Uni 0 - size:");
          //Serial.println(uniSize);
          sendWS();
        }
      } // if Artnet Data
    }
  }
}
