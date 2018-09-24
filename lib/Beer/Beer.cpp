#include "Beer.h"
#include <ESP8266WiFi.h>

void Beer::init() {
    _tap_no = -1;
}

void Beer::set_tap(int8_t tap_no) {
    Serial.println("Setting tap number " + String(tap_no) );
    _tap_no = tap_no;
}


uint8_t Beer::tap(void) {
    if (_tap_no == -1) {
        uint8_t index = (WiFi.localIP()[3] % 10);
        return min(index, (uint8_t) 7);
    } else {
        return _tap_no;
    }
}

String Beer::name(void) {
    String beers[] = {
        "Armadillo P.A",
        "Milkshake IPA",
        "Smollusque IPA",
        "Legen-dairy",
        "She May Rouge",
        "Smoked Porter",
        "Not defined"
    };

    Serial.println("Getting beer number " + String(this->tap() - 1));
    return beers[this->tap() - 1];
}

String Beer::type(void)
{
    String types[] = {
        "Am. Pale Ale",
        "New England IPA",
        "American IPA",
        "Milk Stout",
        "Belgian Abbey",
        "Porter",
        "Derp"
    };

    return types[this->tap() - 1];
}

String Beer::abv(void)
{
    String abvs[] = {
        "4.5%",
        "5.3%",
        "7.2%",
        "4.5%",
        "8.3%",
        "5.0%",
        "N/A%",
    };

    return abvs[this->tap() - 1];
}

String Beer::ibu(void)
{
    String ibus[] = {
        "20",
        "10",
        "99",
        "25",
        "20",
        "20",
        "NA",
    };

    return ibus[this->tap() - 1];
}

String Beer::og(void)
{
    String ogs[] = {
        "1.045",
        "1.054",
        "1.064",
        "1.066",
        "1.070",
        "1.051",
        "NA",
    };

    return ogs[this->tap() - 1];
}