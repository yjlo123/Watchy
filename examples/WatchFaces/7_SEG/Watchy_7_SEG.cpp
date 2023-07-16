#include "Watchy_7_SEG.h"

#define DARKMODE false

const uint8_t BATTERY_SEGMENT_WIDTH = 7;
const uint8_t BATTERY_SEGMENT_HEIGHT = 11;
const uint8_t BATTERY_SEGMENT_SPACING = 9;
const uint8_t WEATHER_ICON_WIDTH = 48;
const uint8_t WEATHER_ICON_HEIGHT = 32;

const int apiInterval = 15;  // minutes
RTC_DATA_ATTR int apiIntervalCounter = -1;
RTC_DATA_ATTR char apiDataCache[16];  // must be char array to use RTC_DATA_ATTR

RTC_DATA_ATTR char apiDataWordCache[16];
RTC_DATA_ATTR char apiDataMeaningCache[20];


void Watchy7SEG::drawWatchFace(){
    display.fillScreen(DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);
    display.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    drawTime();
    drawDate();
    drawSteps();
    drawWeather();
    drawBattery();
    display.drawBitmap(120, 77, WIFI_CONFIGURED ? wifi : wifioff, 26, 18, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    if(BLE_CONFIGURED){
        display.drawBitmap(100, 75, bluetooth, 13, 21, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    }
}

void Watchy7SEG::drawTime(){
    display.setFont(&DSEG7_Classic_Bold_53);
    display.setCursor(5, 53+5);
    int displayHour;
    if(HOUR_12_24==12){
      displayHour = ((currentTime.Hour+11)%12)+1;
    } else {
      displayHour = currentTime.Hour;
    }
    if(displayHour < 10){
        display.print("0");
    }
    display.print(displayHour);
    display.print(":");
    if(currentTime.Minute < 10){
        display.print("0");
    }
    display.println(currentTime.Minute);
}

void Watchy7SEG::drawDate(){
    display.setFont(&Seven_Segment10pt7b);

    int16_t  x1, y1;
    uint16_t w, h;

    String dayOfWeek = dayStr(currentTime.Wday);
    display.getTextBounds(dayOfWeek, 5, 85, &x1, &y1, &w, &h);
    if(currentTime.Wday == 4){
        w = w - 5;
    }
    display.setCursor(85 - w, 85);
    display.println(dayOfWeek);

    String month = monthShortStr(currentTime.Month);
    display.getTextBounds(month, 60, 110, &x1, &y1, &w, &h);
    display.setCursor(85 - w, 110);
    display.println(month);

    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(5, 120);
    if(currentTime.Day < 10){
    display.print("0");
    }
    display.println(currentTime.Day);
    display.setCursor(5, 150);
    display.println(tmYearToCalendar(currentTime.Year));// offset from 1970, since year is stored in uint8_t
}

String request() {
  HTTPClient http;
  http.setConnectTimeout(3000);
  String weatherQueryURL = "http://167.172.128.163:8088/api/iot/v1/random_word";
  http.begin(weatherQueryURL.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
      return http.getString();
  }
  return "";
}

void Watchy7SEG::drawSteps(){
    //-- reset step counter at midnight
    // if (currentTime.Hour == 0 && currentTime.Minute == 0){
    //   sensor.resetStepCounter();
    // }
    // uint32_t stepCount = sensor.getCounter();
    // display.drawBitmap(10, 165, steps, 19, 23, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    // display.setCursor(35, 190);
    // display.println(stepCount);

    display.setFont(&FreeMonoBold9pt7b);
    if (apiIntervalCounter < 0) {
      //-1 on first run, set to apiInterval
      apiIntervalCounter = apiInterval;
    }
    if (apiIntervalCounter >= apiInterval && connectWiFi()) {
      String apiResult = request();

      if (apiResult == "") {
        display.setCursor(5, 170);
        display.println("API request error");
        return;
      }

      JSONVar obj     = JSON.parse(apiResult);
      String word = JSONVar::stringify(obj["word"]);

      word = word.substring(1, word.length()-1);  // remove quote
      String meaning = JSONVar::stringify(obj["meaning"]);
      if (meaning.length() > 18) {
        meaning = meaning.substring(1, 18);
      } else {
        meaning = meaning.substring(1, meaning.length()-1);
      }

      word.toCharArray(apiDataWordCache, word.length()+1);
      meaning.toCharArray(apiDataMeaningCache, 19);

      display.setCursor(5, 170);
      display.println(word);
      display.setCursor(5, 190);
      display.println(meaning);

      apiIntervalCounter = 0;
    } else {
      apiIntervalCounter++;

      display.setCursor(5, 170);
      display.println(String(apiDataWordCache));
      display.setCursor(5, 190);
      display.println(String(apiDataMeaningCache));

      display.setCursor(190, 170);
      display.println("*");
    }
    
}
void Watchy7SEG::drawBattery(){
    display.drawBitmap(154, 73, battery, 37, 21, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.fillRect(159, 78, 27, BATTERY_SEGMENT_HEIGHT, DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);//clear battery segments
    int8_t batteryLevel = 0;
    float VBAT = getBatteryVoltage();
    if(VBAT > 4.1){
        batteryLevel = 3;
    }
    else if(VBAT > 3.95 && VBAT <= 4.1){
        batteryLevel = 2;
    }
    else if(VBAT > 3.80 && VBAT <= 3.95){
        batteryLevel = 1;
    }
    else if(VBAT <= 3.80){
        batteryLevel = 0;
    }

    for(int8_t batterySegments = 0; batterySegments < batteryLevel; batterySegments++){
        display.fillRect(159 + (batterySegments * BATTERY_SEGMENT_SPACING), 78, BATTERY_SEGMENT_WIDTH, BATTERY_SEGMENT_HEIGHT, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    }

    int8_t batteryPercentLevel = 0;
    if (VBAT >= 3.3) {
        batteryPercentLevel = 100.0*(VBAT-3.3)/0.9;
    }
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(150, 106);
    display.print(batteryPercentLevel);
    display.print("%");
}

void Watchy7SEG::drawWeather(){

    weatherData currentWeather = getWeatherData();

    int8_t temperature = currentWeather.temperature;
    int16_t weatherConditionCode = currentWeather.weatherConditionCode;

    display.setFont(&DSEG7_Classic_Regular_39);
    int16_t  x1, y1;
    uint16_t w, h;
    display.getTextBounds(String(temperature), 0, 0, &x1, &y1, &w, &h);
    if(159 - w - x1 > 87){
        display.setCursor(159 - w - x1, 150);
    }else{
        display.setFont(&DSEG7_Classic_Bold_25);
        display.getTextBounds(String(temperature), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(159 - w - x1, 136);
    }
    display.println(temperature);
    display.drawBitmap(165, 110, currentWeather.isMetric ? celsius : fahrenheit, 26, 20, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    const unsigned char* weatherIcon;

    //https://openweathermap.org/weather-conditions
    if(weatherConditionCode > 801){//Cloudy
    weatherIcon = cloudy;
    }else if(weatherConditionCode == 801){//Few Clouds
    weatherIcon = cloudsun;
    }else if(weatherConditionCode == 800){//Clear
    weatherIcon = sunny;
    }else if(weatherConditionCode >=700){//Atmosphere
    weatherIcon = atmosphere;
    }else if(weatherConditionCode >=600){//Snow
    weatherIcon = snow;
    }else if(weatherConditionCode >=500){//Rain
    weatherIcon = rain;
    }else if(weatherConditionCode >=300){//Drizzle
    weatherIcon = drizzle;
    }else if(weatherConditionCode >=200){//Thunderstorm
    weatherIcon = thunderstorm;
    }else
    return;
    display.drawBitmap(145, 158, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
}
