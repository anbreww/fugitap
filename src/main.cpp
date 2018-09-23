#include <Arduino.h>
#include <ESP.h>

// Display driver
#include <SPI.h>
#include <TFT_eSPI.h>
#include <FS.h>

// Download manager, OTA updates etc
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>

// TODO later : setup WiFi manager
// #include <WiFiManager.h>

// Flow meter input
#include <FlowMeter.h>

#include "settings.h"
#include "SPIFFS_Support.h"
#include "GfxUi.h"
#include "Beer.h"

#define SERIAL_DEBUG

#define HOSTNAME    "FUGITAP-OTA-"
#define UART_BAUD   115200

#define TFT_WIDTH   240
#define TFT_HEIGHT  320

#define PIN_BACKLIGHT   12  // GPIO12 - pin D6
#define PIN_FLOW_IN     4  // GPIO4 - WS2812 pin on PCB (v1 is GPIO2 but doesn't work)

#define FONT_STATUS &FreeMono9pt7b
#define FONT_LABELS &FreeMonoBold9pt7b
#define FONT_BEER   &FreeMonoBold12pt7b
#define FONT_STATS  &FreeMono12pt7b


// TFT Library : check User_Setup.h in the library for your hardware settings!
TFT_eSPI tft = TFT_eSPI();
GfxUi ui = GfxUi(&tft);

void initScreen(void);
void drawBeerScreen(void);
void drawFillMeter(bool update_fill);
void writeStatusBar(const char *status, uint16_t text_color);
void drawFlowRate(void);
void drawFlowScreen(void);
void MeterISR(void);    // flow meter ISR

// define the sensor characteristics here
const double cap = 20.0f;       // l/min
const double kf = 93.3333f;     // Hz per l/min

// let's provide our own sensor properties (without applying further calibration)
FlowSensorProperties MySensor = {cap, kf, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

FlowMeter Meter = FlowMeter(PIN_FLOW_IN, MySensor);

void setup() {
#ifdef SERIAL_DEBUG
    Serial.begin(UART_BAUD);
#endif

    initScreen();

    ArduinoOTA.onStart([]() {
        digitalWrite(PIN_BACKLIGHT, HIGH);
        Serial.println("OTA Starting");
        writeStatusBar("OTA UPDATE STARTING", TFT_ORANGE);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("Done! Rebooting");
        writeStatusBar("DONE. REBOOTING!", TFT_ORANGE);
    });

    SPIFFS.begin();


    listFiles();
    // Serial.println("Formatting SPIFFS, please wait.");
    // SPIFFS.format();

    Serial.println("Initializing flow sensor");

    // Flow meter init
    // pinMode(PIN_FLOW_IN, INPUT);
    attachInterrupt(PIN_FLOW_IN, MeterISR, RISING);
    Meter.reset();

    delay(1500);

    Serial.println("setup() finished.");
}

void loop()
{
    static int32_t lastScreenUpdate = -1;
    static int32_t lastFillUpdate = -1;
    ArduinoOTA.handle();
    //pinMode(PIN_FLOW_IN, INPUT);

    // if (Meter.getCurrentFlowrate() > 0.01) {
    //     if (millis() - lastScreenUpdate > 1500) {
    //         drawFlowScreen();
    //         drawFillMeter(false);
    //     }
    //     return;
    // }

    // only update beer status screen every 30 seconds
    if (millis() - lastScreenUpdate > 30000 || lastScreenUpdate == -1) {
        drawBeerScreen();
        drawFillMeter(false);
        lastScreenUpdate = millis();
    }

    // update pour status live
    if (millis() - lastFillUpdate > 1500 || lastFillUpdate == -1 ) {
        drawFillMeter(true);
        lastFillUpdate = millis();
        drawFlowRate();
    }
}


#define NUM_LINES   4
#define MARGIN  10
const uint16_t line_pos[] = {28, 100, 170, 265};
const uint8_t sp_top = 5; // space between line and text on top
const uint8_t sp_bot = 7; // space between line and text below

#define LBL_FONT    2
#define STAT_FONT   4

void MeterISR(void) {
    Meter.count();
}

void drawFlowRate(void) {
    const uint8_t line_h = line_pos[0] - sp_top;
    const uint8_t text_w = 75;

    Meter.tick(); // process ticks

    String flow_rate = String(Meter.getCurrentFlowrate());
    String total_vol = String(Meter.getTotalVolume());
    String duration = String(Meter.getTotalDuration()/1000);

    tft.setTextDatum(BL_DATUM);
    tft.setFreeFont(FONT_LABELS);

    const bool debug = false;

    if (debug) {
        // beer line
        tft.fillRect(MARGIN+text_w, line_pos[0] - sp_top - 23, 240-2*MARGIN-text_w, line_pos[0] - sp_top, TFT_BLACK);
        tft.drawString(flow_rate + " l/min", MARGIN+text_w, line_pos[0] - sp_top);
        // style line
        tft.fillRect(MARGIN+text_w, line_pos[1] - sp_top - 23, 240-2*MARGIN-text_w, line_pos[0] - sp_top, TFT_BLACK);
        tft.drawString(total_vol + " l", MARGIN+text_w, line_pos[1] - sp_top);
        // stats line
        tft.fillRect(MARGIN+text_w, line_pos[2] - sp_top - 23, 240-2*MARGIN-text_w, line_pos[0] - sp_top, TFT_BLACK);
        tft.drawString(duration + " s.", MARGIN+text_w, line_pos[2] - sp_top);
    }
    // fill line
    tft.fillRect(MARGIN+text_w, line_pos[3] - sp_top - 23, 240-2*MARGIN-text_w, line_pos[0] - sp_top, TFT_BLACK);
    tft.drawString(flow_rate + " l/min", MARGIN+text_w, line_pos[3] - sp_top);

}

void drawFlowScreen(void) {
    const uint8_t line_h = line_pos[0] - sp_top;
    const uint8_t text_w = 60;

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    Meter.tick(); // process ticks

    String flow_rate = String(Meter.getCurrentFlowrate()) + " l/min";
    String total_vol = String(Meter.getTotalVolume()) + " l";
    String duration = String(Meter.getTotalDuration()/1000) + " s.";
    // draw divider lines first
    tft.drawLine(MARGIN, line_pos[0],  TFT_WIDTH-MARGIN, line_pos[0], TFT_WHITE);
    tft.drawLine(MARGIN, line_pos[3],  TFT_WIDTH-MARGIN, line_pos[3], TFT_WHITE);
    

    Beer beer;
    
    // labels are positioned above the lines
    tft.setTextDatum(BL_DATUM);
    tft.setFreeFont(FONT_LABELS);
    tft.drawString("BEER", MARGIN, line_pos[0] - sp_top);
    tft.drawString("FILL", MARGIN, line_pos[3] - sp_top);

    // stats are positioned below the lines
    tft.setTextDatum(TL_DATUM);
    tft.setFreeFont(FONT_BEER);
    tft.setTextWrap(true);
    tft.drawString(beer.name(), MARGIN, line_pos[0] + sp_bot);

    tft.drawString("Flow Rate: " + flow_rate, MARGIN, line_pos[1] + sp_bot);
    tft.drawString("Total Vol: " + flow_rate, MARGIN, line_pos[1] + sp_bot + 20);
    tft.drawString("Duration : " + flow_rate, MARGIN, line_pos[1] + sp_bot + 40);

    writeStatusBar((String(ESP.getChipId()) + "  " + WiFi.localIP().toString()).c_str(), TFT_YELLOW);
}

void drawBeerScreen(void) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    // draw divider lines first
    for (uint8_t i = 0; i < NUM_LINES; i++) {
        tft.drawLine(MARGIN, line_pos[i],  TFT_WIDTH-MARGIN, line_pos[i], TFT_WHITE);
    }

    Beer beer;
    // stats are positioned below the lines
    tft.setTextDatum(TL_DATUM);
    tft.setFreeFont(FONT_BEER);
    tft.setTextWrap(true);
    tft.drawString(beer.name(), MARGIN, line_pos[0] + sp_bot);

    
    // labels are positioned above the lines
    tft.setTextDatum(BL_DATUM);
    tft.setFreeFont(FONT_LABELS);
    tft.drawString("BEER", MARGIN, line_pos[0] - sp_top);
    tft.drawString("STYLE", MARGIN, line_pos[1] - sp_top);
    tft.drawString("STATS", MARGIN, line_pos[2] - sp_top);
    tft.drawString("FILL", MARGIN, line_pos[3] - sp_top);
    
    // stats are positioned below the lines
    tft.setTextDatum(TL_DATUM);
    tft.setFreeFont(FONT_BEER);
    tft.setTextWrap(true);
    //String beername = "Shoop da whoop";
    //String beername = Firebase.getString("fugidaire/v2/beers/beer1/name");
    //tft.drawString(beername, MARGIN, line_pos[0] + sp_bot);
    tft.drawString(beer.name(), MARGIN, line_pos[0] + sp_bot);
    //tft.drawString(beer_name[1], MARGIN, line_pos[0] + sp_bot + 20);
    tft.setFreeFont(FONT_STATS);
    tft.drawString(beer.type(), MARGIN, line_pos[1] + sp_bot);
    tft.setTextWrap(false);
    tft.drawString("ABV: " + beer.abv(), MARGIN, line_pos[2] + sp_bot);
    tft.drawString("IBU: " + beer.ibu(), MARGIN, line_pos[2] + sp_bot + 20);
    tft.drawString("OG : " + beer.og(), MARGIN, line_pos[2] + sp_bot + 40);

    // Placeholder for glass which will be BMP / JPEG / PNG
    const uint8_t glass_w = 70;
    const uint8_t glass_h = 60;
    tft.drawRect(240 - MARGIN - glass_w, line_pos[2] + sp_bot, glass_w, glass_h, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("glass.png", 240 - MARGIN - glass_w/2, line_pos[2] + sp_bot + glass_h/2, LBL_FONT);

    //writeStatusBar("Last pour : 230ml", TFT_YELLOW);
    writeStatusBar((String(ESP.getChipId()) + "  " + WiFi.localIP().toString()).c_str(), TFT_YELLOW);
}

void drawFillMeter(bool update_fill) {
    // draw fill meter, capacity remaining and flow rate below
    double litres = (19.00 - Meter.getTotalVolume());

    int8_t fill_percent = (uint8_t) litres * 100 / 19;
    if (fill_percent > 100) {
        fill_percent = 100;
    }
    if (litres < 0) {
        fill_percent = 0;
    }

    uint16_t fill_color = TFT_GREEN;
    if (fill_percent < 20) {
        fill_color = TFT_RED;
    } else if (fill_percent < 30) {
        fill_color = TFT_ORANGE;
    } else if (fill_percent < 40) {
        fill_color = TFT_YELLOW;
    }

    tft.fillRect(240-95, line_pos[3] + sp_bot, 240, 20, TFT_BLACK);
    ui.drawProgressBar(MARGIN, line_pos[3] + sp_bot, 240-100, 20, fill_percent, 
        TFT_WHITE, fill_color);
    
    tft.setTextDatum(TR_DATUM);
    tft.setFreeFont(FONT_STATUS);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(String(litres) + "l", TFT_WIDTH-MARGIN, line_pos[3] + sp_bot);

    //analogWrite(PIN_BACKLIGHT, PWMRANGE*fill_percent/100);

    if (!update_fill) {
        return;
    }

    // if (fill_percent > 5) {
    //     fill_percent -= 5;
    // } else {
    //     fill_percent = 100;
    // }
}



void initScreen(void) {
    tft.begin();
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(0);

    tft.setFreeFont(FONT_STATUS);

    pinMode(PIN_BACKLIGHT, OUTPUT);
    analogWrite(PIN_BACKLIGHT, PWMRANGE/4);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    tft.setTextDatum(BC_DATUM);
    tft.drawString("Fugidaire Tap Manager", 120, 40);
    tft.drawString("Andrew Watson - 2017", 120, 60);

    Serial.println("\nConnecting to WiFi");


    // paint the UI while we wait for the wifi
    drawBeerScreen();
    drawFillMeter(false);

    tft.setTextDatum(BC_DATUM);
    tft.setFreeFont(FONT_STATUS);
    writeStatusBar("Connecting to WiFi", TFT_WHITE);

    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    //pinMode(FLOW_IN_PIN, INPUT);

    writeStatusBar("Connected Successfully", TFT_WHITE);
    delay(1500);
    //tft.fillRect
    char ip_status[30] = "IP: ";
    strcat(ip_status, WiFi.localIP().toString().c_str());
    writeStatusBar(ip_status, TFT_WHITE);
    delay(2500);
}

void writeStatusBar(const char * status, uint16_t text_color)
{
    tft.setTextDatum(BC_DATUM);
    tft.setFreeFont(FONT_STATUS);
    tft.setTextColor(text_color);
    tft.fillRect(0, 300, 240, 20, TFT_BLACK);
    tft.drawString(status, 120, 318);
}