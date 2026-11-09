/*
  =====================================================================
   ROBOT TRANSPORTER V2 - Kontrol lewat KEYBOARD LAPTOP (Serial Monitor)
   Arduino Uno + Driver TA6586 + Servo
  =====================================================================
   Library: hanya Servo.h (bawaan Arduino IDE, tidak perlu download)

   Pembagian pin (sama dengan versi PS2):
     Motor A (kiri)  : IN1 = D5,  IN2 = D6   (driver D0, D1)
     Motor B (kanan) : IN3 = D3,  IN4 = D11  (driver D2, D3)
     Servo (sinyal)  : D9

   Cara pakai:
     1. Upload, lalu buka Serial Monitor (115200 baud).
     2. Ketik huruf perintah di kotak kirim, lalu tekan ENTER.
     3. Bisa juga beberapa perintah sekaligus, misal "2w" = level 2 lalu maju.

   PERINTAH GERAK:
     w = maju              s = mundur
     a = putar kiri        d = putar kanan
     q = maju belok kiri   e = maju belok kanan
     z = mundur belok kiri c = mundur belok kanan
     x atau spasi = STOP

   KECEPATAN:
     1 = pelan   2 = sedang   3 = cepat

   SERVO:
     i = naik 10 derajat   k = turun 10 derajat
     u = posisi ATAS       j = posisi TENGAH     m = posisi BAWAH
     v<angka> = sudut tertentu, contoh: v60

   LAIN-LAIN:
     p = tampilkan status    h = tampilkan bantuan
  =====================================================================
*/

#include <Servo.h>

// ---------------------- PIN ----------------------
#define PIN_IN1    5    // Motor A
#define PIN_IN2    6    // Motor A
#define PIN_IN3    3    // Motor B
#define PIN_IN4    11   // Motor B
#define PIN_SERVO  9

// ------------------- PENGATURAN ------------------
const bool BALIK_MOTOR_KIRI  = false;   // true kalau arah motor kiri terbalik
const bool BALIK_MOTOR_KANAN = false;   // true kalau arah motor kanan terbalik
const bool TUKAR_KIRI_KANAN  = false;   // true kalau motor kiri & kanan tertukar

const int SPEED_LEVEL[] = {80, 100, 180}; // pelan, sedang, cepat (maks 255)
int speedIndex = 1;                        // mulai dari "sedang"
const int RASIO_BELOK = 35;                // roda dalam saat belok = 35% kecepatan

// Servo (sesuaikan dengan mekanik gripper / lifter)
const int SERVO_BAWAH  = 0;
const int SERVO_TENGAH = 45;
const int SERVO_ATAS   = 90;
const int SERVO_STEP   = 10;               // derajat per tekan i / k

// Berhenti otomatis setelah X ms sejak perintah gerak terakhir.
// 0 = nonaktif (robot jalan terus sampai diberi perintah x / spasi).
// Contoh 500 = tiap perintah gerak membuat robot jalan 0,5 detik lalu berhenti.
const unsigned long AUTO_STOP_MS = 0;

// Tampilkan status otomatis tiap X ms selama robot bergerak (0 = nonaktif)
const unsigned long STATUS_BERKALA_MS = 1000;
// -------------------------------------------------

Servo servoAngkut;

int servoPos   = SERVO_TENGAH;
int motorKiri  = 0;
int motorKanan = 0;
char gerakTerakhir = 'x';
const char *namaGerak = "STOP";
unsigned long waktuGerak  = 0;
unsigned long lastStatus  = 0;

// ================= FUNGSI MOTOR =================
void setMotor(int pinA, int pinB, int speed) {
  speed = constrain(speed, -100, 100);
  if (speed > 0) {
    analogWrite(pinA, speed);
    digitalWrite(pinB, LOW);
  } else if (speed < 0) {
    digitalWrite(pinA, LOW);
    analogWrite(pinB, -speed);
  } else {
    digitalWrite(pinA, LOW);
    digitalWrite(pinB, LOW);
  }
}

void drive(int kiri, int kanan) {
  motorKiri  = kiri;
  motorKanan = kanan;
  if (TUKAR_KIRI_KANAN) { int t = kiri; kiri = kanan; kanan = t; }
  if (BALIK_MOTOR_KIRI)  kiri  = -kiri;
  if (BALIK_MOTOR_KANAN) kanan = -kanan;
  setMotor(PIN_IN1, PIN_IN2, kiri);
  setMotor(PIN_IN3, PIN_IN4, kanan);
}

// Jalankan gerakan sesuai huruf perintah, dengan kecepatan level saat ini
void jalankanGerak(char g) {
  int v  = SPEED_LEVEL[speedIndex];
  int vb = (long)v * RASIO_BELOK / 100;

  switch (g) {
    case 'w': drive( v,   v);   namaGerak = "MAJU";              break;
    case 's': drive(-v,  -v);   namaGerak = "MUNDUR";            break;
    case 'a': drive(-v,   v);   namaGerak = "PUTAR KIRI";        break;
    case 'd': drive( v,  -v);   namaGerak = "PUTAR KANAN";       break;
    case 'q': drive( vb,  v);   namaGerak = "MAJU BELOK KIRI";   break;
    case 'e': drive( v,   vb);  namaGerak = "MAJU BELOK KANAN";  break;
    case 'z': drive(-vb, -v);   namaGerak = "MUNDUR BELOK KIRI"; break;
    case 'c': drive(-v,  -vb);  namaGerak = "MUNDUR BELOK KANAN";break;
    default:  drive( 0,   0);   namaGerak = "STOP"; g = 'x';     break;
  }
  gerakTerakhir = g;
  waktuGerak = millis();
}

// ================= FUNGSI SERVO =================
void gerakServo(int sudut, const char *sebab) {
  int batasMin = min(SERVO_BAWAH, SERVO_ATAS);
  int batasMax = max(SERVO_BAWAH, SERVO_ATAS);
  bool mentok = (sudut < batasMin || sudut > batasMax);
  sudut = constrain(sudut, batasMin, batasMax);

  servoPos = sudut;
  servoAngkut.write(servoPos);

  Serial.print(F("[SERVO] "));
  Serial.print(servoPos);
  Serial.print(F(" deg  <- "));
  Serial.print(sebab);
  if (mentok) Serial.print(F("  (mentok batas)"));
  Serial.println();
}

// ================= TAMPILAN =================
void tampilkanStatus() {
  Serial.print(F("[STATUS] Gerak: "));  Serial.print(namaGerak);
  Serial.print(F(" | Motor L:"));       Serial.print(motorKiri);
  Serial.print(F(" R:"));               Serial.print(motorKanan);
  Serial.print(F(" | Level:"));         Serial.print(speedIndex + 1);
  Serial.print(F(" (PWM "));            Serial.print(SPEED_LEVEL[speedIndex]);
  Serial.print(F(") | Servo: "));       Serial.print(servoPos);
  Serial.println(F(" deg"));
}

void tampilkanBantuan() {
  Serial.println(F("\n============ KONTROL KEYBOARD ============"));
  Serial.println(F("  q   w   e      w/s = maju/mundur"));
  Serial.println(F("  a   x   d      a/d = putar kiri/kanan"));
  Serial.println(F("  z   s   c      q/e/z/c = belok sambil jalan"));
  Serial.println(F("                 x / spasi = STOP"));
  Serial.println(F("  1/2/3 = kecepatan pelan/sedang/cepat"));
  Serial.println(F("  i/k = servo naik/turun 10 deg"));
  Serial.println(F("  u/j/m = servo ATAS/TENGAH/BAWAH"));
  Serial.println(F("  v<angka> = servo ke sudut, contoh v60"));
  Serial.println(F("  p = status   h = bantuan"));
  Serial.println(F("  (ketik perintah lalu tekan ENTER)"));
  Serial.println(F("==========================================\n"));
}

// ================= BACA PERINTAH =================
void prosesPerintah(char c) {
  if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';   // huruf besar dianggap kecil

  switch (c) {
    // ---- gerak ----
    case 'w': case 's': case 'a': case 'd':
    case 'q': case 'e': case 'z': case 'c':
    case 'x': case ' ':
      jalankanGerak(c == ' ' ? 'x' : c);
      Serial.print(F("[MOTOR] "));
      Serial.print(namaGerak);
      Serial.print(F("  L:")); Serial.print(motorKiri);
      Serial.print(F(" R:"));  Serial.println(motorKanan);
      break;

    // ---- kecepatan ----
    case '1': case '2': case '3':
      speedIndex = c - '1';
      Serial.print(F("[SPEED] Level "));
      Serial.print(speedIndex + 1);
      Serial.print(F(" (PWM "));
      Serial.print(SPEED_LEVEL[speedIndex]);
      Serial.println(F(")"));
      // kalau sedang bergerak, langsung pakai kecepatan baru
      if (gerakTerakhir != 'x') jalankanGerak(gerakTerakhir);
      break;

    // ---- servo ----
    case 'i': gerakServo(servoPos + SERVO_STEP, "i (naik)");      break;
    case 'k': gerakServo(servoPos - SERVO_STEP, "k (turun)");     break;
    case 'u': gerakServo(SERVO_ATAS,   "u (posisi ATAS)");        break;
    case 'j': gerakServo(SERVO_TENGAH, "j (posisi TENGAH)");      break;
    case 'm': gerakServo(SERVO_BAWAH,  "m (posisi BAWAH)");       break;
    case 'v': {
      int sudut = Serial.parseInt();   // baca angka setelah huruf v
      gerakServo(sudut, "v (sudut manual)");
      break;
    }

    // ---- info ----
    case 'p': tampilkanStatus();  break;
    case 'h': case '?': tampilkanBantuan(); break;

    // ---- abaikan karakter Enter ----
    case '\r': case '\n': break;

    default:
      Serial.print(F("[?] Perintah tidak dikenal: '"));
      Serial.print(c);
      Serial.println(F("'  (ketik h untuk bantuan)"));
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);   // batas tunggu angka untuk perintah v<angka>

  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  drive(0, 0);

  servoAngkut.attach(PIN_SERVO);
  servoAngkut.write(servoPos);

  Serial.println(F("\n=== ROBOT TRANSPORTER V2 (KEYBOARD) ==="));
  if (AUTO_STOP_MS > 0) {
    Serial.print(F("Auto-stop aktif: robot berhenti "));
    Serial.print(AUTO_STOP_MS);
    Serial.println(F(" ms setelah perintah gerak."));
  }
  tampilkanBantuan();
  tampilkanStatus();
}

// ===================== LOOP =====================
void loop() {
  while (Serial.available()) {
    prosesPerintah(Serial.read());
  }

  // Auto-stop (kalau diaktifkan)
  if (AUTO_STOP_MS > 0 && gerakTerakhir != 'x' && millis() - waktuGerak > AUTO_STOP_MS) {
    jalankanGerak('x');
    Serial.println(F("[MOTOR] STOP (auto-stop)"));
  }

  // Status berkala selama robot bergerak
  if (STATUS_BERKALA_MS > 0 && gerakTerakhir != 'x' && millis() - lastStatus > STATUS_BERKALA_MS) {
    lastStatus = millis();
    tampilkanStatus();
  }
}
