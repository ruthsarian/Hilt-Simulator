#define BLADE_DATA_PIN  4       // cap this to 3.3V, use a voltage divider or something
#define BUTTON_PIN      2       // only pin on the ATtiny85 that has an interrupt
#define BUTTON_INT      0
#define LED_PIN         0

#define SIG_PREAMBLE_DLY      17    // ms
#define SIG_SHORT             1200  // uS
#define SIG_LONG              2400  // uS
#define SIG_ACTIVE            LOW
#define SIG_IDLE              HIGH

#define LAZY_DEBOUNCE 10

// send command to blade by alternating blade data pin state between high and low
void send_cmd(uint8_t cmd) {
  uint8_t i;
  uint16_t sig[16];

  // convert each bit of cmd into a pair of delay values for the signal
  // first value (even) is the idle period, second value (odd) is the active period
  // these values are precalculated to remove unwanted delays during signal transmission caused by calculating these values 
  for(i=0; i<8; i++) {

    // 1 = LONG idle, SHORT active
    if ((cmd >> 7-i) & 1) {
      sig[(2*i)] = SIG_LONG;
      sig[(2*i)+1] = SIG_SHORT;

    // 0 = SHORT idle, LONG active
    } else {
      sig[(2*i)] = SIG_SHORT;  
      sig[(2*i)+1] = SIG_LONG;
    }
  }

  // send the preamble that lets the blade know a command is coming
  digitalWrite(BLADE_DATA_PIN, SIG_ACTIVE);
  delay(SIG_PREAMBLE_DLY);
  digitalWrite(BLADE_DATA_PIN, SIG_IDLE);
  delay(SIG_PREAMBLE_DLY);
  digitalWrite(BLADE_DATA_PIN, SIG_ACTIVE);
  delay(SIG_PREAMBLE_DLY);
  digitalWrite(BLADE_DATA_PIN, SIG_IDLE);

  // send the command
  for(i=0;i<8;i++){
    digitalWrite(BLADE_DATA_PIN, SIG_IDLE);
    delayMicroseconds(sig[(2*i)]);
    digitalWrite(BLADE_DATA_PIN, SIG_ACTIVE);
    delayMicroseconds(sig[(2*i)+1]);
  }

  // put the data pin back to IDLE when finished
  digitalWrite(BLADE_DATA_PIN, SIG_IDLE);
}

volatile uint8_t button_triggered = 0;
void button_press() {
  static uint32_t last_press_time = 0;
  if (millis() - last_press_time > LAZY_DEBOUNCE) {
    button_triggered = 1;
    last_press_time = millis();
  }
}

void setup() {
  uint8_t i;
  pinMode(BLADE_DATA_PIN, OUTPUT);
  digitalWrite(BLADE_DATA_PIN, SIG_IDLE);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_INT, button_press, FALLING);

  pinMode(LED_PIN, OUTPUT);

  for (i=0;i<3;i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
  digitalWrite(LED_PIN, HIGH);

}

void loop() {
  static uint8_t cmd = 0x20;

  if (button_triggered == 1) {
    button_triggered = 0;

    digitalWrite(LED_PIN, LOW);

    // send command
    send_cmd(cmd);          // ignite
    delay(2500);
    send_cmd(cmd + 0xA0);   // clash
    delay(500);
    send_cmd(cmd + 0x20);   // extinguish
    delay(2500);

    // increment to next command
    if (cmd == 0x3F) {
        cmd = 0x20;
    } else {
      cmd++;
    }

    digitalWrite(LED_PIN, HIGH);
  }
}
