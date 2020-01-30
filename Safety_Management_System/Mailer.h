/*
    ESP32 E-mail Library
    Author:  kerikun11 (Github: kerikun11)
    Date:    2017.04.08

    参考： https://kerikeri.top/posts/2017-04-08-esp32-mail/
*/
#pragma once

#include <WiFiClientSecure.h>
#include <base64.h>

class Mailer {
 public:
  Mailer(const char* username, const char* password, const char* from_address,
         const int smtp_port = 465,
         const char* smtp_hostname = "smtp.gmail.com")
      : username(username),
        password(password),
        from_address(from_address),
        smtp_port(smtp_port),
        smtp_hostname(smtp_hostname) {}

  bool send(const String& to_address, const String& subject,
            const String& content) {
    WiFiClientSecure client;
    log_d("Connecting to %s\n", smtp_hostname);
    if (!client.connect(smtp_hostname, smtp_port)) {
      log_e("Could not connect to mail server");
      return false;
    }
    if (!readResponse(client, "220")) {
      log_e("Connection Error");
      return false;
    }
    client.println("HELO friend");
    if (!readResponse(client, "250")) {
      log_e("identification error");
      return false;
    }
    client.println("AUTH LOGIN");
    if (!readResponse(client, "334")) {
      log_e("AUTH LOGIN failed");
      return false;
    }
    client.println(base64::encode(username));
    if (!readResponse(client, "334")) {
      log_e("AUTH LOGIN failed");
      return false;
    }
    client.println(base64::encode(password));
    if (!readResponse(client, "235")) {
      log_e("SMTP AUTH error");
      return false;
    }
    client.println("MAIL FROM: <" + String(from_address) + '>');
    if (!readResponse(client, "250")) {
      log_e("MAIL FROM failed");
      return false;
    }
    client.println("RCPT TO: <" + to_address + '>');
    if (!readResponse(client, "250")) {
      log_e("RCPT TO failed");
      return false;
    }
    client.println("DATA");
    if (!readResponse(client, "354")) {
      log_e("SMTP DATA error");
      return false;
    }
    client.println("From: <" + String(from_address) + ">");
    delay(100);
    client.println("To: <" + to_address + ">");
    delay(100);
    client.println("Subject: " + subject);
    delay(100);
    client.println("Mime-Version: 1.0");
    delay(100);
    client.println("Content-Type: text/html");
    delay(100);
    client.println();
    delay(100);
    client.println(content);
    delay(100);
    client.println(".");
    if (!readResponse(client, "250")) {
      log_e("Sending message error");
      return false;
    }
    client.println("QUIT");
    if (!readResponse(client, "221")) {
      log_e("QUIT failed");
      return false;
    }
    log_i("Sending E-mail Successful");
    return true;
  }

 private:
  const char* username;
  const char* password;
  const char* from_address;
  const int smtp_port;
  const char* smtp_hostname;

  bool readResponse(WiFiClientSecure& client, const String& target,
                    uint32_t timeout_ms = 3000) {
    uint32_t timeStamp = millis();
    while (1) {
      if (client.available()) break;
      if (millis() > timeStamp + timeout_ms) {
        log_e("SMTP Response TIMEOUT!");
        return false;
      }
      delay(1);
    }
    String res = client.readStringUntil('\n');
    res.trim();
    log_d("Response: %s", res.c_str());
    if (target != "" && res.indexOf(target) == -1) return false;
    return true;
  }
};
