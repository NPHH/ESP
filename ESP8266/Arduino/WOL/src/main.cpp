#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <ESP8266Ping.h>

/******************************************************************************************************************************
    Khu Vuc Can Dieu Chinh Thong So
******************************************************************************************************************************/
const char* WIFI_SSID = "MissuBee";                                 // Ten Dang Nhap Wifi

const char* WIFI_PASS = "12344321";                                 // Pass Wifi

/******************************************************************************************************************************
    Trang WEB Duoc Tao Ra Tu APP Script Khi Trien Khai Du An
******************************************************************************************************************************/
const char* WEBAPP_URL = "https://script.google.com/macros/s/AKfycbxEUiLffpZxOXKhnkSyvNv5Wzn0jWUtmCDqp3iZ4Jmusva9ebmWczSL-UN0wHuO_qLJ/exec";

/******************************************************************************************************************************
    Tu Dat - Gia Tri Phai Giong Voi File Code.gs - WEB_PIN Trong Code.gs La Mat Khau De Bat NAS Trong WEB
******************************************************************************************************************************/
const char* DEVICE_KEY = "ESP8266WOL920PLUS_A7F93C21";

/******************************************************************************************************************************
    Thong So NAS
******************************************************************************************************************************/
const uint8_t NAS_MAC[6] =
{
    0x10, 0x90, 0x28, 0xEA, 0x2F, 0xDC
};

const IPAddress NAS_IP(192, 168, 1, 200);                           // Phai trung IP hien thi tren web

const IPAddress BROADCAST_IP(192, 168, 1, 255);

const uint16_t WOL_PORT = 9;

/******************************************************************************************************************************
    Thoi Gian
******************************************************************************************************************************/
#define WOL_WAIT_TIME_TO_PING                         120000        // Thoi gian NAS Khoi Dong Cho Den Khi Ping Duoc (ms)

#define WOL_TIME_EVERY_PING                           3000          // Thoi gian Giua Cac Lan Ping (ms)

#define WOL_NUMBER_PING                               10            // Tong So Lan Ping

#define ACK_RETRY_COUNT                               3             // So lan gui lai khi ack loi

#define ACK_RETRY_DELAY                               1000          // Thoi gian giua cac lan gui lai (ms)

/******************************************************************************************************************************
    Trang Thai
******************************************************************************************************************************/
enum WOL_STATE_t : uint8_t
{
    WOL_STATE_IDLE                                    = 0x00,

    WOL_STATE_WAIT_BOOT                               = 0x01,

    WOL_STATE_PINGING                                 = 0x02,
};

/******************************************************************************************************************************
    Bien Toan Cuc
******************************************************************************************************************************/
int ledState = LOW;

unsigned long previousMillis = 0;

const long interval = 300;

WiFiUDP udp;

unsigned long lastPoll = 0;

const unsigned long POLL_INTERVAL = 5000;

WiFiClientSecure tlsClient;

WOL_STATE_t wolState = WOL_STATE_IDLE;

unsigned long wolStateTimer = 0;

uint8_t pingRetryCount = 0;

/******************************************************************************************************************************
    Phat Magic Packet
******************************************************************************************************************************/
bool sendWOL()
{
    uint8_t packet[102];

    memset(packet, 0xFF, 6);

    for (int i = 0; i < 16; i++)
    {
        memcpy(&packet[6 + i * 6], NAS_MAC, 6);
    }

    if (!udp.beginPacket(BROADCAST_IP, WOL_PORT))
    {
        Serial.println("[WOL] Khong tao duoc UDP packet");

        return false;
    }

    udp.write(packet, sizeof(packet));

    if (udp.endPacket() != 1)
    {
        Serial.println("[WOL] Gui Magic Packet that bai");

        return false;
    }

    Serial.println("[WOL] -> Da phat Magic Packet thanh cong!");

    return true;
}

/******************************************************************************************************************************
    Lay Du Lieu Tu WEB
******************************************************************************************************************************/
bool getWebResponse(const String& action, String& body)
{
    String url = String(WEBAPP_URL) + "?action=" + action + "&key=" + DEVICE_KEY;

    HTTPClient http;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    http.setTimeout(12000);

    if (!http.begin(tlsClient, url))
    {
        Serial.println("[POLL] Loi: khong khoi tao duoc ket noi HTTP");

        return false;
    }

    int code = http.GET();

    if (code == HTTP_CODE_OK)
    {
        body = http.getString();

        http.end();

        return true;
    }

    Serial.printf("[POLL] Loi HTTP: %d (%s)\n", code, HTTPClient::errorToString(code).c_str());

    http.end();

    return false;
}

/******************************************************************************************************************************
    Gui Trang Thai Len WEB (waitTimeSec > 0 thi gui kem WaitTime, chi dung cho BOOTING)
******************************************************************************************************************************/
bool sendAckStatus(const String& status, uint32_t waitTimeSec = 0)
{
    String url = String(WEBAPP_URL) + "?action=ack&key=" + DEVICE_KEY + "&status=" + status;

    if (waitTimeSec > 0)
    {
        url += "&waitTime=" + String(waitTimeSec);
    }

    Serial.printf("[ACK] Gui trang thai: %s (WaitTime: %u s)\n", status.c_str(), (unsigned int)waitTimeSec);

    HTTPClient http;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    http.setTimeout(12000);

    if (!http.begin(tlsClient, url))
    {
        return false;
    }

    int code = http.GET();

    http.end();

    return (code == HTTP_CODE_OK);
}

/******************************************************************************************************************************
    Gui Trang Thai Len WEB, Gui Lai Neu Loi
******************************************************************************************************************************/
bool sendAckWithRetry(const String& status, uint32_t waitTimeSec = 0)
{
    for (uint8_t i = 0; i < ACK_RETRY_COUNT; i++)
    {
        if (sendAckStatus(status, waitTimeSec))
        {
            return true;
        }

        Serial.printf("[ACK] Gui that bai (lan %u/%u), thu lai...\n", i + 1, ACK_RETRY_COUNT);

        delay(ACK_RETRY_DELAY);
    }

    Serial.println("[ACK] Khong gui duoc len Web!");

    return false;
}

/******************************************************************************************************************************
    Hoi Lenh Tu WEB
******************************************************************************************************************************/
void pollCommand()
{
    static const char* const STATE_NAME[] = { "IDLE", "WAIT_BOOT", "PINGING" };

    // Dang xu ly WOL (cho boot / ping) thi khong poll
    if (wolState != WOL_STATE_IDLE) return;

    String body;

    if (!getWebResponse("poll", body))
    {
        Serial.println("[POLL] That bai - khong nhan duoc phan hoi tu Web");

        return;
    }

    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, body);

    if (err)
    {
        Serial.printf("[POLL] Loi doc JSON: %s | Du lieu: %s\n", err.c_str(), body.c_str());

        return;
    }

    if (!doc["ok"].as<bool>())
    {
        const char* error = doc["error"] | "Khong ro";

        Serial.printf("[POLL] Web tu choi: %s\n", error);

        return;
    }

    const char* command = doc["command"] | "NONE";

    Serial.printf("[POLL] ESP: %s | Lenh tu Web: %s\n", STATE_NAME[wolState], command);

    if (strcmp(command, "WOL") == 0)
    {
        Serial.println("\n[LENH] Nhan duoc lenh WOL tu Web!");

        if (sendWOL())
        {
            wolState = WOL_STATE_WAIT_BOOT;

            // Gui WaitTime (giay) len web de web dem nguoc
            sendAckWithRetry("BOOTING", WOL_WAIT_TIME_TO_PING / 1000);

            // Bat dau dem WaitTime SAU KHI ack xong de khop voi dong ho tren web
            wolStateTimer = millis();

            Serial.printf("[BOOT] Bat dau dem WaitTime: %u giay\n", (unsigned int)(WOL_WAIT_TIME_TO_PING / 1000));
        }
    }
}

/******************************************************************************************************************************
    Xu Ly Qua Trinh Cho Boot Va Ping
******************************************************************************************************************************/
void processWolProcess()
{
    if (wolState == WOL_STATE_WAIT_BOOT)
    {
        unsigned long elapsed = millis() - wolStateTimer;

        if (elapsed < WOL_WAIT_TIME_TO_PING)
        {
            static unsigned long lastBootDebug = 0;

            if (millis() - lastBootDebug >= 1000)
            {
                lastBootDebug = millis();

                uint32_t remainingSec = (WOL_WAIT_TIME_TO_PING - elapsed) / 1000;

                Serial.printf("[BOOT] Dang cho NAS khoi dong... Con lai: %u giay\n", (unsigned int)remainingSec);
            }
        }
        else
        {
            Serial.println("[BOOT] Het thoi gian cho boot. Chuyen sang Ping kiem tra...");

            wolState = WOL_STATE_PINGING;

            // Tru di chu ky de lan ping dau tien chay ngay
            wolStateTimer = millis() - WOL_TIME_EVERY_PING;

            pingRetryCount = 0;
        }
    }
    else if (wolState == WOL_STATE_PINGING)
    {
        if (millis() - wolStateTimer >= WOL_TIME_EVERY_PING)
        {
            wolStateTimer = millis();

            Serial.printf("[PING] Dang Ping NAS (Lan %d/%d)...\n", pingRetryCount + 1, WOL_NUMBER_PING);

            if (Ping.ping(NAS_IP, 1))
            {
                Serial.println("[KET QUA] -> SUCCESS: NAS da Online (Ping OK)!");

                sendAckWithRetry("ONLINE");

                wolState = WOL_STATE_IDLE;
            }
            else
            {
                pingRetryCount++;

                if (pingRetryCount >= WOL_NUMBER_PING)
                {
                    Serial.println("[KET QUA] -> FAILED: Het luot Ping, NAS khong phan hoi!");

                    sendAckWithRetry("FAILED");

                    wolState = WOL_STATE_IDLE;
                }
            }
        }
    }
}

/******************************************************************************************************************************
    Ket Noi WiFi
******************************************************************************************************************************/
void connectWiFi()
{
    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Ket noi WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);

        Serial.print(".");
    }

    Serial.println();

    Serial.print("IP ESP8266: ");

    Serial.println(WiFi.localIP());
}

/******************************************************************************************************************************
    Setup
******************************************************************************************************************************/
void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(9600);

    Serial.println("\n--- ESP8266 WOL READY ---");

    connectWiFi();

    udp.begin(2390);

    tlsClient.setInsecure();
}

/******************************************************************************************************************************
    Loop
******************************************************************************************************************************/
void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.reconnect();

        delay(1000);

        return;
    }

    // Chi poll khi dang IDLE; trong luc WAIT_BOOT / PINGING chi dem thoi gian
    if (wolState == WOL_STATE_IDLE && millis() - lastPoll >= POLL_INTERVAL)
    {
        lastPoll = millis();

        pollCommand();
    }

    processWolProcess();

    delay(10);

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        ledState = (ledState == LOW) ? HIGH : LOW;

        digitalWrite(LED_BUILTIN, ledState);
    }
}