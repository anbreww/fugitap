#include "Beer.h"

uint8_t Beer::tap(void) {
    switch(ESP.getChipId()) {
        case 1185308:
            return 0;
        case 1184547:
            return 1;
        case 15951906:
            return 2;
        case 15951948:
            return 3;
        default:
            return 4;

    }
}

String Beer::name(void) {
    String beers[] = {
        "Armadillo P.A",
        "Dunkelweizen",
        "Milkshake IPA",
        "Smollusque IPA",
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
        "Dark Weizen",
        "New England IPA",
        "American IPA",
        "Belgian Abbey",
        "Porter",
        "Derp"
    };

    return types[this->tap()];
}

String Beer::abv(void)
{
    String abvs[] = {
        "5.5%",
        "5.0%",
        "6.5%",
        "7.0%",
        "8.5%",
        "4.7%",
        "N/A%",
    };
    
    return abvs[this->tap()];
}

String Beer::ibu(void)
{
    String ibus[] = {
        "10",
        "20",
        "30",
        "40",
        "50",
        "60",
        "NA",
    };

    return ibus[this->tap()];
}

String Beer::og(void)
{
    if (ESP.getChipId() == 1185308) {
        return String("1.070");
    } else {
        return String("1.055");
    }
}