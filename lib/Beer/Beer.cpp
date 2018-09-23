#include "Beer.h"
#include <ESP8266WiFi.h>

uint8_t Beer::tap(void) {
    uint8_t index = (WiFi.localIP()[3] % 10) - 1;
    return min(index, (uint8_t) 6);
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

    return beers[this->tap()];
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

    return types[this->tap()];
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
    
    return abvs[this->tap()];
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

    return ibus[this->tap()];
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

    return ogs[this->tap()];
}