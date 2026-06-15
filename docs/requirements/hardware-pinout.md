# Hardware Pinout

Complete GPIO map for the **Waveshare ESP32-S3 1.54" e-Paper** board (codec board
id `S3_ePaper_1_54`), as wired by the reference firmware. scribr targets the same
board. Captured here so the mapping survives removal of the reference firmware.

## E-paper display (SPI2 / `SPI2_HOST`)

| Signal | GPIO | Notes                        |
| ------ | ---- | ---------------------------- |
| DC     | 10   | data/command                 |
| CS     | 11   | chip select                  |
| SCK    | 12   | SPI clock                    |
| MOSI   | 13   | SPI data                     |
| RST    | 9    | reset                        |
| BUSY   | 8    | busy (input)                 |
| PWR    | 6    | panel power rail, active-low |

Panel: 200×200, 1-bit (1.54"). See `docs/device-rendering-constraints.md`.

## Buttons (active-low, internal pull-ups)

| Button     | GPIO | Notes                                            |
| ---------- | ---- | ------------------------------------------------ |
| BOOT / REC | 0    | also strapping pin; deep-sleep EXT1 wake source  |
| PWR        | 18   | deep-sleep EXT1 wake source                      |

## Power rails (active-low enables)

| Rail        | GPIO | Notes                                       |
| ----------- | ---- | ------------------------------------------- |
| VBAT latch  | 17   | software power-hold — must be held to stay on |
| E-paper     | 6    | (same as EPD PWR above)                     |
| Audio       | 42   | codec / amplifier                           |

## Battery sense

| Signal      | GPIO | Notes                                          |
| ----------- | ---- | ---------------------------------------------- |
| Battery ADC | 4    | ADC1_CH3, `ADC_11db`, ×2 divider (see time-power-spec) |

## I²C bus (shared)

| Signal | GPIO |
| ------ | ---- |
| SDA    | 47   |
| SCL    | 48   |

Devices on the bus:

| Device            | 7-bit addr | Purpose                  |
| ----------------- | ---------- | ------------------------ |
| ES8311 codec      | 0x18 (`0x30` 8-bit) | audio in/out    |
| PCF85063 RTC      | 0x51       | real-time clock          |
| SHTC3             | 0x70       | temp / humidity sensor   |

## Audio I²S (codec ES8311)

| Signal      | GPIO | Notes              |
| ----------- | ---- | ------------------ |
| MCLK        | 14   | master clock       |
| BCLK        | 15   | bit clock          |
| WS (LRCLK)  | 38   | word select        |
| DOUT        | 45   | data out (to codec)|
| DIN         | 16   | data in (from mic) |
| PA enable   | 46   | amplifier enable   |

## SD card (SD-MMC, 1-bit mode)

| Signal | GPIO |
| ------ | ---- |
| CLK    | 39   |
| CMD    | 41   |
| D0     | 40   |

## Deep-sleep wake

EXT1 wake on **GPIO0 (BOOT/REC)** and **GPIO18 (PWR)**,
`ESP_EXT1_WAKEUP_ANY_LOW` — any button press wakes the device.

See also `docs/requirements/recording-playback-spec.md` and
`docs/requirements/time-power-spec.md`.
