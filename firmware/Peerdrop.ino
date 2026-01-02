#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>

#define SCAN_TIME 5
#define LED_PIN 2

// ---------------- BADGE ID (FROM MAC) ----------------
uint32_t BADGE_ID = 0;

// ---------------- BLE GLOBALS ----------------
BLEAdvertising* pAdvertising = nullptr;
BLEScan* pScan = nullptr;
BLEServer* pServer = nullptr;

BLECharacteristic* privateChar = nullptr;
BLECharacteristic* requestChar = nullptr;

// ---------------- NVS ----------------
Preferences prefs;      // exchanged contacts
Preferences myIdPrefs;  // my identity

// ---------------- WIFI ----------------
WebServer server(80);
bool isConfigured = false;

// ---------------- MY CONTACT DATA ----------------
String contactName;
String contactEmail;
String contactPhone;
String contactOrg;

// ---------------- DISTANCE ----------------
float A = -59;
float n = 2.0;
float DIST_THRESHOLD = 0.8;

// ---------------- EXCHANGE REGISTRY ----------------
#define MAX_EXCHANGED 20
String exchangedMACs[MAX_EXCHANGED];
int exchangedCount = 0;

bool exchangeConfirmed = false;

// ---------------- GENERATE BADGE ID ----------------
void generateBadgeID() {
    uint64_t mac = ESP.getEfuseMac();       // unique hardware MAC
    BADGE_ID = (uint32_t)(mac & 0xFFFFFF);  // last 3 bytes
}

// ---------------- DISTANCE FUNC ----------------
float rssiToDist(int rssi) {
    float exp = (A - rssi) / (10 * n);
    return pow(10, exp);
}

// ---------------- LOAD MY IDENTITY ----------------
void loadMyIdentity() {
    myIdPrefs.begin("my_id", true);
    isConfigured = myIdPrefs.getBool("configured", false);

    if (isConfigured) {
        contactName  = myIdPrefs.getString("name");
        contactEmail = myIdPrefs.getString("email");
        contactPhone = myIdPrefs.getString("phone");
        contactOrg   = myIdPrefs.getString("org");
    }
    myIdPrefs.end();
}

// ---------------- SETUP WIFI AP ----------------
void startSetupAP() {
    String ssid = "ID:" + String(BADGE_ID, HEX);
    WiFi.softAP(ssid.c_str());

    Serial.println("\n[SETUP MODE]");
    Serial.println("WiFi AP : " + ssid);
    Serial.println("Open    : http://192.168.4.1");

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html",
            "<h2>Contact Badge Setup</h2>"
            "<form action='/save'>"
            "Name:<br><input name='name'><br>"
            "Email:<br><input name='email'><br>"
            "Phone:<br><input name='phone'><br>"
            "Organisation:<br><input name='org'><br><br>"
            "<input type='submit' value='Save'>"
            "</form>"
        );
    });

    server.on("/save", HTTP_GET, []() {
        myIdPrefs.begin("my_id", false);
        myIdPrefs.putString("name", server.arg("name"));
        myIdPrefs.putString("email", server.arg("email"));
        myIdPrefs.putString("phone", server.arg("phone"));
        myIdPrefs.putString("org", server.arg("org"));
        myIdPrefs.putBool("configured", true);
        myIdPrefs.end();

        server.send(200, "text/html",
            "<h3>Saved! Restarting badge...</h3>"
        );

        delay(1500);
        ESP.restart();
    });

    server.begin();
}

// ---------------- EXCHANGE HELPERS ----------------
bool isAlreadyExchanged(String mac) {
    for (int i = 0; i < exchangedCount; i++) {
        if (exchangedMACs[i] == mac) return true;
    }
    return false;
}

void saveExchangeToNVS(String mac) {
    if (exchangedCount >= MAX_EXCHANGED) return;
    exchangedMACs[exchangedCount] = mac;
    prefs.putInt("count", exchangedCount + 1);
    prefs.putString(("mac" + String(exchangedCount)).c_str(), mac);
    exchangedCount++;
}

void loadExchangesFromNVS() {
    exchangedCount = prefs.getInt("count", 0);
    if (exchangedCount > MAX_EXCHANGED) exchangedCount = MAX_EXCHANGED;

    for (int i = 0; i < exchangedCount; i++) {
        exchangedMACs[i] = prefs.getString(("mac" + String(i)).c_str(), "");
    }
}

// ---------------- SAVE CONTACT ----------------
void saveContact(String mac, int id, String data) {
    Serial.println("\n[✔] Contact Saved");
    Serial.println("ID   : " + String(id, HEX));
    Serial.println("MAC  : " + mac);
    Serial.println("DATA : " + data);
}

// ---------------- BLE CALLBACKS ----------------
class PrivateCallback : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* c) {
        String payload =
            contactName + "," +
            contactEmail + "," +
            contactPhone + "," +
            contactOrg;
        c->setValue(payload.c_str());
    }
};

class RequestCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        String v = String(c->getValue().c_str());

        if (v == "1") {
            String payload =
                contactName + "," +
                contactEmail + "," +
                contactPhone + "," +
                contactOrg;
            privateChar->setValue(payload.c_str());
            c->setValue("0");
        }
        else if (v == "ACK") {
            c->setValue("OK");
            exchangeConfirmed = true;
        }
    }
};

// ---------------- ADVERTISE ----------------
void advertisePokemon() {
    char buf[20];
    snprintf(buf, sizeof(buf), "ID:%06X", BADGE_ID);

    BLEAdvertisementData adv;
    adv.setName("CONTACT_BADGE");
    adv.setManufacturerData(buf);

    pAdvertising->setAdvertisementData(adv);
    pAdvertising->start();
}

// ---------------- CLIENT EXCHANGE ----------------
bool sendRequestAndReceive(BLEAdvertisedDevice dev, int otherID) {
    BLEClient* client = BLEDevice::createClient();
    exchangeConfirmed = false;

    if (!client->connect(&dev)) return false;

    BLERemoteService* svc = client->getService("1234");
    if (!svc) return false;

    auto* req = svc->getCharacteristic("c001");
    auto* pc  = svc->getCharacteristic("abcd");

    req->writeValue("1");
    delay(300);

    String raw = pc->readValue().c_str();
    if (raw.length() == 0) return false;

    saveContact(dev.getAddress().toString().c_str(), otherID, raw);

    req->writeValue("ACK");
    delay(300);

    client->disconnect();
    return true;
}

// ---------------- SCAN LOOP ----------------
void scanForPokemon() {
    BLEScanResults* results = pScan->start(SCAN_TIME, false);
    if (!results) return;

    for (int i = 0; i < results->getCount(); i++) {
        BLEAdvertisedDevice dev = results->getDevice(i);
        String manu = dev.getManufacturerData().c_str();
        if (!manu.startsWith("ID:")) continue;

        String mac = dev.getAddress().toString();
        if (isAlreadyExchanged(mac)) continue;

        float dist = rssiToDist(dev.getRSSI());
        if (dist > DIST_THRESHOLD) continue;

        int otherID = strtol(manu.substring(3).c_str(), nullptr, 16);

        Serial.println("\n[*] Device Found");
        Serial.println("ID  : " + String(otherID, HEX));
        Serial.println("MAC : " + mac);
        Serial.println("Press 0 to exchange");

        digitalWrite(LED_PIN, HIGH);
        unsigned long t0 = millis();
        bool pressed = false;

        while (millis() - t0 < 4000) {
            if (Serial.available() && Serial.read() == '0') {
                pressed = true;
                break;
            }
        }
        digitalWrite(LED_PIN, LOW);

        if (!pressed) continue;

        if (sendRequestAndReceive(dev, otherID)) {
            saveExchangeToNVS(mac);
            Serial.println("[✔] Exchange done");
        }
    }
    pScan->clearResults();
}

// ---------------- CLEAR NVS ----------------
void clearAllContacts() {
    Serial.println("\n[RESET] Clearing ALL NVS");

    prefs.clear();
    myIdPrefs.begin("my_id", false);
    myIdPrefs.clear();
    myIdPrefs.end();

    exchangedCount = 0;

    delay(1000);
    ESP.restart();
}

// ---------------- SETUP ----------------
void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    generateBadgeID();
    Serial.printf("[BADGE ID] %06X\n", BADGE_ID);

    prefs.begin("contacts", false);
    loadExchangesFromNVS();
    loadMyIdentity();

    if (!isConfigured) {
        startSetupAP();
        return;
    }

    BLEDevice::init("CONTACT_BADGE");

    pServer = BLEDevice::createServer();
    BLEService* svc = pServer->createService("1234");

    privateChar = svc->createCharacteristic("abcd", BLECharacteristic::PROPERTY_READ);
    privateChar->setCallbacks(new PrivateCallback());

    requestChar = svc->createCharacteristic(
        "c001",
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    requestChar->setCallbacks(new RequestCallback());

    svc->start();

    pAdvertising = pServer->getAdvertising();
    advertisePokemon();

    pScan = BLEDevice::getScan();
    pScan->setActiveScan(true);

    Serial.println("[READY] BLE badge active");
}

// ---------------- LOOP ----------------
void loop() {

    // Setup portal
    if (!isConfigured) {
        server.handleClient();
        return;
    }

    // Clear NVS
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'C' || cmd == 'c') {
            clearAllContacts();
        }
    }

    pAdvertising->start();
    scanForPokemon();
    delay(500);
}

