#include <WiFi.h>
#include <HTTPClient.h>

// Thông tin Wi-Fi
const char* ssid = "PhuongQuang";
const char* password = "phuong188";

// Link Firebase (không có dấu / ở cuối)
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
  // Kiểm tra lại trạng thái WiFi mỗi 5 giây
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
    
  }

  //   // Gửi dữ liệu mỗi 10 giây
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 10000 && WiFi.status() == WL_CONNECTED) {
        lastSend = millis();
        float current = random(0, 100) / 1.0;
        float power   = random(0, 1000) / 1.0;
        float voltage = random(200, 240) / 1.0;
        float energy  = random(0, 500) / 1.0;
        float power_factor  = random(0, 500) / 1.0;

        sendData("gia_tri_cam_bien/Current", String(current), true);
        sendData("gia_tri_cam_bien/Power", String(power), true);
        sendData("gia_tri_cam_bien/Voltage", String(voltage), true);
        sendData("gia_tri_cam_bien/Energy", String(energy), true);
        sendData("gia_tri_cam_bien/Power_factor", String(power_factor), true);
      }
  
}

