# ESP32-S3 Camera Web Streaming

โปรเจค IP Camera โดยใช้ ESP32-S3 พร้อมกล้อง OV5640 ที่สามารถ Stream วิดีโอแบบ Real-time ผ่านเว็บเบราว์เซอร์

## ฟีเจอร์

- ✅ Real-time MJPEG Video Streaming
- ✅ WiFi Connection (2.4GHz)
- ✅ mDNS Support (http://camera01.local)
- ✅ LED Status Indicator
- ✅ Serial Debug Output
- ✅ Responsive Web Interface
- ✅ Auto Camera Configuration

## ฮาร์ดแวร์

- **บอร์ด**: CB008 ESP32-S3 CAM N16R8 OV5640
- **MCU**: Expressif ESP32-S3 WROOM
  - Flash: 16MB
  - PSRAM: 8MB
- **กล้อง**: OV5640 (5 Megapixel)
- **LED**: GPIO 2

## การติดตั้งและใช้งาน

### ขั้นตอนที่ 1: ตั้งค่า PlatformIO

```bash
# ติดตั้ง PlatformIO Core
pip install platformio

# Clone repository
git clone https://github.com/sorn-AI/ESP32_WEB_Camera.git
cd ESP32_WEB_Camera
```

### ขั้นตอนที่ 2: แก้ไขการตั้งค่า WiFi

แก้ไขไฟล์ `src/main.cpp` บรรทัด 8-9:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### ขั้นตอนที่ 3: Upload โค้ด

1. เชื่อมต่อบอร์ด ESP32-S3 กับคอมพิวเตอร์ผ่าน USB-UART
2. กดปุ่ม BOOT ค้างไว้
3. Upload:
   ```bash
   platformio run --target upload
   ```
4. ปล่อยปุ่ม BOOT เมื่อเห็น "Connecting..."

### ขั้นตอนที่ 4: ดู Serial Monitor

```bash
platformio device monitor
```

กดปุ่ม RESET แล้วจะเห็น IP Address

### ขั้นตอนที่ 5: เข้าถึงกล้อง

เปิดเว็บเบราว์เซอร์แล้วไปที่:
- `http://camera01.local` (macOS/iOS/Linux)
- หรือ `http://192.168.1.xxx` (IP Address จาก Serial Monitor)

## LED Status

| สถานะ LED | ความหมาย |
|-----------|----------|
| ติดค้าง | กำลังเริ่มระบบ |
| กระพริบ 1 วินาที | WiFi เชื่อมต่อสำเร็จ (ปกติ) |
| กระพริบ 0.5 วินาที 3 ครั้ง | WiFi เชื่อมต่อไม่สำเร็จ |
| กระพริบเร็ว 0.1 วินาที | กล้อง Initialize ล้มเหลว |

## โครงสร้างโปรเจค

```
ESP32_WEB_Camera/
├── src/
│   └── main.cpp           # โค้ดหลัก
├── platformio.ini         # การตั้งค่า PlatformIO
├── README.md             # เอกสารนี้
└── summary_project.txt   # สรุปโปรเจคและ Flowchart
```

## การตั้งค่ากล้อง

- **ความละเอียด**: UXGA (1600x1200) with PSRAM
- **คุณภาพ JPEG**: 10 (with PSRAM)
- **Frame Rate**: ขึ้นอยู่กับความเร็ว WiFi

## Troubleshooting

### Camera initialization failed (Error 0x105)
- ตรวจสอบ Camera Pin Configuration
- ตรวจสอบว่าบอร์ดเป็น CB008 ESP32-S3 CAM

### WiFi connection timeout
- ตรวจสอบ SSID และ Password
- ใช้ WiFi 2.4GHz เท่านั้น (ESP32 ไม่รองรับ 5GHz)

### Serial Monitor ไม่แสดงข้อความ
- ตรวจสอบว่าเชื่อมต่อ USB-UART ไม่ใช่ USB-OTG
- กดปุ่ม RESET บนบอร์ด

### mDNS ไม่ทำงาน
- บน Windows: ติดตั้ง [Bonjour Print Services](https://support.apple.com/kb/DL999)
- ใช้ IP Address แทน

## เอกสารเพิ่มเติม

ดูรายละเอียดเพิ่มเติมใน [summary_project.txt](summary_project.txt) ที่มี:
- Flowchart การทำงานระบบ
- ปัญหาที่พบและวิธีแก้ไข
- ข้อมูลทางเทคนิค

## License

MIT License - สามารถนำไปใช้งาน แก้ไข และแจกจ่ายได้อย่างอิสระ

## ผู้พัฒนา

- **ชื่อ**: Sorn
- **Email**: likit.tronics@gmail.com
- **GitHub**: [@sorn-AI](https://github.com/sorn-AI)
