/*
 * IoT Automated Duckweed Harvesting System
 * ESP32-CAM Monitoring Controller
 *
 * Responsibilities:
 * - Connect to Wi-Fi
 * - Capture grayscale image
 * - Estimate duckweed coverage
 * - Send coverage to Flask server
 * - Send coverage to Arduino
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <SoftwareSerial.h>
#include <HTTPClient.h>

// AI Thinker ESP32-CAM Pin Configuration

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


// Wi-Fi Configuration

const char *wifiList[][2] = {
//Replace this value locally before uploading to ESP32!!!
    {"YOUR_WIFI_SSID_1", "YOUR_WIFI_PASSWORD_1"}, 
    {"YOUR_WIFI_SSID_2", "YOUR_WIFI_PASSWORD_2"}

};

// Server Configuration

const char *serverUrl =
    "http://YOUR_SERVER_ADDRESS/update"; //Replace w your server address here

// Communication with Arduino

SoftwareSerial mySerial(3, 1);

// Setup

void setup() {

    Serial.begin(9600);

    mySerial.begin(9600);

    Serial.println();
    Serial.println("Starting ESP32-CAM...");

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

    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;

    // Grayscale is used for simpler threshold processing
    config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // Initialize ESP32 camera
    esp_err_t err = esp_camera_init(&config);

    if (err != ESP_OK) {
        Serial.printf(
            "Camera initialization failed with error 0x%x",
            err
        );
        return;
    }
    connectToWiFi();
}

// Main Loop

void loop() {
    checkWiFi();
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
  
    // Threshold selected during prototype testing
    int intensity_threshold = 80;
    int bright_pixel_count = 0;
    int total_pixels = fb->len;

    Serial.printf(
        "Total Pixels: %d\n",
        total_pixels
    );

    for (size_t i = 0; i < fb->len; i++) {
        uint8_t intensity = fb->buf[i];
        if (i % 1000 == 0) {
            Serial.printf(
                "Pixel %d: %d\n",
                i,
                intensity
            );
        }
        if (intensity > intensity_threshold) {
            bright_pixel_count++;
        }
    }

    float coverage =
        (float)bright_pixel_count /
        total_pixels *
        100;

    Serial.printf(
        "Bright Pixels: %d\n",
        bright_pixel_count
    );


    Serial.printf(
        "Duckweed Coverage: %.2f%%\n",
        coverage
    );

    // Send result to cloud/web server
    sendDataToServer(coverage);
    // Send integer coverage to Arduino
    mySerial.println((int)coverage);
    // Release camera memory
    esp_camera_fb_return(fb);

    delay(1000);
}

// Connect to available Wi-Fi network

void connectToWiFi() {

    Serial.println(
        "Trying to connect to available Wi-Fi networks..."
    );


    int numberOfNetworks =
        sizeof(wifiList) /
        sizeof(wifiList[0]);


    for (int i = 0; i < numberOfNetworks; i++) {

        Serial.print("Connecting to Wi-Fi: ");

        Serial.println(wifiList[i][0]);


        WiFi.begin(
            wifiList[i][0],
            wifiList[i][1]
        );


        int attempt = 0;


        while (
            WiFi.status() != WL_CONNECTED &&
            attempt < 20
        ) {

            delay(500);

            Serial.print(".");

            attempt++;
        }


        if (WiFi.status() == WL_CONNECTED) {

            Serial.println();
            Serial.println("Wi-Fi connected!");

            Serial.print("IP Address: ");

            Serial.println(
                WiFi.localIP()
            );

            return;

        } else {

            Serial.println();

            Serial.println(
                "Failed to connect. Trying next network..."
            );
        }
    }


    Serial.println(
        "No Wi-Fi networks available. Restarting ESP32..."
    );


    ESP.restart();
}

// Check Wi-Fi

void checkWiFi() {

    if (WiFi.status() != WL_CONNECTED) {

        Serial.println(
            "Wi-Fi disconnected. Reconnecting..."
        );

        connectToWiFi();
    }
}

// Send coverage data to Flask API

void sendDataToServer(float coverage) {

    if (WiFi.status() == WL_CONNECTED) {

        HTTPClient http;


        http.begin(serverUrl);


        http.addHeader(
            "Content-Type",
            "application/json"
        );


        String payload =
            "{\"coverage\": " +
            String(coverage, 2) +
            "}";


        Serial.print(
            "Sending Data to Server: "
        );


        Serial.println(payload);


        int httpResponseCode =
            http.POST(payload);


        if (httpResponseCode > 0) {

            String response =
                http.getString();


            Serial.printf(
                "Server Response: %s\n",
                response.c_str()
            );

        } else {

            Serial.printf(
                "Error sending POST request: %d\n",
                httpResponseCode
            );
        }


        http.end();

    } else {

        Serial.println(
            "Wi-Fi not connected. Skipping data transmission."
        );
    }
}
