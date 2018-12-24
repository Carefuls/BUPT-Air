#include <HttpPacket.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal.h>//调用库文件
#include <dht.h>
LiquidCrystal lcd(0, 13, 12, 14, 4, 5);//用到的的IO口
HttpPacketHead packet;
dht DHT;
#define DHT11_PIN 2
#define myPeriodic 1

unsigned long seconds;
int sent = 0;
int s = 1,m = 12, h = 19, d = 23, mon = 12, y = 2018;
int second = 1, minute = 12, hour = 19, day = 23, month = 12, year = 2018;
int SECOND = 1, MINUTE = 12, HOUR = 19, DAY = 23, MONTH = 12, YEAR = 2018;
int chose = 0, ButtonDelay = 10;
int dustPin = A0;
float dustVal = 0;
int ledPower = 16;
int delayTime = 280;
int delayTime2 = 40;
float offTime = 9680;
String monthstr = "";
String weekstr = "";
String daystr = "";
char OneNetServer[] = "api.heclouds.com";       //不需要修改
const char ssid[] = "iPhone";     //修改为自己的路由器用户名
const char password[] = "16150050727"; //修改为自己的路由器密码
char device_id[] = "502977540";    //修改为自己的设备ID
char API_KEY[] = "YyURrj0Ic5ch=xG3P2iDUxoI5c0=";    //修改为自己的API_KEY
char sensor_id1[] = "Temperature";
char sensor_id2[] = "Humidity";
char sensor_id3[] = "Light";

void connectWifi(){  
  Serial.print("Connecting to " + *ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");  
  }
  Serial.println("");
  Serial.println("Connected");
  Serial.println("");
  digitalWrite(0, HIGH);
  delay(1000);
  digitalWrite(0, LOW);
  delay(1000);
  digitalWrite(0, HIGH);
}

void postDataToOneNet(char* API_VALUE_temp, char* device_id_temp, char* sensor_id_temp, float thisData){  
  WiFiClient client;
  StaticJsonBuffer<250> jsonBuffer;
  JsonObject& myJson = jsonBuffer.createObject();
  JsonArray& datastreams= myJson.createNestedArray("datastreams");
  JsonObject& id_datapoints = datastreams.createNestedObject();
  id_datapoints["id"] = sensor_id_temp;
  JsonArray& datapoints = id_datapoints.createNestedArray("datapoints");
  JsonObject& value = datapoints.createNestedObject();
  value["value"] =thisData;
  char p[180];
  myJson.printTo(p, sizeof(p)); 
  packet.setHostAddress(OneNetServer);
  packet.setDevId(device_id_temp);   //device_id
  packet.setAccessKey(API_VALUE_temp);  //API_KEY
  /*create the http message about add datapoint */
  packet.createCmdPacket(POST, TYPE_DATAPOINT, p);
  if (strlen(packet.content))
  Serial.print(packet.content);
  Serial.println(p);
  char cmd[400];
  memset(cmd, 0, 400);  
  strcpy(cmd, packet.content);
  strcat(cmd, p);
  if (client.connect(OneNetServer, 80)){ 
    Serial.println("WiFi Client connected ");
    client.print(cmd);
    delay(1000);
  }//end if
  //  Serial.println(cmd);
  client.stop();
}

void FormatDisplay(int col, int row, int num)
{   
  lcd.setCursor(col, row); 
  if(num < 10)
  {
    lcd.print("0");
  }   
  lcd.print(num);   
}

void time()
{   
  second = (SECOND + seconds) % 60;
  m = (SECOND + seconds) / 60;
  minute = (MINUTE + m) % 60; 
  h = (MINUTE + m) / 60;     
  hour = (HOUR + h) % 24; 
  d = (HOUR + h) / 24;
}
int Days(int year, int month){   
  int days = 0;
  if(month != 2){     
    switch(month){       
      case 1: case 3: case 5: case 7: case 8: case 10: case 12: days = 31;  break;
      case 4: case 6: case 9: case 11:  days = 30;  break;
    }
  } 
  else{ 
    if(year % 4 == 0 && year % 100 != 0 || year % 400 == 0){
      days = 29;
    }   
    else{
      days = 28;
    }     
  }
  return days;   
}

void Day(){      
  int days = Days(year, month);
  int days_up;
  if(month == 1){
    days_up = Days(year - 1, 12);
  } 
  else{
    days_up = Days(year, month - 1);
  }  
  day = (DAY + d) % days;
  if(day == 0){
    day = days;    
  }   
  if((DAY + d) == days + 1){   
    DAY -= days;
    mon++;
  }
  if((DAY + d) == 0){
    DAY += days_up;
    mon--;
  }
  switch(day){     
    case 1: daystr = "st";   break;
    case 2: daystr = "nd";   break;
    case 3: daystr = "rd";   break;
    case 21: daystr = "st";  break;
    case 22: daystr = "nd";  break;
    case 23: daystr = "rd";  break;
    case 31: daystr = "st";  break;
    default: daystr = "th";  break;
  }
}

void Month(){   
  month = (MONTH + mon) % 12;
  if(month == 0){
    month = 12;
  }  
  y = (MONTH + mon - 1) / 12;
  switch(month){     
    case 1: monthstr = "January";   break;
    case 2: monthstr = "February";  break;
    case 3: monthstr = "March";     break;
    case 4: monthstr = "April";     break;
    case 5: monthstr = "May";       break;
    case 6: monthstr = "June";      break;
    case 7: monthstr = "July";      break;
    case 8: monthstr = "August";    break;
    case 9: monthstr = "September"; break;
    case 10: monthstr = "October";  break;
    case 11: monthstr = "November"; break;
    case 12: monthstr = "December"; break;
  }
}

void Year(){     
  year = (YEAR + y) % 9999;
  if(year == 0){
    year = 9999;
  }
}

void Week(int y,int m, int d){        
  if(m == 1) m = 13;
  if(m == 2) m = 14;
  int week = (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7 + 1; 
  switch(week){     
    case 1: weekstr = "Monday";    break;
    case 2: weekstr = "Tuesday";   break;
    case 3: weekstr = "Wednesday"; break;
    case 4: weekstr = "Thursday";  break;
    case 5: weekstr = "Friday";    break;
    case 6: weekstr = "Saturady";  break;
    case 7: weekstr = "Sunday";    break;
  }
}

void Display()
{  
  time();
  Day();  
  Month();
  Year();
  Week(year, month, day);  
}

void DisplayCursor(int rol, int row)
{
  lcd.setCursor(rol, row);   
  lcd.cursor();
  delay(100);     
  lcd.noCursor();
  delay(100);    
}

void set(int y, int mon, int d, int h, int m, int s)
{
  YEAR = y;
  MONTH = mon;
  DAY = d;  
  HOUR = h;
  MINUTE = m;
  SECOND = s;  
}

void setup() {
// 设置行列值
  Serial.begin(9600);
  lcd.begin(16, 2);//16列，2行。1602 液晶可以显示2 行，每行显示16 个字符。
// 打印字符串
  set(2018, 12, 23, 20, 10, 00);
  lcd.setCursor(0, 0);  lcd.print(" BUPT SICE 2018");
  lcd.setCursor(0, 1);  lcd.print("    Team 174");
  pinMode(ledPower, OUTPUT);
  pinMode(dustPin, INPUT);
  delay(4000);
}

void loop() {
 digitalWrite(ledPower,LOW); 
 delayMicroseconds(delayTime);
 dustVal=analogRead(dustPin); 
 delayMicroseconds(delayTime2);
 digitalWrite(ledPower,HIGH); 
 delayMicroseconds(offTime);
 if (dustVal > 36.455)
 Serial.println((float(dustVal / 1024) - 0.0356) * 120000 * 0.035);
 lcd.setCursor(0, 0);  lcd.print("                ");
 lcd.setCursor(0, 1);  lcd.print("                ");
 seconds = millis() / 1000;
 Display(); 
 FormatDisplay(11, 0, hour);
 FormatDisplay(14, 0, minute);  
 lcd.setCursor(13, 0); lcd.print(":");
 int chk = DHT.read11(DHT11_PIN);
 Serial.print(DHT.humidity, 1);
 Serial.print(",\t");
 Serial.println(DHT.temperature, 1);
 lcd.setCursor(0, 0);
 lcd.print("PM:");
 lcd.print(((float((dustVal / 1024) - 0.0356) * 120000 * 0.035) / 10 * 1.239), 3);
 lcd.setCursor(0, 1);
 lcd.print("H:");
 lcd.print(DHT.humidity, 1);
 lcd.print("%  ");
 lcd.print("T:");
 lcd.print(DHT.temperature, 1);
 lcd.print("C");
 int temp =(DHT.temperature); int hum = (DHT.humidity); float pm = (((dustVal / 1024) - 0.0356) * 14868 * 0.035);
 postDataToOneNet(API_KEY, device_id, sensor_id1, temp);
  delay(100);
  postDataToOneNet(API_KEY, device_id, sensor_id2, hum);
  delay(100);
  postDataToOneNet(API_KEY, device_id, sensor_id3, pm);
  Serial.println("N0 " + String(sent) + " Stream: was send");
  sent++;
  int count = myPeriodic;
  while (count--)
 delay(10000);
 lcd.setCursor(0, 0);  lcd.print("                ");
 lcd.setCursor(0, 1);  lcd.print("                ");
 lcd.setCursor(0, 0);  lcd.print(monthstr);  lcd.print(" ");  lcd.print(day);  lcd.print(daystr); 
 lcd.setCursor(0, 1);  lcd.print(weekstr);
 delay(4000);
}
