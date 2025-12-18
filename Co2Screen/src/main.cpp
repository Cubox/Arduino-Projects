#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266HttpClient.h>
#include <Wire.h>
#include "secrets.h"

// including both doesn't use more code or ram
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include "GxEPD2_display_selection_new_style.h"
#include <Fonts/FreeMonoBold18pt7b.h>

const char *ssid = SSID;
const char *password = PASSWORD;

// `secrets.h` uses macros; undef them so they don't break method names like `WiFi.SSID()`.
#undef SSID
#undef PASSWORD

const char Text[] = "1111 PPM";

static const uint32_t WIFI_BOOT_SCREEN_AFTER_MS = 1500;
static const uint32_t WIFI_STUCK_SCREEN_AFTER_MS = 10000;
static const uint32_t WIFI_DIAG_AFTER_MS = 20000;
static const uint32_t FETCH_INTERVAL_MS = 1000UL * 60UL * 5UL;

static uint32_t bootStartMs;
static uint32_t wifiStartMs;
static uint32_t nextFetchMs;
static uint32_t lastWifiLogMs;
static bool bootScreenShown;
static bool wifiScreenShown;
static bool wifiDiagPrinted;
static String lastScreenText;

static void drawCenteredText(const String &text)
{
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t cx = ((display.width() - tbw) / 2) - tbx;
    uint16_t cy = ((display.height() - tbh) / 2) - tby;

#if IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS)
    // B/W panels support partial refresh (much faster). Only repaint the text box with padding.
    const int16_t pad = 6;
    int16_t px = cx + tbx - pad;
    int16_t py = cy + tby - pad;
    uint16_t pw = tbw + 2 * pad;
    uint16_t ph = tbh + 2 * pad;

    // Clamp to the display.
    if (px < 0)
    {
        pw -= (uint16_t)(-px);
        px = 0;
    }
    if (py < 0)
    {
        ph -= (uint16_t)(-py);
        py = 0;
    }
    if ((uint16_t)px + pw > display.width())
        pw = display.width() - (uint16_t)px;
    if ((uint16_t)py + ph > display.height())
        ph = display.height() - (uint16_t)py;

    display.setPartialWindow(px, py, pw, ph);
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(cx, cy);
    display.print(text);
    display.display(true);
#else
    // Tri-color panels are full-refresh only and slow by nature.
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(cx, cy);
    display.print(text);
    display.display(false);
#endif
}

static const char *wlStatusToString(wl_status_t status)
{
    switch (status)
    {
    case WL_NO_SHIELD:
        return "WL_NO_SHIELD";
    case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
        return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
        return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
        return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
        return "WL_DISCONNECTED";
    default:
        return "WL_UNKNOWN";
    }
}

static const char *disconnectReasonToString(uint8_t reason)
{
    // Values are from the SDK's wifi_event_reason_t; keep this best-effort.
    switch (reason)
    {
    case 1:
        return "UNSPECIFIED";
    case 2:
        return "AUTH_EXPIRE";
    case 3:
        return "AUTH_LEAVE";
    case 4:
        return "ASSOC_EXPIRE";
    case 5:
        return "ASSOC_TOOMANY";
    case 6:
        return "NOT_AUTHED";
    case 7:
        return "NOT_ASSOCED";
    case 8:
        return "ASSOC_LEAVE";
    case 9:
        return "ASSOC_NOT_AUTHED";
    case 10:
        return "DISASSOC_PWRCAP_BAD";
    case 11:
        return "DISASSOC_SUPCHAN_BAD";
    case 13:
        return "IE_INVALID";
    case 14:
        return "MIC_FAILURE";
    case 15:
        return "4WAY_HANDSHAKE_TIMEOUT";
    case 16:
        return "GROUP_KEY_UPDATE_TIMEOUT";
    case 17:
        return "IE_IN_4WAY_DIFFERS";
    case 18:
        return "GROUP_CIPHER_INVALID";
    case 19:
        return "PAIRWISE_CIPHER_INVALID";
    case 20:
        return "AKMP_INVALID";
    case 21:
        return "UNSUPP_RSN_IE_VERSION";
    case 22:
        return "INVALID_RSN_IE_CAP";
    case 23:
        return "802_1X_AUTH_FAILED";
    case 24:
        return "CIPHER_SUITE_REJECTED";
    case 200:
        return "BEACON_TIMEOUT";
    case 201:
        return "NO_AP_FOUND";
    case 202:
        return "AUTH_FAIL";
    case 203:
        return "ASSOC_FAIL";
    case 204:
        return "HANDSHAKE_TIMEOUT";
    default:
        return "(unknown)";
    }
}

static void printScanResults()
{
    Serial.println("Scanning WiFi...");
    int n = WiFi.scanNetworks(/*async*/ false, /*hidden*/ true);
    if (n < 0)
    {
        Serial.printf("scanNetworks failed: %d\n", n);
        return;
    }

    Serial.printf("Found %d networks\n", n);
    for (int i = 0; i < n; i++)
    {
        String foundSsid = WiFi.SSID(i);
        int32_t rssi = WiFi.RSSI(i);
        uint8_t enc = WiFi.encryptionType(i);
        int32_t channel = WiFi.channel(i);
        uint8_t *bssid = WiFi.BSSID(i);
        Serial.printf(
            "[%2d] ssid='%s' rssi=%lddBm ch=%ld enc=%u bssid=%02X:%02X:%02X:%02X:%02X:%02X\n",
            i,
            foundSsid.c_str(),
            (long)rssi,
            (long)channel,
            (unsigned)enc,
            bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    }
    WiFi.scanDelete();
}

static WiFiEventHandler onConnected;
static WiFiEventHandler onGotIp;
static WiFiEventHandler onDisconnected;

static void onWifiBecameStuck(uint32_t now)
{
    if (!bootScreenShown && (now - bootStartMs) > WIFI_BOOT_SCREEN_AFTER_MS)
    {
        bootScreenShown = true;
        drawCenteredText("BOOT");
        display.hibernate();
        return;
    }

    if (!wifiScreenShown && (now - wifiStartMs) > WIFI_STUCK_SCREEN_AFTER_MS)
    {
        wifiScreenShown = true;
        drawCenteredText("WiFi...");
        display.hibernate();
        return;
    }

    if (!wifiDiagPrinted && (now - wifiStartMs) > WIFI_DIAG_AFTER_MS)
    {
        wifiDiagPrinted = true;
        printScanResults();
        WiFi.printDiag(Serial);
    }

    if (now - lastWifiLogMs > 5000)
    {
        lastWifiLogMs = now;
        wl_status_t st = WiFi.status();
        Serial.printf("WiFi... %s\n", wlStatusToString(st));
    }
}

void setup(void)
{
    Serial.begin(9600);
    // Keep our own logs, avoid SDK spam.
    Serial.setDebugOutput(false);
    Serial.println();
    Serial.println("setup");
    Serial.printf("reset: %s\n", ESP.getResetReason().c_str());
    Serial.printf("free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("epd pins: CS=%d DC=%d RST=%d BUSY=%d\n", EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);
    display.init();
    display.setRotation(3);
    display.setFont(&FreeMonoBold18pt7b);
    display.setTextColor(GxEPD_BLACK);
    // Don't draw on boot unless we're "stuck" (tri-color refresh is slow).
    // WiFi.setPhyMode(WIFI_PHY_MODE_11N);

    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.mode(WIFI_STA);
    WiFi.hostname("Co2Screen");

    onConnected = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected &evt)
                                              { Serial.printf("[wifi] connected to '%s' ch=%d\n", evt.ssid.c_str(), evt.channel); });
    onGotIp = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &evt)
                                      { Serial.printf("[wifi] got ip: %s gw: %s mask: %s\n",
                                                      evt.ip.toString().c_str(),
                                                      evt.gw.toString().c_str(),
                                                      evt.mask.toString().c_str()); });
    onDisconnected = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &evt)
                                                    {
                                                        Serial.printf("[wifi] disconnected from '%s' reason=%u (%s)\n",
                                                                      evt.ssid.c_str(),
                                                                      (unsigned)evt.reason,
                                                                      disconnectReasonToString(evt.reason));
                                                        // Mark WiFi as "in progress" again. Keep the last value on screen.
                                                        wifiStartMs = millis();
                                                        wifiScreenShown = false;
                                                        wifiDiagPrinted = false;
                                                        lastWifiLogMs = 0; });

    bootStartMs = millis();
    wifiStartMs = bootStartMs;
    nextFetchMs = bootStartMs; // fetch ASAP once connected
    lastWifiLogMs = 0;
    bootScreenShown = false;
    wifiScreenShown = false;
    wifiDiagPrinted = false;

    Serial.printf("Connecting to ssid='%s'...\n", ssid);
    WiFi.begin(ssid, password);
}

void loop(void)
{
    const uint32_t now = millis();

    if (WiFi.status() != WL_CONNECTED)
    {
        onWifiBecameStuck(now);
        delay(25);
        return;
    }

    // Connected.
    if (now - lastWifiLogMs > 30000)
    {
        lastWifiLogMs = now;
        Serial.printf("WiFi ok: %s\n", WiFi.localIP().toString().c_str());
    }

    if (now < nextFetchMs)
    {
        delay(25);
        return;
    }

    nextFetchMs = now + FETCH_INTERVAL_MS;

    if (/* Ping.ping("192.168.0.42") && Ping.ping("192.168.0.5") */ true)
    {
        WiFiClientSecure client;
        HTTPClient http;
        client.setInsecure();
        http.begin(client, "https://cubox.dev/files/co2");
        int returnCode = http.GET();
        Serial.printf("HTTP status: %d\n", returnCode);
        String payload;
        if (returnCode > 0)
        {
            payload = http.getString();
        }
        http.end();

        String text;
        if (returnCode == 200)
        {
            payload.trim();
            int nl = payload.indexOf('\n');
            if (nl >= 0)
                payload = payload.substring(0, nl);

            text = payload;
            if (text.length() == 0)
                text = "ERROR";
            else
                text += " PPM";

            Serial.printf("ppm raw: '%s'\n", payload.c_str());
        }
        else
        {
            text = "HTTP ";
            text += returnCode;
            Serial.printf("HTTP failed: %s\n", HTTPClient::errorToString(returnCode).c_str());
        }

        // Only refresh e-paper when the visible text changes.
        if (text != lastScreenText)
        {
            lastScreenText = text;
            drawCenteredText(text);
            display.hibernate();
        }
    }
}
