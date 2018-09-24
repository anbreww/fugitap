#define WIFI_SSID "exampleSSID"
#define WIFI_PASS "wifipassword"

const char *mqtt_server_host = "test.mosquitto.org";
const uint16_t mqtt_server_port = 1883;
const char *mqtt_server_user = "";
const char *mqtt_server_pass = "";
const char *mqtt_client_id = "fugitaps-dev";

const char *MQTT_TOPIC_TOWER = "fugi/leds/tower";
const char *MQTT_TOPIC_BASE = "fugi/leds/base";
const char *MQTT_TOPIC_LEDS = "fugi/leds/#";
const char *MQTT_TOPIC_TAPS = "fugi/taps/#";