#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>

// ************************************************************
// تنظیمات شبکه وای‌فای
// ************************************************************
const char* ssid = "MohsenZiadi";
const char* password = "123456789";
WiFiServer wifiServer(8888); 

// ************************************************************
// تنظیمات پین‌ها
// ************************************************************
#define rxPin D0
#define txPin D4

SoftwareSerial mySerial(rxPin, txPin);

const char HEADER = 0xFF;

// آرایه موقعیت سرووها
int servoPos[16] = {
  123, 190, 165, 45, 100, 132, 70, 85,
  205, 140, 144, 170, 179, 195, 200, 100
};

// متغیر برای مدیریت کلاینت وای‌فای
WiFiClient globalClient;

void setup() {
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  
  Serial.begin(115200);
  mySerial.begin(115200);
  
  // اتصال به وای‌فای
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  // منتظر ماندن برای اتصال
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected!");
  
  wifiServer.begin();
  Serial.println("TCP Server started on port 8888");
  
  // راه‌اندازی ربات
  Serial.println("Initializing Servos...");
  initializeServos();
  configurePID();
  standup();

  printHelp();
}

void loop() {
  // ------------------------------------------
  // 1. بررسی دستورات سریال (USB)
  // ------------------------------------------
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processSerialCommand(command);
  }

  // ------------------------------------------
  // 2. بررسی اتصال کلاینت جدید
  // ------------------------------------------
  if (wifiServer.hasClient()) {
    // اگر کلاینت قبلی متصل بود و هنوز زنده است، آن را قطع کن تا کلاینت جدید وصل شود
    // (یا اگر نود-رد کانکشن جدید باز کرد)
    if (globalClient && globalClient.connected()) {
      globalClient.stop();
    }
    globalClient = wifiServer.available();
    Serial.println("New TCP Client Connected!");
  }

  // ------------------------------------------
  // 3. خواندن اطلاعات از کلاینت متصل
  // ------------------------------------------
  if (globalClient && globalClient.connected()) {
    if (globalClient.available() > 0) {
      // خواندن تا رسیدن به خط جدید
      String req = globalClient.readStringUntil('\n');
      req.trim(); // حذف فضاها و اینترهای اضافی
      
      if(req.length() > 0) {
         processTCPCommand(req);
      }
    }
  }
}

// ================== پردازش دستورات TCP ==================
void processTCPCommand(String data) {
  // فرمت مورد انتظار: "5:120"
  int separatorIndex = data.indexOf(':');
  
  if (separatorIndex > 0) {
    String idStr = data.substring(0, separatorIndex);
    String angleStr = data.substring(separatorIndex + 1);
    
    int id = idStr.toInt();
    int angle = angleStr.toInt();
    
    // اعتبارسنجی
    if (id >= 0 && id <= 15 && angle >= 0 && angle <= 254) {
      // فقط در صورتی که تغییر کرده ارسال کن (برای جلوگیری از ترافیک اضافی)
      if (servoPos[id] != angle) {
        moveServoAbsolute(id, angle);
        // لاگ نزنیم تا سرعت بالا برود، فقط اجرا کنیم
      }
    }
  }
}

// ================== پردازش دستورات سریال ==================
void processSerialCommand(String cmd) {
  if (cmd.length() == 0) return;
  
  if (cmd.equalsIgnoreCase("help") || cmd.equalsIgnoreCase("h")) {
    printHelp();
    return;
  }
  if (cmd.equalsIgnoreCase("stand") || cmd.equalsIgnoreCase("s")) {
    standup();
    return;
  }
  
  int spaceIndex = cmd.indexOf(' ');
  if (spaceIndex > 0) {
    String idStr = cmd.substring(0, spaceIndex);
    String valueStr = cmd.substring(spaceIndex + 1);
    valueStr.trim();
    int id = idStr.toInt();
    
    if (valueStr.length() > 0 && (valueStr[0] == '+' || valueStr[0] == '-')) {
        int delta = valueStr.toInt();
        int newPos = servoPos[id] + delta;
        if(newPos > 254) newPos = 254;
        if(newPos < 0) newPos = 0;
        
        Serial.print("Servo "); Serial.print(id); 
        Serial.print(" -> "); Serial.println(newPos);
        moveServoAbsolute(id, newPos);
    }
  }
}

// ================== راهنما ==================
void printHelp() {
  Serial.println("\n📋 ** ROBOT COMMANDS **");
  Serial.println("──────────────────────");
  Serial.print("🌐 IP Address: ");
  Serial.println(WiFi.localIP()); 
  Serial.println("──────────────────────");
  Serial.println(" help / h      → Show this menu & IP");
  Serial.println(" stand / s     → Return to stand position");
  Serial.println(" [ID] +[value] → Increase servo by value");
  Serial.println("──────────────────────");
  Serial.println("Node-RED Format: 'ID:ANGLE\\n'");
  Serial.println("──────────────────────\n");
}

// ================== حرکت سروو ==================
void moveServoAbsolute(int id, int angle) {
  servoPos[id] = angle;
  MovePosition(id, 4, angle);
  // تاخیر بسیار کوتاه برای اینکه ربات هنگ نکند
  delayMicroseconds(200); 
}

// ================== توابع درایور (بدون تغییر) ==================
void initializeServos() {
  for (int i = 0; i <= 15; i++) {
    if(i >= 10) SetSpeedAndAcceleration(i, 20, 100); 
    else SetSpeedAndAcceleration(i, 40, 100);
    delay(5);
  }
}

void configurePID() {
  P_D_set(1, 30, 20); I_set(1, 0);
  P_D_set(3, 30, 20); I_set(3, 0);
  P_D_set(8, 30, 20); I_set(8, 0);
}

void standup() {
  Serial.println("Moving to Stand Position...");
  MovePosition(0, 4, 123); MovePosition(2, 4, 165); MovePosition(7, 4, 85); delay(50);
  MovePosition(6, 4, 70); MovePosition(1, 4, 190); MovePosition(3, 4, 45);
  MovePosition(4, 4, 100); MovePosition(5, 4, 132); MovePosition(8, 4, 205);
  MovePosition(9, 4, 140); MovePosition(10, 4, 144); MovePosition(13, 4, 195);
  MovePosition(11, 4, 170); MovePosition(14, 4, 200); MovePosition(12, 4, 179);
  MovePosition(15, 4, 100);
  delay(500);
  
  int standPos[16] = {123, 190, 165, 45, 100, 132, 70, 85, 205, 140, 144, 170, 179, 195, 200, 100};
  for(int i=0; i<16; i++) servoPos[i] = standPos[i];
  Serial.println("✓ Stand position achieved");
}

void MovePosition(char Id, char Tourq, char TargetPosition) {
  char Checksum = 0;
  char data1 = ((Tourq << 5) | Id) & 0xFF;
  Checksum = (data1 ^ TargetPosition) & 0x7F;
  mySerial.write(HEADER);
  mySerial.write(data1);
  mySerial.write(TargetPosition);
  mySerial.write(Checksum);
}

void SetSpeedAndAcceleration(char Id, char Speed, char Acceleration) {
  char Checksum = 0;
  char data1 = ((7 << 5) | Id) & 0xFF;
  Checksum = (data1 ^ 0x0D ^ Speed ^ Acceleration) & 0x7F;
  mySerial.write(HEADER); mySerial.write(data1); mySerial.write(0x0D);
  mySerial.write(Speed); mySerial.write(Acceleration); mySerial.write(Checksum);
}

void P_D_set(char Id, char P, char D) {
  char Checksum = 0;
  char data1 = (((7 << 5) | Id) & 0xFF);
  char data2 = 0x0B;
  Checksum = ((data1 ^ data2 ^ P ^ D) & 0x7F);
  mySerial.write(HEADER); mySerial.write(data1); mySerial.write(data2);
  mySerial.write(P); mySerial.write(D); mySerial.write(Checksum);
}

void I_set(char Id, char I) {
  char Checksum = 0;
  char data1 = (((7 << 5) | Id) & 0xFF);
  char data2 = 0x18;
  char data3 = I & 0xff;
  char data4 = I;
  Checksum = ((data1 ^ data2 ^ data3 ^ data4) & 0x7F);
  mySerial.write(HEADER); mySerial.write(data1); mySerial.write(data2);
  mySerial.write(data3); mySerial.write(data4); mySerial.write(Checksum);
}
