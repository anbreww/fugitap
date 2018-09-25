#include <Arduino.h>
// #include <ESP.h>

// Display driver
#include <SPI.h>
#include <TFT_eSPI.h>
#include <FS.h>

// Download manager, OTA updates etc
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>

// MQTT
#include <PubSubClient.h>
#define MQTT_VERSION MQTT_VERSION_3_1_1
void mqtt_callback(char *p_topic, byte *p_payload, unsigned int p_length);

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

// MQTT
WiFiClient wifiClient;
PubSubClient client(wifiClient);


// TFT Library : check User_Setup.h in the library for your hardware settings!
TFT_eSPI tft = TFT_eSPI();
GfxUi ui = GfxUi(&tft);

void initScreen(void);
void drawBeerScreen(void);
void drawFillMeter(bool update_fill);
void writeStatusBar(const char * status, uint16_t text_color);
void writeStatusBar(const char * status, uint16_t text_color, bool force);
void drawFlowRate(void);
void drawFlowScreen(void);
void MeterISR(void);    // flow meter ISR

// MQTT stuff
void hello(void);

// define the sensor characteristics here
const double cap = 20.0f;       // l/min
const double kf = 93.3333f;     // Hz per l/min

// let's provide our own sensor properties (without applying further calibration)
FlowSensorProperties MySensor = {cap, kf, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

FlowMeter Meter = FlowMeter(PIN_FLOW_IN, MySensor);

bool debug = false;

Beer beer(Meter);

String my_topic = "fugi/taps/XX";
void pouring_callback(bool pouring);

void setup() {
#ifdef SERIAL_DEBUG
    Serial.begin(UART_BAUD);
#endif

    initScreen();

    ArduinoOTA.onStart([]() {
        digitalWrite(PIN_BACKLIGHT, HIGH);
        Serial.println("OTA Starting");
        writeStatusBar("OTA UPDATE STARTING", TFT_ORANGE, true);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("Done! Rebooting");
        writeStatusBar("DONE. REBOOTING!", TFT_ORANGE, true);
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

    // Connect to MQTT server
    Serial.println("Connecting to MQTT server");
    client.setServer(mqtt_server_host, mqtt_server_port);
    client.setCallback(mqtt_callback);
}

void reconnect()
{
    millis();
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("INFO: Attempting MQTT connection...");
        Serial.println(client.state());
        // Attempt to connect
        long time = millis();
        String clientID = mqtt_client_id + time;
        if (client.connect(clientID.c_str(), mqtt_server_user, mqtt_server_pass))
        {
            Serial.println("INFO: connected");
            Serial.print("Connected : ");
            Serial.println(client.connected());
            Serial.print("State rc=");
            Serial.println(client.state());
            // ... and resubscribe
            //client.subscribe(MQTT_TOPIC_LEDS);
            client.subscribe(MQTT_TOPIC_TAPS);

            hello();
            break;
        }
        else
        {
            Serial.print("ERROR: failed, rc=");
            Serial.print(client.state());
            Serial.println("DEBUG: try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

uint32_t update_frequency(void)
{
    if (millis() < 300000) {
        return 30 * 1000;   // 30 seconds
    } else {
        return 60 * 60 * 1000;  // 1 hour
    }
}

void loop()
{
    static int32_t lastScreenUpdate = -1;
    static int32_t lastFillUpdate = -1;
    static int32_t lastFlowUpdate = -1;
    ArduinoOTA.handle();
    client.loop();
    //pinMode(PIN_FLOW_IN, INPUT);

    // if (Meter.getCurrentFlowrate() > 0.01) {
    //     if (millis() - lastScreenUpdate > 1500) {
    //         drawFlowScreen();
    //         drawFillMeter(false);
    //     }
    //     return;
    // }

    // update beer screen every 30 seconds for 5 minutes, then every hour
    if (millis() - lastScreenUpdate > update_frequency() || lastScreenUpdate == -1 ||
        beer.last_updated() > lastScreenUpdate) {
        // TODO : update beer screen on change of beer
        drawBeerScreen();
        drawFillMeter(false);
        lastScreenUpdate = millis();
    }

    // update pour status live
    if (millis() - lastFlowUpdate > 500) {
        lastFlowUpdate = millis();
        drawFlowRate();
    }

    if (millis() - lastFillUpdate > 2500 || lastFillUpdate == -1 ) {
        drawFillMeter(true);
        lastFillUpdate = millis();
        lastFlowUpdate = millis();
        drawFlowRate();
    }

    if (!client.connected())
    {
        Serial.print("Client not connected. Going to Reconnect. rc=");
        Serial.println(client.state());
        reconnect();
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
    const uint8_t text_w = 85;

    static uint32_t last_update = millis()-1000;
    uint32_t now = millis();

    if (now - last_update < 250) {
        return;
    }

    //Serial.print("Duration = ");
    //Serial.println(now - last_update);
    Meter.tick(now - last_update); // process ticks
    last_update = now;

    double flow_rate = Meter.getCurrentFlowrate();

    String flow_rate_str = String(Meter.getCurrentFlowrate());
    String total_vol = String(Meter.getTotalVolume());
    //Serial.print("Vols : ");
    //Serial.print(Meter.getCurrentVolume());
    //Serial.print(" ");
    //Serial.println(Meter.getTotalVolume());
    String duration = String(Meter.getTotalDuration()/1000);

    tft.setTextDatum(BL_DATUM);
    tft.setFreeFont(FONT_LABELS);

    if (debug) {
        // beer line
        tft.fillRect(MARGIN+text_w, line_pos[0] - sp_top - 23, 240-2*MARGIN-text_w, line_pos[0] - sp_top, TFT_BLACK);
        tft.drawString("pouring: " + String(beer.is_pouring()), MARGIN+text_w, line_pos[0] - sp_top);
        // style line
        tft.fillRect(MARGIN+text_w, line_pos[1] - sp_top - 23, 240-2*MARGIN-text_w, line_pos[0] - sp_top, TFT_BLACK);
        tft.drawString(total_vol + " l", MARGIN+text_w, line_pos[1] - sp_top);
        // stats line
        tft.fillRect(MARGIN+text_w, line_pos[2] - sp_top - 23, 240-2*MARGIN-text_w, line_pos[0] - sp_top, TFT_BLACK);
        tft.drawString(duration + " s.", MARGIN+text_w, line_pos[2] - sp_top);
    }
    // fill line
    tft.fillRect(MARGIN+text_w, line_pos[3] - sp_top - 23, 240-2*MARGIN-text_w, line_pos[0] - sp_top, TFT_BLACK);
    if (beer.is_pouring()) {
        tft.drawString(flow_rate_str + " l/min", MARGIN+text_w, line_pos[3] - sp_top);
    }

    pouring_callback(beer.is_pouring());
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

    writeStatusBar((String(ESP.getChipId(), HEX) + "  " + WiFi.localIP().toString()).c_str(), TFT_YELLOW);
}

void drawBeerScreen(void) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    // draw divider lines first
    for (uint8_t i = 0; i < NUM_LINES; i++) {
        tft.drawLine(MARGIN, line_pos[i],  TFT_WIDTH-MARGIN, line_pos[i], TFT_WHITE);
    }

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
    writeStatusBar((String(ESP.getChipId(), HEX) + "  " + WiFi.localIP().toString()).c_str(), TFT_YELLOW);
}

void drawFillMeter(bool update_fill) {
    // draw fill meter, capacity remaining and flow rate below
    double litres = (beer.volume());

    //Serial.println("Litres: " + String(beer.volume()) + " full : " + String(beer.full_vol()));

    int8_t fill_percent = (uint8_t) litres * 100 / beer.full_vol();
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

    if(ESP.getChipId() == 15951948) {
        debug = true;
    }


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
    delay(500);
}

void writeStatusBar(const char * status, uint16_t text_color)
{
    writeStatusBar(status, text_color, false);
}

void writeStatusBar(const char * status, uint16_t text_color, bool force)
{

    tft.setTextDatum(BC_DATUM);
    tft.setFreeFont(FONT_STATUS);
    tft.setTextColor(text_color);
    tft.fillRect(0, 300, 240, 20, TFT_BLACK);
    if (millis() < 60000 || force) {
        tft.drawString(status, 120, 318);
    }
}



// MQTT stuff
void mqtt_callback(char *p_topic, byte *p_payload, unsigned int p_length)
{
    Serial.println("MQTT Callback");
    String lookup = String("fugi/taps/lookup/") + String(ESP.getChipId());
    Serial.println(lookup);
    Serial.println(p_topic);
    static bool first_load = true;

    String payload;
    uint8_t i;

    for (i = 0; i < p_length; i++)
    {
        payload.concat((char)p_payload[i]);
    }


    if (lookup.equals(p_topic)) {
        // todo : unsubscribe from old tap number
        uint8_t tap = atoi((char*)p_payload);

        String status_str = String(ESP.getChipId(), HEX) + " set tap to " + String(tap);
        client.publish("fugi/taps/setting", status_str.c_str());
        beer.set_tap(tap);
        my_topic = "fugi/taps/" + String(tap);
        Serial.println("My topic : " + my_topic);
    }

    String remaining = my_topic + "/remaining";
    if (first_load && beer.tap() != -1 && remaining.equals(p_topic)) {
        double vol = atof((char*)p_payload);
        Serial.println("Loading volume (" + String(vol) + "l) from MQTT message");
        beer.set_poured(beer.full_vol() - vol);
        first_load = false;
    }

    String reset = my_topic + "/reset";
    if (reset.equals(p_topic)) {
        beer.set_poured(0);
        pouring_callback(false);
    }
    
    return;
}

void pouring_callback(bool pouring)
{
    static bool last_pouring = false;
    if (pouring != last_pouring) {
        String status = pouring ? "true":"false";
        client.publish((my_topic+ "/pouring").c_str() , status.c_str());;
        last_pouring = pouring;
        if (!pouring) {
            // finished pouring : update to MQTT
            client.publish((my_topic + "/remaining").c_str(), String(beer.volume()).c_str(), true);
        }
    }
}

void hello(void)
{
    String conn_str = "FugiTaps ESP-" + String(ESP.getChipId(), HEX)
                    + " is online at " + WiFi.localIP().toString();
    client.publish("fugi/taps/hello", conn_str.c_str(), true);
}