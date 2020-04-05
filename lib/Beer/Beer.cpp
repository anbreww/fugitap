#include "Beer.h"
#include <ESP8266WiFi.h>

// TODO : load beer stats from JSON
// TODO : download image from URL if doesn't exist
// TODO : save/load stats from local storage
// TODO : provide "reset" method to clear pour count

#define MAX_TAPS   7    // 6 real taps and #7 for debugging
#define POUR_TIMEOUT    1800
#define MIN_FLOW_RATE   0.1 // minimum flow to count as pouring

const char * glasses_url = "http://anbrew.ch";

Beer::Beer(FlowMeter& meter) : _flow_meter(meter) {
    _tap_no = -1;
    _full_vol = 19.00;
    loadSamples();
}

void Beer::loadSamples(void) {
    String beers[] = {
        "Gaellaxy P.A",
        "Milkshake IPA",
        "Ardennes IPA",
        "Armadillo P.A",
        "Oatless Stout",
        "AnBrew Porter",
        "Debugging",
        "Not defined"
    };

    String types[] = {
        "Am. Pale Ale",
        "New England IPA",
        "Belgian IPA",
        "Am. Pale Ale",
        "Stout",
        "Porter",
        "Testing",
        "Uninitialized"
    };

    String abvs[] = {
        "4.5%",
        "5.3%",
        "6.8%",
        "5.0%",
        "5.5%",
        "5.5%",
        "6.6%",
        "N/A%",
    };

    String ibus[] = {
        "40",
        "45",
        "55",
        "45",
        "55",
        "50",
        "66",
        "NA",
    };

    String ogs[] = {
        "1.045",
        "1.055",
        "1.078",
        "1.063",
        "1.070",
        "1.060",
        "1.666",
        "NA",
    };

    String glass_imgs[] = {
        "/glass02.bmp",
        "/glass02.bmp",
        "/glass02.bmp",
        "/glass02.bmp",
        "/glass02.bmp",
        "/glass01.bmp",
        "/glass02.bmp",
        "/glass01.bmp",
    };

    _name = beers[this->tap()-1];
    _style = types[this->tap()-1];
    _abv = abvs[this->tap()-1];
    _ibu = ibus[this->tap()-1];
    _og = ogs[this->tap()-1];
    _full_vol = 19.0;
    _glass_img = glass_imgs[this->tap()-1];

    _last_updated = millis();
}

void Beer::set_tap(int8_t tap_no) {
    Serial.println("Setting tap number " + String(tap_no) );
    _tap_no = tap_no;
    loadSamples();
}

void Beer::set_poured(double poured) {
    _flow_meter.setTotalVolume(poured);
    _last_updated = millis();
}

uint8_t Beer::tap(void) {
    if (_tap_no == -1) {
        if (!WiFi.isConnected()) {
            return MAX_TAPS + 1;
        }
        uint8_t index = (WiFi.localIP()[3] % 10);
        return min(index, (uint8_t) (MAX_TAPS + 1));
    } else {
        return (uint8_t) _tap_no;
    }
}

uint32_t Beer::last_updated(void) {
    return _last_updated;
}

String Beer::name(void) {
    return _name;
}

String Beer::type(void)
{
    return _style;
}

String Beer::abv(void)
{
    return _abv;
}

String Beer::ibu(void)
{

    return _ibu;
}

String Beer::og(void)
{
    return _og;
}

double Beer::full_vol(void)
{
    return _full_vol;
}

double Beer::volume(void)
{
    double vol = _full_vol - _flow_meter.getTotalVolume();
    return (vol > -2.0) ? vol : -2.0; 
}

bool Beer::is_pouring(void)
{
    uint32_t now = millis();
    if (_flow_meter.getCurrentFlowrate() > MIN_FLOW_RATE) {
        _last_pouring = now;
        return true;
    }

    if (now - _last_pouring < POUR_TIMEOUT) {
        return true;
    } else {
        return false;
    }
}

void Beer::set_img(String new_img)
{
    _glass_img = "/" + new_img;
    _last_updated = millis();
}

String Beer::glass_img(void)
{
    return _glass_img;
}

void Beer::refresh(void)
{
    _last_updated = millis();
}