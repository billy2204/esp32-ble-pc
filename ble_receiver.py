import asyncio
import json
import logging
import sys
import time
from bleak import BleakScanner, BleakClient

DEVICE_NAME = "ESP32_Vibration"
CHARACTERISTIC_UUID = ""
BUFFER_LIMIT = 4096
LOG_FILE = "vibration_latency_log.json"

logging.basicConfig(
    filename=LOG_FILE,
    filemode='a',
    format='%(message)s',
    level=logging.INFO
)

def monotonic_millis():
    """Return monotonic time in milliseconds."""
    return int(time.monotonic() * 1000)

async def main():
    print("Scanning for BLE devices named ESP32_Vibration...")
    device = None
    devices = await BleakScanner.discover(timeout=10.0)
    for d in devices:
        if d.name == DEVICE_NAME:
            device = d
            break
    if not device:
        print("Device ESP32_Vibration not found. Exiting.")
        return

    print(f"Found device: {device.name} ({device.address}). Connecting...")

    buffer = ""
    offset = None  # offset = client_monotonic - esp32_millis at first timestamp packet

    def handle_notification(_, data):
        nonlocal buffer, offset
        try:
            chunk = data.decode('utf-8')
            buffer += chunk

            if len(buffer) > BUFFER_LIMIT:
                print(f"Buffer overflow: clearing buffer to avoid crash.")
                buffer = ""
                return

            while ';' in buffer:
                packet, buffer = buffer.split(';', 1)
                packet = packet.strip()
                if not packet:
                    continue
                try:
                    json_data = json.loads(packet)

                    # Timestamp packet for latency calculation
                    if "timestamp" in json_data and len(json_data) == 1:
                        esp32_ts = int(json_data["timestamp"])
                        client_now = monotonic_millis()

                        if offset is None:
                            offset = client_now - esp32_ts
                            print(f"Time offset between client and ESP32 clocks set to {offset} ms")

                        latency = client_now - (esp32_ts + offset)
                        if latency < 0:
                            # Handle possible wrap-around or clock issues gracefully
                            print(f"Warning: negative latency detected ({latency} ms), setting latency to 0")
                            latency = 0

                        print(f"Latency calculated: {latency} ms")

                        log_entry = {
                            "type": "latency",
                            "sent_timestamp_esp32": esp32_ts,
                            "client_monotonic_time": client_now,
                            "offset": offset,
                            "latency_ms": latency
                        }
                        logging.info(json.dumps(log_entry))

                    # Motors data packet
                    elif "motors" in json_data:
                        print(f"Received motors data: {json_data}")
                        logging.info(json.dumps({"type": "motors_data", "data": json_data}))

                    else:
                        print(f"Received unknown JSON packet: {json_data}")

                except json.JSONDecodeError as e:
                    print(f"JSON decode error: {e} in packet: {repr(packet)}")

        except Exception as ex:
            print(f"Error in notification handler: {ex}")

    try:
        async with BleakClient(device.address) as client:
            print("Connected. Subscribing to notifications...")
            await client.start_notify(CHARACTERISTIC_UUID, handle_notification)
            print("Notification handler started. Waiting for data...")
            while True:
                await asyncio.sleep(1)
    except Exception as e:
        print(f"Connection or runtime error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Program interrupted by user. Exiting.")
