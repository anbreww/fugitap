#include <Arduino.h>
#include <FlowMeter.h>
#include "WebResource.h"

class Beer {
    public:
        Beer(FlowMeter& meter);
        void    set_tap(int8_t tap_no);
        void    set_full_vol(double);
        void    set_poured(double);
        void    set_img(String new_img);
        void    refresh(void);
        String  name(void),
                type(void),
                abv(void),
                ibu(void),
                glass_img(void),
                og(void);
        uint8_t tap(void);
        double  full_vol(void),
                volume(void);
        uint32_t last_updated(void);
        bool    is_pouring(void);
    private:
        int8_t _tap_no;
        String  _name,
                _style,
                _og,
                _untappd_url,
                _glass_url,
                _abv,
                _ibu,
                _glass_img;
        double  _full_vol,
                _poured;
        uint32_t _last_updated;
        uint32_t _last_pouring;
        FlowMeter& _flow_meter;
        void loadSamples(void);
};