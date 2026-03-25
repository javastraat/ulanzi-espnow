PORT=/dev/cu.usbserial-1130

echo "Waiting for $PORT to be available..."
while [ ! -e "$PORT" ]; do
  sleep 1
done
echo "Port found, flashing..."

esptool.py --chip esp32 --port "$PORT" --baud 115200 write_flash -z \
  0x1000 build/esp32.esp32.esp32/ulanzi-espnow.ino.bootloader.bin \
  0x8000 build/esp32.esp32.esp32/ulanzi-espnow.ino.partitions.bin \
  0xe000 build/esp32.esp32.esp32/boot_app0.bin \
  0x10000 build/esp32.esp32.esp32/ulanzi-espnow.ino.bin
