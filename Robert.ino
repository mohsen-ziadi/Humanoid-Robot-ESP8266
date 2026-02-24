#include <SoftwareSerial.h>

// ************************************************************
// تنظیمات پین‌ها دقیقا طبق کدی که فرستادی (برای برد ESP8266)
// ************************************************************
#define rxPin D0 // پین گیرنده
#define txPin D4 // پین فرستاده

// ساخت پورت سریال نرم‌افزاری برای صحبت با ربات
SoftwareSerial mySerial(rxPin, txPin);

// پروتکل ارتباطی
const char HEADER = 0xFF;

// آرایه‌ای برای ذخیره موقعیت فعلی 16 سروو (موقعیت پیش‌فرض ایستادن)
int servoPos[16] = {
  118, 190, 160, 63, 100, 122, 40, 100,
  200, 145, 230, 200, 195, 170, 200, 100
};

void setup() {
  // 1. تنظیم پین‌ها
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  
  // 2. شروع ارتباط سریال
  Serial.begin(115200); // برای ارتباط با کامپیوتر (USB)
  mySerial.begin(115200); // برای ارتباط با ربات (سرووها)
  
  Serial.println();
  Serial.println(">>> RoboBuilder Controller Started <<<");
  Serial.println("Initializing Servos...");
  
  // 3. مقداردهی اولیه - این بخش حیاتی است
  initializeServos();
  
  // 4. تنظیمات PID
  configurePID();
  
  // 5. قرار گرفتن در وضعیت ایستادن اولیه
  standup();
  
  // 6. نمایش راهنما
  printHelp();
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // حذف فاصله‌های اضافی
    
    processCommand(command);
  }
}

// ================== پردازش دستورات ==================
void processCommand(String cmd) {
  if (cmd.length() == 0) return;
  
  // دستور help
  if (cmd.equalsIgnoreCase("help") || cmd.equalsIgnoreCase("h")) {
    printHelp();
    return;
  }
  
  // دستور stand
  if (cmd.equalsIgnoreCase("stand") || cmd.equalsIgnoreCase("s")) {
    standup();
    Serial.println("→ Returning to Stand Position");
    return;
  }
  
  // دستورات افزایش/کاهش با علامت + و -
  // فرمت: [ID] +[مقدار]   یا   [ID] -[مقدار]
  // مثلاً: 5 +20  یا  10 -20
  
  int spaceIndex = cmd.indexOf(' ');
  if (spaceIndex > 0) {
    String idStr = cmd.substring(0, spaceIndex);
    String valueStr = cmd.substring(spaceIndex + 1);
    valueStr.trim();
    
    int id = idStr.toInt();
    
    if (id >= 0 && id <= 15) {
      // بررسی کنید که مقدار با + یا - شروع شده باشد
      if (valueStr.length() > 0 && (valueStr[0] == '+' || valueStr[0] == '-')) {
        // تبدیل رشته به عدد (خودکار علامت را هم در نظر می‌گیرد)
        int delta = valueStr.toInt();
        
        // محدود کردن دلتا به بازه معقول (اختیاری)
        if (delta > 100) delta = 100;
        if (delta < -100) delta = -100;
        
        adjustServo(id, delta);
      }
      else {
        Serial.println("❌ Invalid format. Use +[number] or -[number]");
        Serial.println("   Example: 5 +20  or  10 -20");
      }
    }
    else {
      Serial.println("❌ Invalid ID. Use 0-15");
    }
  }
  else {
    Serial.println("❌ Invalid format. Use: [ID] [+/-][value]  (example: 5 +20)");
  }
}

// تابع تغییر زاویه سروو
void adjustServo(int id, int delta) {
  int newPos = servoPos[id] + delta;
  
  // محدود کردن به بازه مجاز (0-254)
  if (newPos > 254) newPos = 254;
  if (newPos < 0) newPos = 0;
  
  // محاسبه مقدار واقعی تغییر (ممکنه با دلتای درخواستی فرق کنه بخاطر محدودیت بازه)
  int actualDelta = newPos - servoPos[id];
  
  // اعمال تغییر واقعی
  servoPos[id] = newPos;
  
  // ارسال فرمان به ربات
  MovePosition(id, 4, servoPos[id]);
  
  // نمایش نتیجه
  Serial.print("✓ Servo ");
  Serial.print(id);
  Serial.print(" → ");
  Serial.print(servoPos[id]);
  Serial.print(" (");
  
  if (actualDelta > 0) {
    Serial.print("+");
  }
  Serial.print(actualDelta);
  Serial.println("°)");
  
  // اگه مقدار درخواستی با مقدار واقعی فرق داشت (بخاطر محدودیت بازه)، هشدار بده
  if (actualDelta != delta) {
    Serial.print("  ⚠️ Requested: ");
    Serial.print(delta > 0 ? "+" : "");
    Serial.print(delta);
    Serial.println("° (adjusted due to range limits 0-254)");
  }
  
  delay(50); // تاخیر برای اجرای حرکت
}

// نمایش راهنما
void printHelp() {
  Serial.println("\n📋 ** ROBOT COMMANDS **");
  Serial.println("──────────────────────");
  Serial.println(" help / h      → Show this menu");
  Serial.println(" stand / s     → Return to stand position");
  Serial.println(" [ID] +[value] → Increase servo by value degrees");
  Serial.println(" [ID] -[value] → Decrease servo by value degrees");
  Serial.println("──────────────────────");
  Serial.println("Examples:");
  Serial.println("  '5 +20'  → Increase servo 5 by 20°");
  Serial.println("  '10 -30' → Decrease servo 10 by 30°");
  Serial.println("  '3 +45'  → Increase servo 3 by 45°");
  Serial.println("──────────────────────");
  Serial.println("📌 Note: Values are automatically limited to 0-254° range");
  Serial.println("──────────────────────\n");
}

// ================== توابع راه‌اندازی ==================
void initializeServos() {
  Serial.println("Setting speed and acceleration...");
  
  // تنظیم سرعت و شتاب برای همه موتورها
  for (int i = 0; i <= 15; i++) {
    if(i >= 10) {
      // موتورهای پایینی سرعت کمتر
      SetSpeedAndAcceleration(i, 5, 100);
    }
    else {
      SetSpeedAndAcceleration(i, 30, 100);
    }
    delay(10);
  }
  
  Serial.println("✓ Speed & Acceleration set");
}

void configurePID() {
  // تنظیمات PID طبق کد نمونه
  P_D_set(1, 30, 20); I_set(1, 0);
  P_D_set(3, 30, 20); I_set(3, 0);
  P_D_set(8, 30, 20); I_set(8, 0);
  
  Serial.println("✓ PID configured");
}

// ================== توابع درایور ربات ==================
void MovePosition(char Id, char Tourq, char TargetPosition) {
  char Checksum = 0;
  char data1;
  
  data1 = ((Tourq << 5) | Id) & 0xFF;
  Checksum = (data1 ^ TargetPosition) & 0x7F;
  
  mySerial.write(HEADER);
  mySerial.write(data1);
  mySerial.write(TargetPosition);
  mySerial.write(Checksum);
  
  delay(2); // تاخیر کوتاه برای اطمینان از ارسال
}

void SetSpeedAndAcceleration(char Id, char Speed, char Acceleration) {
  char Checksum = 0;
  char data1 = ((7 << 5) | Id) & 0xFF; // Mode = 7
  Checksum = (data1 ^ 0x0D ^ Speed ^ Acceleration) & 0x7F;
  
  if ((Speed >= 0 && Speed <= 30) && (Acceleration >= 20 && Acceleration <= 100)) {
    mySerial.write(HEADER);
    mySerial.write(data1);
    mySerial.write(0x0D);
    mySerial.write(Speed);
    mySerial.write(Acceleration);
    mySerial.write(Checksum);
  }
}

void P_D_set(char Id, char P, char D) {
  char Checksum = 0;
  char data1 = (((7 << 5) | Id) & 0xFF);
  char data2 = 0x0B;
  Checksum = ((data1 ^ data2 ^ P ^ D) & 0x7F);
  
  mySerial.write(HEADER); 
  mySerial.write(data1); 
  mySerial.write(data2);
  mySerial.write(P); 
  mySerial.write(D); 
  mySerial.write(Checksum);
}

void I_set(char Id, char I) {
  char Checksum = 0;
  char data1 = (((7 << 5) | Id) & 0xFF);
  char data2 = 0x18;
  char data3 = I & 0xff;
  char data4 = I;
  Checksum = ((data1 ^ data2 ^ data3 ^ data4) & 0x7F);
  
  mySerial.write(HEADER); 
  mySerial.write(data1); 
  mySerial.write(data2);
  mySerial.write(data3); 
  mySerial.write(data4); 
  mySerial.write(Checksum);
}

// تابع ایستادن
void standup() {
  Serial.println("Moving to stand position...");
  
  // ارسال پوزیشن‌ها بر اساس آرایه اولیه
  MovePosition(0, 4, 118);
  MovePosition(2, 4, 160);
  MovePosition(7, 4, 100);
  delay(100);
  
  MovePosition(6, 4, 40);
  MovePosition(1, 4, 190);
  MovePosition(3, 4, 63);
  MovePosition(4, 4, 100);
  MovePosition(5, 4, 122);
  MovePosition(8, 4, 200);
  MovePosition(9, 4, 145);
  MovePosition(10, 4, 230);
  MovePosition(13, 4, 170);
  MovePosition(11, 4, 200);
  MovePosition(14, 4, 200);
  MovePosition(12, 4, 195);
  MovePosition(15, 4, 100);
  delay(1000);
  
  // بروزرسانی آرایه موقعیت‌ها با مقادیر ایستادن
  servoPos[0] = 118; servoPos[1] = 190; servoPos[2] = 140; servoPos[3] = 43;
  servoPos[4] = 100; servoPos[5] = 122; servoPos[6] = 50; servoPos[7] = 100;
  servoPos[8] = 205; servoPos[9] = 145; servoPos[10] = 230; servoPos[11] = 200;
  servoPos[12] = 190; servoPos[13] = 170; servoPos[14] = 200; servoPos[15] = 100;
  
  Serial.println("✓ Stand position achieved");
}
