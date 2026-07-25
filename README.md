# AtomClock

ESP32-based atomic clock project for M5Stack Atom.

## Setup

1. Install [uv](https://docs.astral.sh/uv/) (manages the Python used by the build script):
   ```bash
   curl -LsSf https://astral.sh/uv/install.sh | sh
   ```
   This project is pinned to Python 3.12 via `.python-version`; `uv` will download it automatically on first build.

2. Copy `.env.example` to `.env`:
   ```bash
   cp .env.example .env
   ```

3. Edit `.env` and set your WiFi credentials:
   ```
   WIFI_SSID=YourWiFiNetworkName
   WIFI_PASSWORD=YourWiFiPassword
   ```

4. Build and upload:
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

The Python script `load_env.py` handles loading these during the build process. PlatformIO invokes it as `uv run --python 3.12 load_env.py`, so no system Python or manual venv setup is required — `uv` provisions the interpreter itself.

## Security

The `.env` file is ignored by git to prevent credentials from being committed to the repository.
