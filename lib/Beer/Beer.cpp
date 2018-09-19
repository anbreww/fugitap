#include "Beer.h"


String Beer::name(void) {
    if (ESP.getChipId() == 1185308) {
        return String("Legen-dairy");
    } else {
        return String("Armadillo P.A.");
    }
}

String Beer::type(void)
{
    if (ESP.getChipId() == 1185308) {
        return String("Milk Stout");
    } else {
        return String("Am. Pale Ale");
    }
}

String Beer::abv(void)
{
    if (ESP.getChipId() == 1185308) {
        return String("5.5%");
    } else {
        return String("4.8%");
    }
}

String Beer::ibu(void)
{
    if (ESP.getChipId() == 1185308) {
        return String("20");
    } else {
        return String("50");
    }
}

String Beer::og(void)
{
    if (ESP.getChipId() == 1185308) {
        return String("1.070");
    } else {
        return String("1.055");
    }
}