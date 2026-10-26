










                         // Phai trung IP hien thi tren web







/******************************************************************************************************************************
    Bien Toan Cuc
******************************************************************************************************************************/








const unsigned long POLL_INTERVAL = ;




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

    if(!udp.beginPacket(BROADCAST_IP, WOL_PORT))
    {
        Serial.println("[WOL] Khong tao duoc UDP packet");

        return false;
    }

    udp.write(packet, sizeof(packet));

    if(udp.endPacket() != 1)
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


/******************************************************************************************************************************
    Gui Trang Thai Len WEB (waitTimeSec > 0 thi gui kem WaitTime, chi dung cho BOOTING)
******************************************************************************************************************************/
bool sendAckStatus(const String& status, uint32_t waitTimeSec = 0)
{
    String url = String(WEBAPP_URL) + "?action=ack&key=" + DEVICE_KEY + "&status=" + status;

    if(waitTimeSec > 0)
    {
        url += "&waitTime=" + String(waitTimeSec);
    }

    Serial.printf("[ACK] Gui trang thai: %s (WaitTime: %u s)\n", status.c_str(), (unsigned int)waitTimeSec);

    HTTPClient http;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    http.setTimeout(12000);

    if(!http.begin(tlsClient, url))
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
        if(sendAckStatus(status, waitTimeSec))
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
    
}

/******************************************************************************************************************************
    Xu Ly Qua Trinh Cho Boot Va Ping
******************************************************************************************************************************/
void processWolProcess()
{
    if(wolState == WOL_STATE_WAIT_BOOT)
    {
        unsigned long elapsed = millis() - wolStateTimer;

        if(elapsed < WOL_WAIT_TIME_TO_PING)
        {
            static unsigned long lastBootDebug = 0;

            if(millis() - lastBootDebug >= 1000)
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
    else if(wolState == WOL_STATE_PINGING)
    {
        if(millis() - wolStateTimer >= WOL_TIME_EVERY_PING)
        {
            wolStateTimer = millis();

            Serial.printf("[PING] Dang Ping NAS (Lan %d/%d)...\n", pingRetryCount + 1, WOL_NUMBER_PING);

            if(Ping.ping(NAS_IP, 1))
            {
                Serial.println("[KET QUA] -> SUCCESS: NAS da Online (Ping OK)!");

                sendAckWithRetry("ONLINE");

                wolState = WOL_STATE_IDLE;
            }
            else
            {
                pingRetryCount++;

                if(pingRetryCount >= WOL_NUMBER_PING)
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
    Setup
******************************************************************************************************************************/
void setup()
{
    


    
}

/******************************************************************************************************************************
    
******************************************************************************************************************************/
void loop()
{
    

    // Chi poll khi dang IDLE; trong luc WAIT_BOOT / PINGING chi dem thoi gian
    

    processWolProcess();

    delay(10);

    
}