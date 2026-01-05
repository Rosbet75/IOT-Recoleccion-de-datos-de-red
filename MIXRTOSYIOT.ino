#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <DHT.h>
#include <HTTPClient.h>


const char* ssid = "Test";     
const char* password = "12345678";  


const char* serverName = "http://10.203.84.232/iot_insert.php"; // Cambia IP de tu PC


#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int trigger = 2;
const int echo = 15;


const int touch4_pin = T4; // GPIO 13
const int touch5_pin = T5; // GPIO 12


#define CH_HOP_START 1
#define CH_HOP_END 13
#define HOP_TIME 150
#define QUEUE_LEN 64
#define SAMPLE_LEN 512

typedef struct {
  uint8_t frame_ctrl[2];
  uint8_t duration_id[2];
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  uint8_t seq_ctrl[2];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  int8_t rssi;
  uint8_t channel;
  uint8_t len;
  uint8_t payload[SAMPLE_LEN];
  uint8_t type;
  uint8_t subtype;
} pkt_item_t;

QueueHandle_t pktQueue = NULL;
unsigned long lastHop = 0;
int currentChannel = CH_HOP_START;
unsigned long lastSensor = 0;


String macToString(const uint8_t *mac) {
  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

bool isBroadcastMac(const uint8_t *mac) {
  for (int i = 0; i < 6; ++i) if (mac[i] != 0xFF) return false;
  return true;
}

void extractSSID(const uint8_t *body, int body_len, char *out, int out_len) {
  out[0] = '\0';
  int idx = 0;
  while (idx + 2 <= body_len) {
    uint8_t tag = body[idx];
    uint8_t tlen = body[idx + 1];
    if (idx + 2 + tlen > body_len) break;
    const uint8_t *data = body + idx + 2;
    if (tag == 0) {
      int copy = tlen < (out_len - 1) ? tlen : (out_len - 1);
      if (copy > 0) {
        memcpy(out, data, copy);
        out[copy] = '\0';
      } else {
        strncpy(out, "<hidden>", out_len);
        out[out_len-1] = '\0';
      }
      return;
    }
    idx += 2 + tlen;
  }
  out[0] = '\0';
}

// ------------------ FUNCION PARA ENVIAR DATOS CAMBIANDO MODOS ------------------
void sendDataSwitchMode(String postData) {
    // 1. Desactivar promiscuous
    esp_wifi_set_promiscuous(false);

    // 2. Conectar a WiFi STA
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
        delay(500);
        Serial.print(".");
        tries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConectado a WiFi para enviar datos");

        HTTPClient http;
        http.begin(serverName);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        int httpResponse = http.POST(postData);
        Serial.printf("POST -> %d\n", httpResponse);
        http.end();
    } else {
        Serial.println("\nNo se pudo conectar a WiFi, datos no enviados");
    }

    // 3. Desconectar STA
    WiFi.disconnect(true);

    // 4. Volver a modo promiscuo
    WiFi.mode(WIFI_MODE_STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
}

// ------------------ SNIFFER CALLBACK ------------------
void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type){
  wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t*) buf;
  wifi_pkt_rx_ctrl_t rx_ctrl = ppkt->rx_ctrl;

  int len = rx_ctrl.sig_len;
  if (len <= 0) return;
  const uint8_t *payload = ppkt->payload;
  if (len < 24) return;

  wifi_ieee80211_mac_hdr_t *hdr = (wifi_ieee80211_mac_hdr_t *) payload;
  const uint8_t *addr1 = hdr->addr1;
  const uint8_t *addr2 = hdr->addr2;
  const uint8_t *addr3 = hdr->addr3;

  uint8_t fc0 = payload[0];
  uint8_t type_field = (fc0 >> 2) & 0x03;
  uint8_t subtype = (fc0 >> 4) & 0x0F;

  pkt_item_t item;
  item.rssi = rx_ctrl.rssi;
  item.channel = rx_ctrl.channel;
  item.len = len;
  item.type = type_field;
  item.subtype = subtype;
  memcpy(item.payload, ppkt->payload, item.len);

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(pktQueue, &item, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

// ------------------ PRINTER Y ENVÍO ------------------
void printer(pkt_item_t* ppkt){
  int len = ppkt->len;
  if (len < 24) return;

  wifi_ieee80211_mac_hdr_t *hdr = (wifi_ieee80211_mac_hdr_t *) ppkt->payload;
  const uint8_t *addr1 = hdr->addr1;
  const uint8_t *addr2 = hdr->addr2;
  const uint8_t *addr3 = hdr->addr3;

  if (!isBroadcastMac(addr1)) return;

  int tipo = ppkt->type;
  int subtipo = ppkt->subtype;
  int rssi = ppkt->rssi;
  int channel = ppkt->channel;

  if (millis() - lastSensor >= 2000) { // cada 2s
    lastSensor = millis();

    // Sensores
    digitalWrite(trigger, LOW); delayMicroseconds(2);
    digitalWrite(trigger, HIGH); delayMicroseconds(10);
    digitalWrite(trigger, LOW);
    long duracion = pulseIn(echo, HIGH, 30000);
    float g_dist = duracion * 0.034 / 2;

    float g_temp = dht.readTemperature();
    float g_hum = dht.readHumidity();

    int t4 = touchRead(touch4_pin);
    int t5 = touchRead(touch5_pin);

    String mac_src = macToString(addr2);
    String mac_dst = macToString(addr1);
    String mac_bssid = macToString(addr3);

    Serial.printf("RSSI:%d CH:%d Type:%d Sub:%d TEMP:%.2f HUM:%.2f DIST:%.2f T4:%d T5:%d\n",
                  rssi, channel, tipo, subtipo, g_temp, g_hum, g_dist, t4, t5);

    String postData =
      "rssi=" + String(rssi) +
      "&channel=" + String(channel) +
      "&mac_src=" + mac_src +
      "&mac_dst=" + mac_dst +
      "&mac_bssid=" + mac_bssid +
      "&tipo=" + String(tipo) +
      "&subtipo=" + String(subtipo) +
      "&temp=" + String(g_temp) +
      "&hum=" + String(g_hum) +
      "&distancia=" + String(g_dist) +
      "&touch4=" + String(t4) +
      "&touch5=" + String(t5);

    sendDataSwitchMode(postData);
  }
}


void setup() {
  Serial.begin(115200);
  delay(1500);

  // Sensores
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
  dht.begin();

  // Sniffer
  pktQueue = xQueueCreate(QUEUE_LEN, sizeof(pkt_item_t));
  if(!pktQueue){
    Serial.println("ERROR: No se pudo crear la cola");
    while(1){ delay(1000); }
  }

  WiFi.mode(WIFI_MODE_STA);       // Modo base
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
  esp_wifi_set_channel(CH_HOP_START, WIFI_SECOND_CHAN_NONE);
}


void loop() {
  unsigned long now = millis();
  if (now - lastHop >= HOP_TIME) {
    currentChannel++;
    if (currentChannel > CH_HOP_END) currentChannel = CH_HOP_START;
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    lastHop = now;
  }

  pkt_item_t item;
  if (xQueueReceive(pktQueue, &item, 0) == pdTRUE){
    printer(&item);
  }

  delay(1000);
}
