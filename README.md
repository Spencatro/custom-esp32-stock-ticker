# ESP32 Custom Stock Ticker LED Grid

Display a stock symbol on a custom-build multiplexed LED grid using ESP32 SOC and finnhub.io

![Stock Ticker Video](https://media.giphy.com/media/bXthvr191GygM4luh5/giphy-downsized-large.gif)

This project is derived from a few esp32 idf examples:
- peripherals/gpio/generic_gpio/ - basic GPIO operation
- protocols/https_request/       - basic HTTPS requests

## Setup

- Install ESP-IDF . (The VS Code setup is super easy. see: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/vscode-setup.html)
- Set up "example WIFI connection" parameters
    - `idf.py menuconfig`
    - select `Example Connection Configuration  --->`
    - Enter your WiFi AP details
- Get a finnhub.io API key (it's free): https://finnhub.io/
    - `export FINNHUB_API_KEY=YOUR_KEY_GOES_HERE` (cmake will find it automatically during build)
    - (consider adding this to ~/.zshrc or ~/.bashrc)

## Build

Use ESP-IDF to build:

```
idf.py build
```

## Flashing

Use ESP-IDF to flash:

```
idf.py -p YOUR_SERIAL_PORT flash
```
