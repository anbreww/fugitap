#include <Arduino.h>

class Beer {
    public:
        String  name(void),
                type(void),
                abv(void),
                ibu(void),
                og(void);
        uint8_t tap(void);
};