#include "esp_camera.h"
#include <WiFi.h>

// Configuration de la caméra
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char* ssid = "WiFiCyclope"; // Remplacez par votre SSID WiFi
const char* password = "azerty123"; // Remplacez par votre mot de passe WiFi

// Déclaration du serveur web
WiFiServer server(80);

void startCameraServer() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Attendre que le client envoie une requête
  while (!client.available()) {
    delay(1);
  }

  // Lire la requête HTTP
  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("GET /") >= 0) {  // Toujours envoyer le MJPEG, sans HTML
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println();

    while (client.connected()) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Échec de la capture d'image");
        break;
      }

      client.printf("--frame\r\n");
      client.printf("Content-Type: image/jpeg\r\n");
      client.printf("Content-Length: %u\r\n\r\n", fb->len);
      client.write(fb->buf, fb->len);
      client.printf("\r\n");

      esp_camera_fb_return(fb);
      delay(50); // Ajuste pour contrôler le taux d'images par seconde
    }
  }

  client.stop();
}


void setup() {
  Serial.begin(115200);
  Serial.println();

  // Configuration de la caméra
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA; // Résolution 640x480
    config.jpeg_quality = 10; // Qualité JPEG
    config.fb_count = 2; // Nombre de frame buffers
  } else {
    config.frame_size = FRAMESIZE_VGA; // Résolution 640x480
    config.jpeg_quality = 12; // Qualité JPEG réduite
    config.fb_count = 1; // Un seul frame buffer
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Connexion au WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Démarrage du serveur web
  server.begin();
  Serial.println("Camera ready! Visit:");
  Serial.println(WiFi.localIP());
}

void loop() {
  startCameraServer();
}
