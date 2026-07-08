#pragma once

#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
private:

    lgfx::Panel_ILI9341 _panel;
    lgfx::Bus_SPI       _bus;
    lgfx::Light_PWM     _light;
public:

    LGFX();
};

extern LGFX lcd;
