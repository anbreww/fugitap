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
        "Armadillo P.A",
        "Milkshake IPA",
        "Smollusque IPA",
        "Legen-dairy",
        "She May Rouge",
        "Smoked Porter",
        "Debugging",
        "Not defined"
    };

    String types[] = {
        "Am. Pale Ale",
        "New England IPA",
        "American IPA",
        "Milk Stout",
        "Belgian Abbey",
        "Porter",
        "Testing",
        "Uninitialized"
    };

    String abvs[] = {
        "4.5%",
        "5.3%",
        "7.2%",
        "4.5%",
        "8.3%",
        "5.0%",
        "6.6%",
        "N/A%",
    };

    String ibus[] = {
        "20",
        "10",
        "99",
        "25",
        "20",
        "20",
        "66",
        "NA",
    };

    String ogs[] = {
        "1.045",
        "1.054",
        "1.064",
        "1.066",
        "1.070",
        "1.051",
        "1.666",
        "NA",
    };

    String glass_imgs[] = {
        "/glass02.bmp",
        "/glass02.bmp",
        "/glass03.bmp",
        "/glass01.bmp",
        "/glass02.bmp",
        "/glass01.bmp",
        "/glass02.bmp",
        "/glass02.bmp",
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
    return _full_vol - _flow_meter.getTotalVolume();
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