#!/bin/bash
# Full recovery flash — writes bootloader + partition table + boot_app0 + firmware
# Use this to recover a bricked or fresh Ulanzi TC001.

export PATH="$HOME/.platformio/penv/bin:$PATH"
ESPTOOL="$HOME/.platformio/packages/tool-esptoolpy/esptool.py"
BOOT_APP0="$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'

echo "==============================="
echo "   Ulanzi ESP32 Flash Tool"
echo "==============================="
echo ""

# ── Select flash size ─────────────────────────────────────────────────────────
echo "Flash size:"
echo "  1) 8 MB  (default — TC001 has 8 MB)"
echo "  2) 4 MB"
echo "  0) Exit"
echo ""
while true; do
  read -p "Select [1-2] or 0 to exit: " FSIZE
  case "$FSIZE" in
    0) echo "Exiting."; exit 0 ;;
    1|"") ENV="ulanzi-tc001-8mb"; break ;;
    2) ENV="ulanzi-tc001-4mb"; break ;;
    *) echo "Invalid choice, try again." ;;
  esac
done

BUILD_DIR=".pio/build/${ENV}"

# Check build exists
if [ ! -f "${BUILD_DIR}/firmware.bin" ]; then
  echo -e "${RED}[ERROR]${NC} No build found for env '$ENV'."
  echo "Run: pio run -e $ENV"
  exit 1
fi

echo ""
echo -e "${BLUE}[INFO]${NC} Using env: $ENV"
echo -e "${BLUE}[INFO]${NC} Bootloader : ${BUILD_DIR}/bootloader.bin"
echo -e "${BLUE}[INFO]${NC} Partitions : ${BUILD_DIR}/partitions.bin"
echo -e "${BLUE}[INFO]${NC} boot_app0  : $BOOT_APP0"
echo -e "${BLUE}[INFO]${NC} Firmware   : ${BUILD_DIR}/firmware.bin"
echo ""

# ── Select serial port ────────────────────────────────────────────────────────
echo "Scanning for serial ports..."
PORTS=($(ls /dev/cu.usb* /dev/cu.SLAB* /dev/cu.wch* 2>/dev/null))

if [ ${#PORTS[@]} -eq 0 ]; then
  echo -e "${YELLOW}[WAIT]${NC} No serial ports found. Connect your Ulanzi and press Enter to retry (or Ctrl+C to exit)."
  while [ ${#PORTS[@]} -eq 0 ]; do
    read -p ""
    PORTS=($(ls /dev/cu.usb* /dev/cu.SLAB* /dev/cu.wch* 2>/dev/null))
  done
fi

echo ""
echo "Available serial ports:"
for i in "${!PORTS[@]}"; do
  echo "  $((i+1))) ${PORTS[$i]}"
done
echo "  0) Exit"
echo ""

while true; do
  read -p "Select port [1-${#PORTS[@]}] or 0 to exit: " CHOICE
  case "$CHOICE" in
    0) echo "Exiting."; exit 0 ;;
    '') continue ;;
    *)
      if [[ "$CHOICE" =~ ^[0-9]+$ ]] && [ "$CHOICE" -ge 1 ] && [ "$CHOICE" -le "${#PORTS[@]}" ]; then
        PORT="${PORTS[$((CHOICE-1))]}"; break
      fi
      echo "Invalid choice, try again." ;;
  esac
done

echo ""
echo -e "${BLUE}[INFO]${NC} Selected: $PORT"
echo "Waiting for device to be ready..."
while [ ! -e "$PORT" ]; do sleep 1; done
echo -e "${GREEN}[OK]${NC} Device ready! Starting full flash..."
echo ""

# ── Flash ─────────────────────────────────────────────────────────────────────
python3 "$ESPTOOL" --chip esp32 --port "$PORT" --baud 921600 \
  --before default_reset --after hard_reset \
  write_flash -z \
  0x1000  "${BUILD_DIR}/bootloader.bin" \
  0x8000  "${BUILD_DIR}/partitions.bin" \
  0xe000  "$BOOT_APP0" \
  0x10000 "${BUILD_DIR}/firmware.bin"

if [ $? -eq 0 ]; then
  echo ""
  echo -e "${GREEN}[SUCCESS]${NC} Flash complete! Device is rebooting."
else
  echo ""
  echo -e "${RED}[FAILED]${NC} Flash failed. Check connection and try again."
  echo "Tip: hold the BOOT button on the ESP32 while connecting, then retry."
  exit 1
fi
