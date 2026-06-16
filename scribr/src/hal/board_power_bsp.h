#ifndef BOARD_POWER_BSP_H
#define BOARD_POWER_BSP_H

class board_power_bsp_t
{
private:
    const uint8_t epd_power_pin;
    const uint8_t audio_power_pin;
    const uint8_t vbat_power_pin;

public:
    board_power_bsp_t(uint8_t _epd_power_pin,uint8_t _audio_power_pin,uint8_t _vbat_power_pin);
    ~board_power_bsp_t();

    void POWEER_EPD_ON();
    void POWEER_EPD_OFF();
    void POWEER_Audio_ON();
    void POWEER_Audio_OFF();

    // GPIO17 is a power-hold latch, not an active-low peripheral rail:
    // HIGH holds board power, LOW releases it for intentional shutdown.
    void holdVbatLatch();
    void releaseVbatLatch();

    // Backward-compatible prototype names. Prefer the explicit latch names.
    void VBAT_POWER_ON();
    void VBAT_POWER_OFF();
};

#endif