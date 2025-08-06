#!/usr/bin/env python3

import os
import sys

def load_env_file(env_file_path=".env"):
    """Load environment variables from .env file"""
    env_vars = {}
    
    if not os.path.exists(env_file_path):
        return env_vars
    
    try:
        with open(env_file_path, 'r') as file:
            for line in file:
                line = line.strip()
                
                # Skip empty lines and comments
                if not line or line.startswith('#'):
                    continue
                
                # Parse KEY=VALUE pairs
                if '=' in line:
                    key, value = line.split('=', 1)
                    key = key.strip()
                    value = value.strip()
                    
                    # Remove quotes if present
                    if (value.startswith('"') and value.endswith('"')) or \
                       (value.startswith("'") and value.endswith("'")):
                        value = value[1:-1]
                    
                    env_vars[key] = value
        
    except Exception as e:
        print(f"Error reading {env_file_path}: {e}", file=sys.stderr)
        sys.exit(1)
    
    return env_vars

def get_wifi_credentials():
    """Get WiFi credentials from .env file or environment variables"""
    # First try to load from .env file
    env_vars = load_env_file()
    
    # Get WIFI_SSID
    wifi_ssid = env_vars.get('WIFI_SSID') or os.environ.get('WIFI_SSID')
    if not wifi_ssid:
        print("Error: WIFI_SSID not found in .env file or environment variables", file=sys.stderr)
        sys.exit(1)
    
    # Get WIFI_PASSWORD  
    wifi_password = env_vars.get('WIFI_PASSWORD') or os.environ.get('WIFI_PASSWORD')
    if not wifi_password:
        print("Error: WIFI_PASSWORD not found in .env file or environment variables", file=sys.stderr)
        sys.exit(1)
    
    return wifi_ssid, wifi_password

if __name__ == "__main__":
    ssid, password = get_wifi_credentials()
    
    # Output build flags for PlatformIO - ONLY output the flags, no other text to stdout
    print(f'-DWIFI_SSID=\\"{ssid}\\"')
    print(f'-DWIFI_PASSWORD=\\"{password}\\"')