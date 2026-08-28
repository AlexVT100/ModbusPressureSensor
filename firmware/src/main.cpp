#include <Adafruit_SSD1306.h> // OLED display
#include <SoftwareSerial.h>   // Software UART
#include <Wire.h>

#include "ModbusHelper.h"
#include "WiFiHelper.h"

#include "Config.h"
#include "DebouncedButton.h"
#include "TelnetServer.h"
#include "TerminalLogger.h"

#include "States.h"
#include "bitmaps.h"
#include "functions.h"

//=============================================================================
// General Settings
//=============================================================================

// SSD1306 display settings
// Character dimensions (WxH):
//      Text size 1: 5x7 (vertical spacing 1)
//      Text size 3: 15x21 (vertical spacing 2)
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1 // Sharing Arduino reset pin
#define SCREEN_ADDRESS 0x3C

// Board-dependent values (discover_board.py)
#if defined(BOARD_1A86_7523) // Development board HW364A with a display and USB-C
#    define OLED_SDA     14  // D6
#    define OLED_SCL     12  // D5
#    define SERIAL_SPEED 115200
#elif defined(BOARD_0403_6001)  // No name board with MicroUSB used in the sensor assembly
#    define OLED_SDA     4      // D2
#    define OLED_SCL     5      // D1
#    define SERIAL_SPEED 115200 // 74880
#else                           // Defaults
#    define OLED_SDA     4      // D2
#    define OLED_SCL     5      // D1
#    define SERIAL_SPEED 115200
#    warning No board is connected. The image may not work as expected.
#endif

// The button pin. D5, D6 and D7 are available
#define BUTTON_PIN D7

//=============================================================================
// Global variables and containers
//=============================================================================

// Timers
static constexpr ulong SensorInterval = 500;    // Sensor polling frequency
static constexpr ulong DisplayInterval = 60000; // Display off timeout

// millis() rollover counter for full uptime counting
ulong RolloverCount = 0;

// Display state
DisplayState DispState;

// The sensor values
SensorState Sensor;

//=============================================================================
// Global objects
//=============================================================================

// SSD-1306 display object
Adafruit_SSD1306 Display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Button press detector with debouncing
DebouncedButton Button(BUTTON_PIN);

// Wi-Fi object
WiFiHelperClass WiFiHelper("Press_Sensor_AP", Display);

// Config object
// Default settings are hardcoded in FileConfig::_setDefaults in Config.h
FileConfig Conf;

// Modbus object
ModbusHelperClass ModbusHelper;

// Telnet terminal and Logger
TelnetServer Telnet(8023);
TerminalLogger Logger(Telnet);

//=============================================================================
// Forward declarations for the local functions
//=============================================================================

void blinkLEDForever();
void readSensor();
void updateDisplay();
void drawThrobber(int x, int y, int w, int h);

inline void setLEDOn() { digitalWrite(LED_BUILTIN, LOW); }
inline void setLEDOff() { digitalWrite(LED_BUILTIN, HIGH); }

// #############################################################################
//  Initialization
// #############################################################################
//
void setup() {
    Serial.begin(SERIAL_SPEED);
    while (!Serial && millis() < 3000);
    Serial.flush();

    Serial.println("\nInitialization...");

    // Switch on the built-in LED
    pinMode(LED_BUILTIN, OUTPUT);
    setLEDOn();

    // Arm the button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Button.begin();

    // Init the display
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!Display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        // Fatal error
        blinkLEDForever();
    }
    Display.clearDisplay();
    Display.setTextSize(1);
    Display.setTextColor(SSD1306_WHITE);

    // Init the WIFI
    if (digitalRead(BUTTON_PIN) == LOW) {
        Display.clearDisplay();
        Display.setCursor(0, 0);
        Display.println("Button pressed");
        Display.println("Starting WiFi Manager");
        Display.display();
        delay(2000);
        WiFiHelper.startWiFiManager();
    } else {
        if (!WiFiHelper.connectToSavedWiFi()) {
            delay(2000);
            WiFiHelper.startWiFiManager();
        }
    }

    // Initialize the telnet server and te logger
    Telnet.setup();
    Logger.setup();

    // Load the configuration
    Conf.load();

    // Start the Modbus server and configure the registers
    ModbusHelper.setup();

    // Switch off the LED
    setLEDOff();

    Serial.println("Ready");
}

// #############################################################################
//  The main loop
// #############################################################################
//
void loop() {
    static unsigned long lastMillis = millis();
    static unsigned long sensorTimer = lastMillis;
    static unsigned long displayTimer = lastMillis;

    unsigned long curMillis = millis();

    // Check if millis() rolled over back to 0
    if (curMillis < lastMillis) RolloverCount++;
    lastMillis = curMillis;

    if (curMillis - sensorTimer >= SensorInterval) {
        sensorTimer = curMillis; // Set the timer

        // Read the sensor and update the display and the Modbus registry
        readSensor();
        updateDisplay();
        ModbusHelper.pressure(Sensor.pressure >= 0 ? Sensor.pressure : 0);
        ModbusHelper.status(Sensor.status);
    }

    if (Button.wasPressed() || DispState.setOn) {
        // Switch on the display
        displayTimer = curMillis; // Set the timer
        Display.dim(false);
        DispState.isOn = true;
        DispState.setOn = false;
    }

    if (DispState.isOn && curMillis - displayTimer >= DisplayInterval) {
        // Switch off the display
        Display.dim(true);
        DispState.isOn = false;
    }

    Telnet.loop();
    ModbusHelper.loop();
    Conf.save();
}

//-----------------------------------------------------------------------------
// Read the sensor
//-----------------------------------------------------------------------------
//
void readSensor() {
    //
    // Read the ADC value with oversampling to kill WiFi/OLED noise
    //
    const uint8_t samples = Conf.filtSamps();
    uint32_t totalRaw = 0;

    for (uint8_t i = 0; i < samples; i++) {
        totalRaw += analogRead(A0);
        delay(2); // Allow the ADC input RC-filter impedance to settle
    }
    Sensor.adcValue = (totalRaw + samples / 2) / samples;

    //
    // Calculate the pressure (mbar)
    //
    if (Sensor.adcValue > Conf.adcMinServ()) {
        Sensor.pressRaw =
            scale(Sensor.adcValue, Conf.scalerAmin(), Conf.scalerAmax(), Conf.scalerPmin(), Conf.scalerPmax());
        if (Sensor.pressRaw < 0) {
            Sensor.pressure = 0;
        } else {
            // Pressure is rounded to tens
            Sensor.pressure = ema_filter(Sensor.pressRaw, Conf.filtAlpha());
            Sensor.pressure = ((Sensor.pressure + 5) / 10) * 10;
        }

        // Set the sensor status
        if (Sensor.status == Status::LO_PRESS) {
            if (Sensor.pressure > Conf.alertLo() + Conf.alertHyst()) Sensor.status = Status::NORMAL;
        } else if (Sensor.status == Status::HI_PRESS) {
            if (Sensor.pressure + Conf.alertHyst() < Conf.alertHi()) Sensor.status = Status::NORMAL;
        } else {
            if (Sensor.pressure < Conf.alertLo())
                Sensor.status = Status::LO_PRESS;
            else if (Sensor.pressure > Conf.alertHi())
                Sensor.status = Status::HI_PRESS;
            else
                Sensor.status = Status::NORMAL;
        }
    } else {
        Sensor.pressure = 0;
        Sensor.status = Status::FAILURE;
    }
}

//-----------------------------------------------------------------------------
// Update the display
//-----------------------------------------------------------------------------
//
void updateDisplay() {

    if (!DispState.isOn) return;

    Display.clearDisplay();
    Display.setCursor(0, 0);
    Display.setTextColor(SSD1306_WHITE);

    // Display.drawRect(0, 0, 128, 64, SSD1306_WHITE);

    // Print the WiFi info
    Display.setTextSize(1);
    Display.print("WiFi: ");
    Display.println(WiFiHelper.SSID());
    Display.print("  IP: ");
    Display.println(WiFiHelper.localIP());

    Display.println("");

    // Draw the icon depending on the Sensor.pressure value
    const unsigned char *bitmap;
    switch (Sensor.status) {
        case Status::NORMAL: // The Sensor.pressure is normal
            setLEDOff();
            bitmap = bitmap_none;
            break;
        case Status::LO_PRESS: // The Sensor.pressure is too low
            setLEDOn();
            bitmap = bitmap_low;
            break;
        case Status::HI_PRESS: // The Sensor.pressure is too high
            setLEDOn();
            bitmap = bitmap_high;
            break;
        default: // Just in case
            Sensor.status = Status::FAILURE;
        case Status::FAILURE: // The sensor failure
            setLEDOn();
            bitmap = bitmap_fault;
            break;
    }
    Display.drawBitmap(Display.getCursorX(), Display.getCursorY(), bitmap, BITMAP_W, BITMAP_H, SSD1306_WHITE);

    // Display the Sensor.pressure
    if (Sensor.status != Status::FAILURE) {
        Display.setTextSize(3);
        Display.setCursor(BITMAP_W + 8, Display.getCursorY());
        Display.printf("%u.%02u", Sensor.pressure / 1000, Sensor.pressure % 1000 / 10);
        // Display.setCursor(Display.getCursorX(), 38);  // Uncomment to align the next line on the bottom
        Display.setTextSize(1);
        Display.print(" bar");
    } else {
        Display.setTextSize(2);
        Display.setCursor(BITMAP_W + 8, Display.getCursorY() - 4);
        Display.println("Sensor");
        Display.setCursor(BITMAP_W + 8, Display.getCursorY() - 2);
        Display.print("failure");
    }

    // The debug info in the bottom line
    Display.setCursor(0, SCREEN_HEIGHT - 8);
    Display.setTextSize(1);
    Display.printf("M%d T%d A%04d %04d/%04d", ModbusHelper.connCount(), Telnet.isConnected(), Sensor.adcValue,
                   Conf.alertLo(), Conf.alertHi());

    // Update the throbber
    drawThrobber(122, 0, 6, 6);

    // Redraw the screen
    Display.display();
}

//-----------------------------------------------------------------------------
// Blink LED forever on fatal error. The function never returns.
//-----------------------------------------------------------------------------
//
void blinkLEDForever() {
    while (1) {
        setLEDOn();
        delay(250);
        setLEDOff();
        delay(250);
    }
}

//-----------------------------------------------------------------------------
// The throbber
//-----------------------------------------------------------------------------
//
void drawThrobber(int x, int y, int w, int h) {
    static int step = 0;

    // Draw on each call
    if (step % 2)
        Display.drawRect(x, y, w, h, SSD1306_WHITE);
    else
        Display.fillRect(x, y, w, h, SSD1306_WHITE);

    step = (step + 1) % 2; // 0 or 1 in rotation
}
