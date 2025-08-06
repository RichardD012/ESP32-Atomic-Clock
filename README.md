# AtomClock

ESP32-based atomic clock project for M5Stack Atom.

## Setup

1. Copy `.env.example` to `.env`:
   ```bash
   cp .env.example .env
   ```

2. Edit `.env` and set your WiFi credentials:
   ```
   WIFI_SSID=YourWiFiNetworkName
   WIFI_PASSWORD=YourWiFiPassword
   ```

3. Build and upload:
   ```bash
   pio run --target upload
   ```

## Environment Variables

The project requires these environment variables to be set:

- `WIFI_SSID`: Your WiFi network name
- `WIFI_PASSWORD`: Your WiFi password

The build process will automatically read credentials from:
1. `.env` file (recommended for development)
2. System environment variables (as fallback)

The Python script `load_env.py` handles loading these during the build process.

## Security

The `.env` file is ignored by git to prevent credentials from being committed to the repository.
