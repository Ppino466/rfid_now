
//Librerias
#include <SPI.h>
#include <MFRC522.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

//Definicion pines
#define SS_PIN 5
#define RST_PIN 4
#define led 2

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Instancia.

//MAC esp32 destino
uint8_t broadcastAddress[] = { 0x0C, 0xB8, 0x15, 0x75, 0x7A, 0x58 };

//SSID esp32 destino
constexpr char WIFI_SSID[] = "Tenda_DF630";

//Variable para enviar dato UID
String content = "";

//Estructura para enviar por ESP-NOW
typedef struct struct_message {
  char a[32];
  int b;
} struct_message;

// myData es la que se envia
struct_message myData;

//Definir informacion del par
esp_now_peer_info_t peerInfo;

//Metodo para obtener el canal de wifi
int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}
// callback cuando se envia datos
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


void setup() {
  //Iniciar comunicaion serial
  Serial.begin(115200);

  // Poner en modo estacion el WiFi
  WiFi.mode(WIFI_STA);

  //Obtener el canal y ponerlo en la configuraion WiFi de la esp32
  int32_t channel = getWiFiChannel(WIFI_SSID);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  //Inidicar el modo de los pines de los leds
  pinMode(led, OUTPUT);

  //Inicializamos ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  //Inicimos el envio
  esp_now_register_send_cb(OnDataSent);

  // Registramos el par
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Agremamos el par
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println();

  // Iniciar  SPI bus
  SPI.begin();

  // Iniciar MFRC522
  mfrc522.PCD_Init();

  //Mensajes
  Serial.println("Coloca tu tarjeta o llavero.");
}

void loop() {
  // En espera
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Lectura
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  //Encendemos el led indicador
  digitalWrite(led, HIGH);
  //Mostrar en monitor serial el UID
  Serial.println();
  Serial.print("UID  :");
  byte letter;

  //Ciclo para dar formato al UID
  for (byte i = 0; i < mfrc522.uid.size; i++) {

    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  delay(1000);
  Serial.println();
  //Apagar el led inidicador
  digitalWrite(led, LOW);

  // Colocamos los valores en el mensaje
  content.toUpperCase();
  content.toCharArray(myData.a, 32);
  myData.b = 1;
  // Mandamos el mensaje via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

  //Verificamos si el envio fue correcto
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }
  delay(2000);
  //Vaciamos la variable del UID
  content = "";
}
