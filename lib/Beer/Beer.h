#include <Arduino.h>

class Beer {
    public:
        void    init(void),
                set_tap(int8_t tap_no);
        String  name(void),
                type(void),
                abv(void),
                ibu(void),
                og(void);
        uint8_t tap(void);
    private:
        int8_t _tap_no;
        double volume;


};