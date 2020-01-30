#include <WiFi.h>
#include <HTTPClient.h>
#include "Mailer.h"
#define USE_ARDUINO_INTERRUPTS true
#include "DHT12.h"
#include <Wire.h>     //The DHT12 uses I2C comunication.
#include <ArduinoJson.h>
#include <M5Stack.h>
#include <PulseSensorPlayground.h>
#include "Ambient.h"
#include <ezTime.h>

const int OUTPUT_TYPE = SERIAL_PLOTTER;

DHT12 dht12;          //Preset scale CELSIUS and ID 0x5c.

const int PIN_INPUT = 36;
PulseSensorPlayground pulseSensor;

const int THRESHOLD = 550;   // Adjust this number to avoid noise when idle


WiFiClient client;
Ambient ambient;
 
const char* ssid = "zero2";
const char* password = "0548872871";
const char* smtp_username = "atuya.god";
const char* smtp_password = "1919atuya";
const char* smtp_from_address = "atuya.god@gmail.com";
const int smtp_port = 465;
const char* smtp_hostname = "smtp.gmail.com";

const char* to_address = "zero2despair@gmail.com";
const char* subject = "温湿度情報";
const char* subject1 = "消費カロリー情報";

const int temperature_upper_limit = 31;
const int temperature_upper_1 = 28;
const int temperature_upper_2 = 25;
const int temperature_lower_limit = 15;
const int minimum_email_interval_seconds = 60*20;   //20 minutes
const int cal_email_interval_seconds = 60*60;   //60 minutes
const int sensing_interval_milliseconds = 5000;       //should be more than 1500

unsigned int channelId = 16017; // AmbientのチャネルID
unsigned int t=0;
unsigned int zikan=1;
unsigned int karo=0;
unsigned int f=0;
const char* writeKey = "c38cea4d13bccb82"; // ライトキー

Mailer mail(smtp_username, smtp_password, smtp_from_address, smtp_port,smtp_hostname);
Timezone Tokyo;
time_t last_emailed_at;
time_t last_emailed_at1;
void send_email(const String content);
void send_email1(const String content);

// 画像を入れるフォルダ名
const char* pictureFolder = "/img/";

// 静岡県中部の天気予報を取得
const char* endpoint = "https://www.drk7.jp/weather/json/22.js";
const char* region = "中部";
DynamicJsonDocument weatherInfo(20000);
 
void setup() {
    M5.begin();
    dacWrite(25, 0); // Speaker OFF
    Wire.begin();
    M5.Lcd.setBrightness(100);
    Serial.begin(115200);
    WiFi.begin(ssid, password);
     
  //connect to WiFi
  M5.Lcd.print("Connecting to YOUR_SSID ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      M5.Lcd.print(".");
  }
  M5.Lcd.println(" CONNECTED");
  delay(2000);
  M5.Lcd.clear();

  

  
    Serial.println("Connected to the WiFi network");
    weatherInfo = getJson();
    //WiFi.disconnect();
    drawTodayWeather(); 

Wire.begin();

    if (!pulseSensor.begin()) {
        Serial.println("PulseSensor.begin: failed");
        for(;;) {
            delay(0);
        }
    }


  Tokyo.setLocation("Asia/Tokyo");
  Serial.println("Asia/Tokyo time: " + Tokyo.dateTime());
  last_emailed_at = Tokyo.now();
  

//  M5.Lcd.println(WiFi.localIP());
  ambient.begin(channelId, writeKey, &client); // チャネルIDとライトキーを指定してAmbientの初期化

    
}
 
void loop() {

  //float temp,humid;
  float temp,humid;
  
  //int sin;

  
  M5.Lcd.setCursor(0,0);
  //M5.Lcd.println("**** Send temp/humid to Ambient ****");
      M5.Lcd.setCursor(0, 0);
      //IP表示
  //M5.Lcd.println(WiFi.localIP());     

  //Read pulce 
  //  M5.Lcd.print("pluce: ");
  //  sin=pulseSensor.getLatestSample();

  //  M5.Lcd.print(sin);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(100, 170);
  M5.Lcd.printf(" %d", pulseSensor.getBeatsPerMinute()-3);
        
M5.Lcd.setTextSize(1);

  //Read temperature with preset scale.
      
     
  M5.Lcd.drawString(" ",220,170);
  M5.Lcd.setTextSize(2);
  temp=dht12.readTemperature();
  //M5.Lcd.drawFloat(temp,-1,220,170);
  M5.Lcd.setCursor(220, 170);
  M5.Lcd.println  ((int)temp);
  M5.Lcd.setCursor(240, 170);
  M5.Lcd.print(" *C ");
  //M5.Lcd.print(" C ");
  //Read humidity.

  M5.Lcd.setCursor(220, 190);
  humid=dht12.readHumidity();
  M5.Lcd.println((int)humid);
  M5.Lcd.setCursor(240, 190);
  M5.Lcd.println(" H");
  M5.Lcd.setTextSize(1);


 // 経過時間を計算しそれが一定以上、かつ温度が閾値を超えていたらメール送信
  auto elapsed_seconds = difftime(Tokyo.now(), last_emailed_at);

  String email_content = "温度: ";
  email_content += temp;
  email_content += "℃、湿度: ";
  email_content += humid;
  email_content += "％です。\n";
  Serial.println(email_content);

  log_d("経過時間：%.1f秒", elapsed_seconds);
  if (elapsed_seconds > minimum_email_interval_seconds) {
    if (temp > temperature_upper_limit) {
      send_email(email_content + "非常に危険な気温です！！この気温下での運動はお薦め出来ません。どうしても運動したい場合は、こまめな水分補給を絶対に忘れないでください。水分・塩分補給の目安は10分に一度です。また自分の運動のメニューが済み次第、速やかに家に帰りましょう。");
    } else if (temp > temperature_upper_1) {
      send_email(email_content + "危険な気温です。熱中症になる危険が高いです。運動する場合は、こまめな水分を忘れないでください。水分・塩分補給の目安は10分～20分に一度です。");
    } else if (temp > temperature_upper_2) {
      send_email(email_content + "熱中症になる危険が高い気温です。積極的に日陰で休憩を取りましょう。ジョギングや球技といった激しい運動は、30分に一度休憩を取って体を休めましょう。また小さなお子さんは、地表熱からの熱の影響をとても受けやすく、熱中症になりやすいです。もし一緒に運動している場合は、顔色をよく見てあげましょう。");
    } else if (temp < temperature_lower_limit) {
      send_email(email_content + "今日は少し寒い日です。運動をする場合はしっかりとストレッチをしてから運動することをお薦めします。動きながらストレッチを行う、ダイナミックストレッチが筋肉に効きます。また寒い日の運動はチャンスです。運動を行い基礎代謝を高めて、冷えやすい体質を改善出来ますし、冬はエネルギー消費量も増えるのでダイエットにも効果的です。");
    }
  }

 // 経過時間を計算しそれが一定以上、かつ温度が閾値を超えていたらメール送信
  auto cal_seconds = difftime(Tokyo.now(), last_emailed_at1);

  String email1_content = "現在：";
  email1_content += zikan;
  email1_content += "時間運動をしています。";
    email1_content += "消費カロリーは、約：";
  Serial.println(email1_content);

  log_d("経過時間：%.1f秒", cal_seconds);
  if (cal_seconds > cal_email_interval_seconds) {
      
      karo=zikan*150;
      email1_content += karo;
      zikan += 1;
      send_email1(email1_content += "kcalです。");
    }
  
  // DHT11 sampling rate is 1HZ.
  //delay(sensing_interval_milliseconds);
  //500が約１分
  if(f==500 * 1){
      ambient.set(1,String(temp).c_str());
      ambient.set(2,String(humid).c_str());
      ambient.set(3, pulseSensor.getBeatsPerMinute()-3);
      ambient.send();
      //M5.Lcd.println("Data sended to Ambient");
      //M5.Lcd.printf("%d\n",t);
      f=0;
  }
  
  if(t>5){
    t=0;
    M5.Lcd.setTextColor(BLACK);
    M5.Lcd.fillRect(220, 160, 300, 200, BLACK);
    M5.Lcd.fillRect(100, 160, 50, 40, BLACK);
  }else{
    t+=1;
    f+=1;
    M5.Lcd.setTextColor(WHITE);
  }
  delay(4);
  //M5.Lcd.clear();

  
    //delay(1);
    if (M5.BtnA.wasPressed()) {
        drawTodayWeather(); 
    }
    if (M5.BtnB.wasPressed()) {
        drawTomorrowWeather();
    }
    if (M5.BtnC.wasPressed()) {
        drawDayAfterTomorrowWeather();
    }

    
    M5.update();

    
}

//データ送信時に時間表示
void send_email(const String content) {
  mail.send(to_address, subject, content);

  M5.Lcd.println("");
//  M5.Lcd.println("Emailed at ");
//  M5.Lcd.println(Tokyo.dateTime());

  last_emailed_at = Tokyo.now();
}

void send_email1(const String content) {
  mail.send(to_address, subject1, content);

  M5.Lcd.println("");
//  M5.Lcd.println("Emailed at ");
//  M5.Lcd.println(Tokyo.dateTime());

  last_emailed_at1 = Tokyo.now();
}


DynamicJsonDocument getJson() {
    DynamicJsonDocument doc(20000);
  
    if ((WiFi.status() == WL_CONNECTED)) {
        HTTPClient http;
        http.begin(endpoint);
        int httpCode = http.GET();
        if (httpCode > 0) {
            //jsonオブジェクトの作成
            String jsonString = createJson(http.getString());
            deserializeJson(doc, jsonString);
        } else {
            Serial.println("Error on HTTP request");
        }
        http.end(); //リソースを解放
    }
    return doc;
}

// JSONP形式からJSON形式に変える
String createJson(String jsonString){
    jsonString.replace("drk7jpweather.callback(","");
    return jsonString.substring(0,jsonString.length()-5);
}

void drawTodayWeather() {
    String today = weatherInfo["pref"]["area"][region]["info"][0];
    drawWeather(today);
   
}

void drawTomorrowWeather() {
    String tomorrow = weatherInfo["pref"]["area"][region]["info"][1];
    drawWeather(tomorrow);
    
}

void drawDayAfterTomorrowWeather() {
    String dayAfterTomorrow = weatherInfo["pref"]["area"][region]["info"][2];
    drawWeather(dayAfterTomorrow);
    
}

void drawWeather(String infoWeather) {
    M5.Lcd.clear();
    DynamicJsonDocument doc(20000);
    deserializeJson(doc, infoWeather);
    String weather = doc["weather"];
    String filename = "";
    if (weather.indexOf("雨") != -1) {
        if (weather.indexOf("くもり") != -1) {
            filename = "rainyandcloudy.jpg";
        } else {
            filename = "rainy.jpg";
        }
    } else if (weather.indexOf("晴") != -1) {
        if (weather.indexOf("くもり") != -1) {
            filename = "sunnyandcloudy.jpg";
        } else {
            filename = "sunny.jpg";
        }
    } else if (weather.indexOf("雪") != -1) {
        filename = "snow.jpg";
    } else if (weather.indexOf("くもり") != -1) {
        filename = "cloudy.jpg";
    }
  
    if (filename.equals("")){ 
        return;
    }
  
    String filePath = pictureFolder+filename;
    M5.Lcd.drawJpgFile(SD,filePath.c_str());
  
    String maxTemperature = doc["temperature"]["range"][0]["content"];
    String minTemperature = doc["temperature"]["range"][1]["content"];
    drawTemperature(maxTemperature, minTemperature);
  
    String railfallchance0_6 = doc["rainfallchance"]["period"][0]["content"];
    String railfallchance6_12 = doc["rainfallchance"]["period"][1]["content"];
    String railfallchance12_18 = doc["rainfallchance"]["period"][2]["content"];
    String railfallchance18_24 = doc["rainfallchance"]["period"][3]["content"];
    drawRainfallChancce(railfallchance0_6, railfallchance6_12, railfallchance12_18, railfallchance18_24);
  
    drawDate(doc["date"]);

}

void drawTemperature(String maxTemperature, String minTemperature) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(150,40);
    M5.Lcd.print(maxTemperature);
  
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(175,40);
    M5.Lcd.print("|");
  
    M5.Lcd.setTextColor(BLUE);
    M5.Lcd.setCursor(200,40);
    M5.Lcd.print(minTemperature);
}

void drawRainfallChancce(String rfc0_6, String rfc6_12, String rfc12_18, String rfc18_24) {
    M5.Lcd.setTextSize(2);  //下のアレ
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(145,110);
    M5.Lcd.print(rfc0_6);
    
    M5.Lcd.setCursor(185,110);
    M5.Lcd.print(rfc6_12);
  
    M5.Lcd.setCursor(228,110);
    M5.Lcd.print(rfc12_18);
  
    M5.Lcd.setCursor(273,110);
    M5.Lcd.print(rfc18_24);
}

void drawDate(String date) {
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);    //さわるな
    M5.Lcd.setCursor(150,10);
    M5.Lcd.print(date);
}
