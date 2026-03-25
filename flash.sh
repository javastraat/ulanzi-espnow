#!/bin/bash

echo "==============================="
echo "   Ulanzi ESP32 Flash Tool"
echo "==============================="
echo ""

# Scan for available serial ports
PORTS=($(ls /dev/cu.usb* /dev/cu.SLAB* /dev/cu.wch* 2>/dev/null))

if [ ${#PORTS[@]} -eq 0 ]; then
  echo "No serial ports found. Please connect your device and re-run."
  exit 1
fi

echo "Available serial ports:"
for i in "${!PORTS[@]}"; do
  echo "  $((i+1))) ${PORTS[$i]}"
done
echo "  0) Exit"
echo ""

# Ask user to pick a port
while true; do
  read -p "Select port [1-${#PORTS[@]}] or 0 to exit: " CHOICE
  if [ "$CHOICE" = "0" ]; then
    echo "Exiting."
    exit 0
  fi
  if [[ "$CHOICE" =~ ^[0-9]+$ ]] && [ "$CHOICE" -ge 1 ] && [ "$CHOICE" -le "${#PORTS[@]}" ]; then
    PORT="${PORTS[$((CHOICE-1))]}"
    break
  fi
  echo "Invalid choice, try again."
done

echo ""
echo "Selected: $PORT"
echo "Waiting for device to be ready..."

# Wait until the port is available (in case it was just selected but not yet ready)
while [ ! -e "$PORT" ]; do
  sleep 1
done

echo "Device ready! Starting flash..."
echo ""

esptool.py --chip esp32 --port "$PORT" --baud 115200 write_flash -z \
  0x1000 build/esp32.esp32.esp32/ulanzi-espnow.ino.bootloader.bin \
  0x8000 build/esp32.esp32.esp32/ulanzi-espnow.ino.partitions.bin \
  0xe000 build/esp32.esp32.esp32/boot_app0.bin \
  0x10000 build/esp32.esp32.esp32/ulanzi-espnow.ino.bin

if [ $? -eq 0 ]; then
  echo ""
  echo "Flash complete!"
else
  echo ""
  echo "Flash failed. Check connection and try again."
fi
