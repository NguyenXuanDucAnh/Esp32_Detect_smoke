
#define BLYNK_TEMPLATE_ID "TMPL6R8VX9Zjb"
#define BLYNK_TEMPLATE_NAME "DAIoT"
#define BLYNK_AUTH_TOKEN "Nmduxgi-ivWFDXNBaViEM97Ur2rGKik0"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>

// ===== WiFi + Blynk =====
char ssid[] = "Duc-Anh";
char pass[] = "0981231344";
char auth[] = BLYNK_AUTH_TOKEN;

// ===== Hardware =====
#define LED_PIN 21
#define ANALOG_PIN 4
int THRESHOLD = 1500;

// ===== Timer =====
BlynkTimer timer;

BlynkTimer timer_send_telegram;

// ===== Telegram Config =====
// const char* TELEGRAM_BOT_TOKEN = "7975724601:AAFNXsGemexZpX9pD8FFpLKSfuVjLlqwep0";
// const char* TELEGRAM_CHAT_ID   = "6329195890";

const char* TELEGRAM_BOT_TOKEN = "8559421132: AAEncPtwt0cJ_g4W0eo6Nh1Y8B6vHegIvNE";
const char* TELEGRAM_CHAT_ID   = "7540158588";

// Telegram requires HTTPS
WiFiClientSecure telegramClient;


// ===== Blynk Button: V0 manual LED control =====
BLYNK_WRITE(V0)
{
  int value = param.asInt();

  if (value == 1)
      digitalWrite(LED_PIN, HIGH);
  else
      digitalWrite(LED_PIN, LOW);
}

//
BLYNK_WRITE(V3)
{
  String value_str = param.asStr();

  Serial.print ("new value THRESHOLD:");
  Serial.println (value_str);

  int value = value_str.toInt();

  THRESHOLD = value;
  // if (value == 1)
  //     digitalWrite(LED_PIN, HIGH);
  // else
  //     digitalWrite(LED_PIN, LOW);
}


String urlencode(const String& str)
{
    String encoded = "";
    char c;
    char code0;
    char code1;

    for (int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);

        if (isalnum(c)) {
            encoded += c;
        } 
        else {
            code1 = (c & 0x0F) + '0';
            if ((c & 0x0F) > 9) {
                code1 = (c & 0x0F) - 10 + 'A';
            }

            code0 = ((c >> 4) & 0x0F) + '0';
            if (((c >> 4) & 0x0F) > 9) {
                code0 = ((c >> 4) & 0x0F) - 10 + 'A';
            }

            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }

    return encoded;
}


// ===== Hàm gửi cảnh báo Telegram =====
void sendTelegramAlert(int value)
{
    telegramClient.setInsecure();   // Bỏ kiểm tra chứng chỉ SSL

    if (!telegramClient.connect("api.telegram.org", 443)) {
        Serial.println("❌ Telegram connection failed");
        return;
    }

    String message = "⚠️ *CẢNH BÁO TỪ ESP32-S3*\n";
    message += "Giá trị ADC vượt ngưỡng!\n";
    message += "Giá trị: ";
    message += value;

    // Tạo URL gửi Telegram
    String url = String("/bot") + TELEGRAM_BOT_TOKEN + 
                 "/sendMessage?chat_id=" + TELEGRAM_CHAT_ID +
                 "&text=" + urlencode(message) +
                 "&parse_mode=Markdown";

    // Gửi HTTP request
    telegramClient.println("GET " + url + " HTTP/1.1");
    telegramClient.println("Host: api.telegram.org");
    telegramClient.println("Connection: close");
    telegramClient.println();

    Serial.println("📨 Đã gửi cảnh báo Telegram!");
}

bool let_send_telegram = false;
int value_adc_global = 0;

// ===== Đọc ADC và xử lý =====
void readAnalogAndProcess()
{
    int value = analogRead(ANALOG_PIN);
    value_adc_global = value;
    Serial.printf("ADC Value = %d\n", value);

    // Gửi về Blynk
    Blynk.virtualWrite(V1, value);

    if (value > THRESHOLD)
    {
        digitalWrite(LED_PIN, HIGH);

        Blynk.virtualWrite(V2, "[Warning] - Vượt ngưỡng an toàn !!!");
        Blynk.virtualWrite(V0, 1);

        let_send_telegram = true;   // ⚠ gửi TELEGRAM
    }
    else
    {
        digitalWrite(LED_PIN, LOW);

        Blynk.virtualWrite(V2, "Mức an toàn!!!");
        Blynk.virtualWrite(V0, 0);

        let_send_telegram = false; // không gửi telegram
    }
}


// ==== xử lý gửi dữ liệu cảnh báo lên telegram
int count_time = 7;
void Check_and_send_telegram_alert ()
{
  if (let_send_telegram == true)
  {
    sendTelegramAlert(value_adc_global);
  }
  else
  {
    count_time++;
    if (count_time > 10) // 10s gửi một lần
    {
      char buf[100];
      sprintf (buf, "%d", THRESHOLD);
      Blynk.virtualWrite(V3, buf);

      count_time = 0;
    }
  }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting...");

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Blynk.begin(auth, ssid, pass);

    timer.setInterval(200, readAnalogAndProcess);

    timer_send_telegram.setInterval(1000, Check_and_send_telegram_alert);
}

void loop()
{
    Blynk.run();
    timer.run();
    timer_send_telegram.run();
}

