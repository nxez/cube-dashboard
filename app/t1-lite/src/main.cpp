/**
Copyright (c) 2021 by NXEZ.com
See more at https://www.nxez.com
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
//#include <SSD1306Wire.h>
#include <SH1106.h>
#include <OLEDDisplayUi.h>
#include <Wire.h>
#include <time.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <RTClib.h>

#include "settings.h"

struct {
    String Version = "1.0.0";
    String ProjectID = "P412-Lite";
    String URL = "https://make.quwj.com/project/412";
    String Author = "NXEZ.com";
} AppInfo;

struct DashboardData{
    float cpu_percent;
    float virtual_memory_free;
    float virtual_memory_total;
    float virtual_memory_used;
    String disk_usage_free;
    String disk_usage_percent;
    int up_time;
    int cpu_temperature;
    int time_timestamp_cst;
};

int enableAutoTransition = 1;
bool screenOn = true;
int batteryRemaining = 0;

int tasterPin[] = {D6, D5, D4};
int timeoutTaster[] = {0, 0, 0, 0};
bool pushed[] = {false, false, false, false};
int blockTimeTaster[] = {0, 0, 0, 0};
bool blockTaster[] = {false, false, false, false};
bool blockTaster2[] = {false, false, false, false};
bool tasterState[3];

unsigned long millisTimeUpdated = millis(); 
String lastUpdate = "--";

Ticker ticker;
DateTime baseTime;
DateTime now;

char monName[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char wdayName[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

//declaring prototypes
void drawProfile(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawHardwareInfo(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state);
int8_t getWifiQuality();
int8_t getbatteryRemaining();
int checkTaster(int nr);

// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = {drawProfile, drawHardwareInfo};
int numberOfFrames = 2;

DashboardData currentData;

void initCurrentData(){
    currentData.up_time = 0;
    currentData.cpu_percent = 0;
    currentData.cpu_temperature = -128;
    currentData.disk_usage_free = "";
    currentData.disk_usage_percent = "";
    currentData.virtual_memory_free = 0;
    currentData.virtual_memory_total = 0;
    currentData.virtual_memory_used = 0;
    currentData.time_timestamp_cst = 0;
}

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;
String clientId = "CUBE-";

void drawSplash(OLEDDisplay *display, String label){
    display->clear();
    display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64, 32, label);
    display->display();
}

void mqttCallback(char *topic, byte *payload, unsigned int length){
    String jsonString = "";
    for (int i = 0; i < (int)length; i++){
        jsonString += (char)payload[i];
    }
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(jsonString);
    //JsonArray &json = jsonBuffer.parseArray(jsonString);
    if(json.success()){
        currentData.up_time = json["up_time"];
        currentData.cpu_percent = json["cpu_percent"];
        currentData.cpu_temperature = json["cpu_temperature"];
        currentData.time_timestamp_cst = json["time"]["timestamp_cst"];

        currentData.virtual_memory_free = json["virtual_memory"]["free"];
        currentData.virtual_memory_total = json["virtual_memory"]["total"];
        currentData.virtual_memory_used = json["virtual_memory"]["used"];

        String disk_usage_free = json["disk_usage"]["free"];
        currentData.disk_usage_free = disk_usage_free;

        String disk_usage_percent = json["disk_usage"]["percent"];
        currentData.disk_usage_percent = disk_usage_percent;

        Serial.println(currentData.time_timestamp_cst);
    }
    else{
        Serial.println("parse object error: " + jsonString);
    }
}

void reconnectBroker(){
    Serial.println("reconnecting to " + String(mqttBroker));

    if (client.connect(clientId.c_str())){
        Serial.println("connected to host!");
        client.subscribe("nxez/cube/dashboard/server");
        Serial.println("subscribed topic: nxez/cube/dashboard/server");
        client.publish("nxez/cube/dashboard", "connected");

        //isClientStarted = true;
        Serial.println("Host OK");
    }
}

void drawProgress(OLEDDisplay *display, int percentage, String label){
    display->clear();
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64, 10, label);
    //display->drawProgressBar(2, 28, 124, 12, percentage);
    display->drawProgressBar(10, 28, 108, 12, percentage);
    display->display();
}

void drawProfile(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y){
    now = DateTime(currentData.time_timestamp_cst);
    
    char date_str[40];

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);

    display->drawString(64 + x, 0 + y, "Dashboard");
    display->drawHorizontalLine(0 + x, 13 + y, 128);

    display->setFont(ArialMT_Plain_24);
    display->setTextAlignment(TEXT_ALIGN_LEFT);

    sprintf(date_str, "%02d:%02d:%02d", now.hour(),now.minute(),now.second());
    display->drawString(x + 16, 16 + y, date_str);

    snprintf_P(date_str,
    sizeof(date_str),
    PSTR("%04u-%02u-%02u (%03s)"),
    now.year(), now.month(), now.day(), wdayName[now.dayOfTheWeek()]);
    
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(ArialMT_Plain_10);
    display->drawString(64 + x, 41 + y, date_str);

    String uptime = "Uptime ";
    
    int days = 0;
    if(currentData.up_time > 3600 * 24){
        days = currentData.up_time / (3600 * 24);
        uptime = uptime + (String)(days) + "days ";
    }
    int hours = 0;
    int mins = 0;
    if((currentData.up_time - 3600 * 24 * days) > 3600){
        hours = (currentData.up_time - 3600 * 24 * days) / 3600;
        uptime = uptime + ((hours < 10) ? "0" : "") + (String)(hours) + ":";
        mins = (currentData.up_time - 3600 * 24 * days - 3600 * hours) / 60;
        uptime = uptime + ((mins < 10) ? "0" : "") + (String)(mins);
    }
    
    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->drawString(64 + x, 52 + y, uptime);
}

void drawHardwareInfo(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y){
    char date_str[80];

    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->setFont(ArialMT_Plain_10);

    String title = "CPU usage: " + (String)(currentData.cpu_percent) + "% (" + (String)(currentData.cpu_temperature) + "°C)";
    display->drawString(0 + x, 0 + y, title);

    display->drawHorizontalLine(0 + x, 14 + y, int(1.28*currentData.cpu_percent));
    display->drawHorizontalLine(0 + x, 15 + y, int(1.28*currentData.cpu_percent));
    display->drawHorizontalLine(0 + x, 16 + y, 128);

    display->drawString(0 + x, 20 + y, "Memory stats: ");
    sprintf(date_str, "total: %.2f MB", currentData.virtual_memory_total);
    display->drawString(0 + x, 32 + y, date_str);
    sprintf(date_str, "free: %.2f MB", currentData.virtual_memory_free);
    display->drawString(0 + x, 42 + y, date_str);

    //sprintf(date_str, "Disk free: ", currentData.disk_usage_free);
    display->drawString(0 + x, 54 + y, "Disk free: " + currentData.disk_usage_free);
}

void setup() {
    pinMode(D4, INPUT_PULLUP);
    pinMode(D5, INPUT_PULLUP);
    pinMode(D6, INPUT_PULLUP);

    //WiFi.mode(WIFI_STA);
    //wifiManager.resetSettings();
    Serial.setRxBufferSize(1024);
    Serial.begin(115200);

    pinMode(D7,OUTPUT);
    initCurrentData();

    clientId += String(ESP.getChipId(), HEX);
    //clientId.toLowerCase();
    clientId.toUpperCase();
    Serial.println("clientId: " + (String)(clientId.c_str()));

    // initialize dispaly
    display.init();
    display.clear();
    display.display();
    //display.flipScreenVertically();
    display.flipScreenVertically(); // Comment out to flip display 180deg
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setContrast(255);

    drawSplash(&display, "- NXEZ CUBE -\nwww.nxez.com");
    //delay(2000);
    
    pinMode(D0, OUTPUT);
    tone(D0, 800);
    delay(100);
    tone(D0, 0);
    delay(100);
    tone(D0, 600);
    delay(100);
    tone(D0, 0);
    delay(1500);

    drawSplash(&display, "Cube Dashboard\nver: " + AppInfo.Version +"\n(" + AppInfo.ProjectID + ")");
    delay(1500);
    
    drawSplash(&display, "Connecting to WiFi..");

    WiFi.begin(WIFI_SSID, WIFI_PWD);

    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }

    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());

    //String info = clientId + ".local\n" + WiFi.localIP().toString();
    drawSplash(&display, clientId + ".local\n" + WiFi.localIP().toString());
    delay(3000);

    client.setServer(mqttBroker, atoi(mqttBrokerPort));
    client.setCallback(mqttCallback);
    Serial.println("Host：" + String(mqttBroker) + ":" + atoi(mqttBrokerPort));

    ui.setTargetFPS(30);
    ui.setTimePerFrame(5000);

    // You can change the transition that is used
    // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
    ui.setFrameAnimation(SLIDE_UP);

    // Add frames
    ui.setFrames(frames, numberOfFrames);
    ui.disableAllIndicators();

    // Inital UI takes care of initalising the display too.
    //ui.init();

    ui.enableAutoTransition();
}

void loop(){
	checkTaster(0);
	checkTaster(1);
	checkTaster(2);

    if (!client.connected()){
        Serial.println("connecting error, retrying...");    
        reconnectBroker();
        delay(10);
    }
    else{
        client.loop();
    }

    now = baseTime.operator+(TimeSpan((millis()-millisTimeUpdated)/1000));

    int remainingTimeBudget = ui.update();

    if (remainingTimeBudget > 0) {
        // You can do some work here
        // Don't do stuff if you are below your
        // time budget.
        delay(remainingTimeBudget);
    }
}

int checkTaster(int nr){
	tasterState[0] = digitalRead(tasterPin[0]);
	tasterState[1] = digitalRead(tasterPin[1]);
	tasterState[2] = digitalRead(tasterPin[2]);

	switch (nr)	{
	case 0:
		if (tasterState[0] == LOW && !pushed[nr] && !blockTaster2[nr] && tasterState[1] && tasterState[2]){
			pushed[nr] = true;
			timeoutTaster[nr] = millis();
		}
		break;
	case 1:
		if (tasterState[1] == LOW && !pushed[nr] && !blockTaster2[nr] && tasterState[0] && tasterState[2]){
			pushed[nr] = true;
			timeoutTaster[nr] = millis();
		}
		break;
	case 2:
		if (tasterState[2] == LOW && !pushed[nr] && !blockTaster2[nr] && tasterState[0] && tasterState[1]){
			pushed[nr] = true;
			timeoutTaster[nr] = millis();
		}
		break;
	case 3:
		if (tasterState[0] == LOW && tasterState[2] == LOW && !pushed[nr] && !blockTaster2[nr] && tasterState[1]){
			pushed[nr] = true;
			timeoutTaster[nr] = millis();
		}
		break;
	}

	if (pushed[nr] && (millis() - timeoutTaster[nr] < 2000) && tasterState[nr] == HIGH){
		if (!blockTaster2[nr]){
			switch (nr){
			case 0:
				Serial.println("LEFT: normaler Tastendruck");
                //transitionToFrame("previous");
                ui.previousFrame();
				break;
			case 1:
				Serial.println("MID: normaler Tastendruck");
                //updateWeatherCurrent();
                screenOn = true;
                display.displayOn();

                enableAutoTransition = !enableAutoTransition;

                if(enableAutoTransition){
                    ui.enableAutoTransition();
                }
                else{
                    ui.disableAutoTransition();
                }

				break;
			case 2:
				Serial.println("RIGHT: normaler Tastendruck");
                //transitionToFrame("next");
                ui.nextFrame();
				break;
			}

			pushed[nr] = false;
			return 1;
		}
	}

	if (pushed[nr] && (millis() - timeoutTaster[nr] > 2000)){
		if (!blockTaster2[nr]){
			switch (nr){
			case 0:
				Serial.println("LEFT: langer Tastendruck");
				break;
			case 1:
				Serial.println("MID: langer Tastendruck");
                screenOn = !screenOn;
                if(screenOn){
                    display.displayOn();
                }
                else{
                    display.displayOff();
                }

				break;
			case 2:
				Serial.println("RIGHT: langer Tastendruck");
				break;
			case 3:
				break;
			}
			blockTaster[nr] = true;
			blockTaster2[nr] = true;
			pushed[nr] = false;
			return 2;
		}
	}
	if (nr == 3){
		if (blockTaster[nr] && tasterState[0] == HIGH && tasterState[2] == HIGH){
			blockTaster[nr] = false;
			blockTimeTaster[nr] = millis();
		}
	}
	else
	{
		if (blockTaster[nr] && tasterState[nr] == HIGH){
			blockTaster[nr] = false;
			blockTimeTaster[nr] = millis();
		}
	}

	if (!blockTaster[nr] && (millis() - blockTimeTaster[nr] > 500)){
		blockTaster2[nr] = false;
	}
	return 0;
}

int8_t getbatteryRemaining(){
    int vola = 0;
    vola = analogRead(A0);
    double vol = ((vola/1023.0)*(110+442))/110.0;

    //Serial.println(vola);
    Serial.printf("vol: %f\n", ((vola/1023.0)*(110+442))/110.0);

    //int32_t percent = 0;

    if (vol <= 3.2){
        return 0;
    }

    if (vol >= 4.2){
        return 5;
    }
    
    if (vol >= 4.0){
        return 4;
    }
    
    if (vol >= 3.8){
        return 4;
    }
    
    if (vol >= 3.6){
        return 3;
    }
    
    if (vol >= 3.4){
        return 2;
    }
    
    if (vol >= 3.3){
        return 1;
    }

    return 0;
}