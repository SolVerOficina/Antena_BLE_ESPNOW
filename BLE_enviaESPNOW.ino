#include <BLEDevice.h> 
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>

#define SERVICE_UUID           "ffe0" // UART service UUID a5f81d42-f76e-11ea-adc1-0242ac120002
#define CHARACTERISTIC_UUID_RX "ffe1"
#define CHARACTERISTIC_UUID_TX "ffe2"
#define rele 33
//58:BF:25:99:AC:98 40:91:51:9B:5B:A8 - 58:BF:25:99:AC:98
// PA Oficina 7C:9E:BD:F8:88:88 - 0x7C, 0x9E, 0xBD, 0xF8, 0x88, 0x88
// PV Oficina 7C:9E:BD:FA:17:B0 - 0x7C, 0x9E, 0xBD, 0xFA, 0x17, 0xB0

uint8_t broadcastAddress1[] = {0x7C, 0x9E, 0xBD, 0xF8, 0x88, 0x88};
uint8_t broadcastAddress2[] = {0x7C, 0x9E, 0xBD, 0xFA, 0x17, 0xB0};

Preferences preferences;

 esp_now_peer_info_t peerInfo;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  String senal;
} struct_message;

// Create a struct_message called myData
struct_message myData;
struct_message myData2;


String spulso;
int cont = 0;
int reseteo = 60;

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("***");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
          spulso[i] = rxValue[i];
        }

        Serial.println();

 if (rxValue.find("X") != -1) {
  preferences.begin("Conteo",false);
  preferences.clear();
  preferences.end();
  sent_PA();
  ESP.restart();
  }
  if (rxValue.find("Y") != -1) {
  preferences.begin("Conteo",false);
  preferences.clear();
  preferences.end();
  sent_PV();
  ESP.restart();
  }

      } 
  }
};



void setup() {
   Serial.begin(115200);
   pinMode(rele, OUTPUT);
   WiFi.mode(WIFI_STA);
  preferences.begin("Conteo", false);
    Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
 
   
   // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
    // register first peer  
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
    // register second peer  
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

   
 // Create the BLE Device
  BLEDevice::init("FOOTCALL SV"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

}

void loop() {

cont++;
delay(1000);
if (cont >= reseteo){
  Serial.println("<<<<<<<<<< REINICIANDO SOFTWARE >>>>>>>>>>");
ESP.restart();
}

}

void sent_PA(){
  myData.senal = spulso;
  Serial.print("Mensaje: ");
  Serial.println(myData.senal);
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(2000);
}

void sent_PV(){
  myData2.senal = spulso;
  Serial.print("Mensaje: ");
  Serial.println(myData2.senal);
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress2, (uint8_t *) &myData2, sizeof(myData2));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(2000);
}