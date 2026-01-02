#!/usr/bin/env python3
import serial
import time

try:
    # เปิด Serial port
    ser = serial.Serial('/dev/cu.usbmodem1101', 115200, timeout=1)
    print("Connected to ESP32-S3 Serial Monitor")
    print("=" * 50)

    # อ่านข้อมูล 10 วินาที
    start_time = time.time()
    while time.time() - start_time < 10:
        if ser.in_waiting > 0:
            data = ser.readline().decode('utf-8', errors='ignore').strip()
            if data:
                print(data)

    ser.close()
    print("=" * 50)
    print("Serial Monitor closed")

except Exception as e:
    print(f"Error: {e}")
    print("\nTips:")
    print("1. Make sure ESP32-S3 is connected")
    print("2. Try pressing RESET button on the board")
    print("3. Check if another program is using the port")
