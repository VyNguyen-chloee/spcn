#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include< Hinh_anh.h>// thứ viện chứa các ảnh cần hiển thị

#include <PZEM004Tv30.h>
// khai báo các chân
#define TFT_CS     15
#define TFT_RST    2 // chân Reset cắm vào GPIO2
#define TFT_DC     4 //chân A0 cắm vào GPIO4
// clk cắm vào gpio 18 (có sẵn)
// mosi 23 (mặc định)

// Kích thước màn hình
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160
// Khởi tạo màn hình (SPI)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Khai báo UART2 trên ESP32
// RX2 = GPIO16, TX2 = GPIO17 (bạn có thể đổi sang chân khác được)
PZEM004Tv30 pzem(Serial2, 16, 17);

// Thông tin Wi-Fi
const char* ssid = "PhuongQuang";
const char* password = "phuong188";

// Link Firebase 
const char* firebaseHost = "https://phuongvy-spcn-default-rtdb.asia-southeast1.firebasedatabase.app";

unsigned long lastCheck = 0;
bool wifiConnected = false;

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("🔌 Đang kết nối WiFi");
  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n✅ Kết nối WiFi thành công, IP: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println("\n❌ Kết nối WiFi thất bại!");
    wifiConnected = false;
  }
}
 

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);        // UART2 tốc độ mặc định của PZEM-004T
  Serial.println("🔌 ESP32 đọc PZEM-004T qua UART2...");
// Khởi động màn hình
  tft.initR(INITR_BLACKTAB);   // Khởi tạo loại ST7735S
  tft.setRotation(1);         // Xoay ngang
  tft.fillScreen(ST77XX_BLACK); // Nền đen

}



// Hàm gửi dữ liệu lên Firebase
void sendData(const String& path, const String& data, bool isNumber = false) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(firebaseHost) + "/" + path + ".json";
    Serial.print("📤 Gửi tới: ");
    Serial.println(url);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String payload = isNumber ? data : "\"" + data + "\"";

    int httpResponseCode = http.PUT(payload);

    if (httpResponseCode > 0) {
      Serial.print("✅ Gửi thành công, code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("❌ Gửi thất bại, lỗi: ");
      Serial.println(http.errorToString(httpResponseCode)); // Không cần .c_str()
    }

    http.end();
  } else {
    Serial.println("⚠️ Không gửi được, chưa kết nối WiFi!");
  }
}

//// Hàm đọc giá trị từ Firebase
bool docDuLieuFirebase(const char* path, double* outValue) {
      HTTPClient http;
      String url = String(firebaseHost) + String(path);
      http.begin(url);
      int httpCode = http.GET();
      if (httpCode == 200) {
        String payload = http.getString();
        String filtered = "";
        for (char c : payload) if ((c >= '0' && c <= '9') || c == '.') filtered += c;
        *outValue = filtered.toFloat();
        http.end();
        return true;
      }
        else {
        http.end();
        return false;
      }
    }



void loop() {
//// Kiểm tra lại trạng thái WiFi mỗi 5 giây
  if (millis() - lastCheck > 1000) {
    lastCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ Mất kết nối WiFi! Đang thử kết nối lại...");
      wifiConnected = false;
      connectWiFi();
    } else if (!wifiConnected) {
      Serial.println("✅ Đã kết nối lại WiFi!");
      wifiConnected = true;
    }
  //// Đọc số liệu từ pzem
    // Đọc điện áp
      float voltage = pzem.voltage();
      if (!isnan(voltage)) {
        Serial.print("Điện áp (V): ");
        Serial.println(voltage);
      } else {
        Serial.println("⚠️ Lỗi đọc điện áp");
      }

    // Đọc dòng điện
      float current = pzem.current();
      if (!isnan(current)) {
        Serial.print("Dòng điện (A): ");
        Serial.println(current);
      } else {
        Serial.println("⚠️ Lỗi đọc dòng điện");
      }

    // Đọc công suất
      float power = pzem.power();
      if (!isnan(power)) {
        Serial.print("Công suất (W): ");
        Serial.println(power);
      } else {
        Serial.println("⚠️ Lỗi đọc công suất");
      }

    // Đọc điện năng tiêu thụ
      float energy = pzem.energy();
      if (!isnan(energy)) {
        Serial.print("Điện năng (kWh): ");
        Serial.println(energy);
      } else {
        Serial.println("⚠️ Lỗi đọc điện năng");
      }

    // Đọc tần số
      float frequency = pzem.frequency();
      if (!isnan(frequency)) {
        Serial.print("Tần số (Hz): ");
        Serial.println(frequency);
      } else {
        Serial.println("⚠️ Lỗi đọc tần số");
      }

    // Đọc hệ số công suất
      float pf = pzem.pf();
      if (!isnan(pf)) {
        Serial.print("Hệ số công suất (PF): ");
        Serial.println(pf);
      } else {
        Serial.println("⚠️ Lỗi đọc PF");
      }

  //   // Gửi dữ liệu mỗi 10 giây
      static unsigned long lastSend = 0;
      if (millis() - lastSend > 10000 && WiFi.status() == WL_CONNECTED) {
          lastSend = millis();
        
          sendData("gia_tri_cam_bien/Current", String(current), true);
          sendData("gia_tri_cam_bien/Power", String(power), true);
          sendData("gia_tri_cam_bien/Voltage", String(voltage), true);
          sendData("gia_tri_cam_bien/Energy", String(energy), true);
          sendData("gia_tri_cam_bien/Power_factor", String(pf), true);
          sendData("gia_tri_cam_bien/Frequency", String(frequency), true);
        }
    
    
  // Gọi hàm đọc dữ liệu từ Firebase
      double Current_max;
      docDuLieuFirebase("/gia_tri_cam_bien/Current_max.json", &Current_max);
      Serial.print("Current max =");
      Serial.print(Current_max);
      Serial.println("A");

      double Voltage_min;
      docDuLieuFirebase("/gia_tri_cam_bien/Voltage_min.json", &Voltage_min);
      Serial.print("Voltage min =");
      Serial.print(Voltage_min);
      Serial.println("V");
      
// In ra màn hình Oled
  // Xóa màn hình trước khi in lại số liệu
  tft.fillScreen(ST77XX_BLACK);

  // Đặt màu chữ và size
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  tft.setCursor(0, 0);
  tft.print("Voltage: ");
  tft.print(voltage);
  tft.println(" V");

  tft.setCursor(0, 20);
  tft.print("Current: ");
  tft.print(current);
  tft.println(" A");

  tft.setCursor(0, 40);
  tft.print("Power: ");
  tft.print(power);
  tft.println(" W");

  tft.setCursor(0, 60);
  tft.print("Energy: ");
  tft.print(energy);
  tft.println(" kWh");

  }

}
