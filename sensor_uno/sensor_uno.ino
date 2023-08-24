#include <Wire.h>
#include <DFRobot_ENS160.h>
#include <DFRobot_AHT20.h>
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_TFTLCD.h>  // Hardware-specific library

DFRobot_ENS160_I2C ENS160(&Wire, /*I2CAddr*/ 0x53);
DFRobot_AHT20 aht20;

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3  // Chip Select goes to Analog 3
#define LCD_CD A2  // Command/Data goes to Analog 2
#define LCD_WR A1  // LCD Write goes to Analog 1
#define LCD_RD A0  // LCD Read goes to Analog 0

#define LCD_RESET 0  // Can alternately just connect to Arduino's reset pin

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// Assign human-readable names to some common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define GRAY_BG 0x2125
#define GRAY_TEXT 0xa514
#define WHITE_TEXT 0xf7de

#define BLUE_BAR 0x057d
#define GREEN_BAR 0x0e44
#define YELLOW_BAR 0xffe0
#define ORANGE_BAR 0xfde0
#define RED_BAR 0xf1a1
#define GRAY_BAR 0x8c51

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
// If using the shield, all control and data lines are fixed, and
// a simpler declaration can optionally be used:
// Adafruit_TFTLCD tft;

void setup(void) {
  Serial.begin(115200);

  // Init the sensor
  while (NO_ERR != ENS160.begin()) {
    Serial.println("Communication with device failed, please check connection");
    delay(3000);
  }
  Serial.println("Begin ok!");

  /**
   * Set power mode
   * mode Configurable power mode:
   *   ENS160_SLEEP_MODE: DEEP SLEEP mode (low power standby)
   *   ENS160_IDLE_MODE: IDLE mode (low-power)
   *   ENS160_STANDARD_MODE: STANDARD Gas Sensing Modes
   */
  ENS160.setPWRMode(ENS160_STANDARD_MODE);

  /**
   * Users write ambient temperature and relative humidity into ENS160 for calibration and compensation of the measured gas data.
   * ambientTemp Compensate the current ambient temperature, float type, unit: C
   * relativeHumidity Compensate the current ambient temperature, float type, unit: %rH
   */
  ENS160.setTempAndHum(/*temperature=*/25.0, /*humidity=*/50.0);

  /**
   * @fn begin
   * @brief Initialize AHT20 sensor
   * @return Init status value
   * @n      0    Init succeeded
   * @n      1    _pWire is NULL, please check if the constructor DFRobot_AHT20 has correctly uploaded a TwoWire class object reference
   * @n      2    Device not found, please check if the connection is correct
   * @n      3    If the sensor init fails, please check if there is any problem with the sensor, you can call the reset function and re-initialize after restoring the sensor
   */
  uint8_t status;
  while ((status = aht20.begin()) != 0) {
    Serial.print("AHT20 sensor initialization failed. error status : ");
    Serial.println(status);
    delay(1000);
  }

  tft.reset();
  uint16_t identifier = tft.readID();
  tft.begin(identifier);

  // tft.setRotation(0);
  tft.fillScreen(GRAY_BG);
  delay(500);
  tft.setCursor(0, 0);
  tft.setTextColor(WHITE_TEXT, GRAY_BG);
  tft.setTextSize(3);
}

void loop() {
  float temp = 0, hum = 0;
  /**
   * @fn startMeasurementReady
   * @brief Start measurement and determine if it's completed.
   * @param crcEn Whether to enable check during measurement
   * @n     false  Measure without check (by default)
   * @n     true   Measure with check
   * @return Whether the measurement is done
   * @n     true  If the measurement is completed, call a related function such as get* to obtain the measured data.
   * @n     false If the measurement failed, the obtained data is the data of last measurement or the initial value 0 if the related function such as get* is called at this time.
   */
  if (aht20.startMeasurementReady(/* crcEn = */ true)) {
    temp = aht20.getTemperature_C();
    hum = aht20.getHumidity_RH();
    Serial.print("temperature: ");
    // Get temp in Celsius (℃), range -40-80℃
    Serial.print(temp);
    Serial.print(" C, ");
    Serial.print("humidity: ");
    // Get relative humidity (%RH), range 0-100℃
    Serial.print(hum);
    Serial.println(" %RH");
  }

  ENS160.setTempAndHum(temp, hum);

  /**
   * Get the sensor operating status
   * Return value: 0-Normal operation, 
   *         1-Warm-Up phase, first 3 minutes after power-on.
   *         2-Initial Start-Up phase, first full hour of operation after initial power-on. Only once in the sensor’s lifetime.
   * note: Note that the status will only be stored in the non-volatile memory after an initial 24h of continuous
   *       operation. If unpowered before conclusion of said period, the ENS160 will resume "Initial Start-up" mode
   *       after re-powering.
   */
  uint8_t Status = ENS160.getENS160Status();
  Serial.print("Sensor operating status : ");
  Serial.println(Status);

  /**
   * Get the air quality index
   * Return value: 1-Excellent, 2-Good, 3-Moderate, 4-Poor, 5-Unhealthy
   */
  uint8_t AQI = ENS160.getAQI();
  Serial.print("Air quality index : ");
  Serial.println(AQI);

  /**
   * Get TVOC concentration
   * Return value range: 0–65000, unit: ppb
   < 50
   50 - 100
   100 - 500
   > 500
   */
  uint16_t TVOC = ENS160.getTVOC();
  Serial.print("Concentration of total volatile organic compounds : ");
  Serial.print(TVOC);
  Serial.println(" ppb");

  /**
   * Get CO2 equivalent concentration calculated according to the detected data of VOCs and hydrogen (eCO2 – Equivalent CO2)
   * Return value range: 400–65000, unit: ppm
   * Five levels: Excellent(400 - 600), Good(600 - 800), Moderate(800 - 1000), 
   *               Poor(1000 - 1500), Unhealthy(> 1500)
   */
  uint16_t ECO2 = ENS160.getECO2();
  Serial.print("Carbon dioxide equivalent concentration : ");
  Serial.print(ECO2);
  Serial.println(" ppm");

  Serial.println();

  tft.setCursor(0, 0);
  tft.print(temp);
  tft.println(" C");

  tft.print(hum);
  tft.println(" %");

  tft.println();

  tft.setTextSize(2);
  switch (Status) {
    case 0:
      tft.println("Normal          ");
      break;
    case 1:
      tft.println("Warm-Up         ");
      break;
    case 2:
      tft.println("Initial Start-Up");
      break;
  }
  tft.setTextSize(3);

  tft.println();

  tft.setTextColor(GRAY_TEXT);
  tft.print("AQI: ");
  tft.setTextColor(WHITE_TEXT, GRAY_BG);
  tft.println(AQI);

  tft.println();

  char buf[32];
  uint16_t bar_color = WHITE;

  tft.setTextColor(GRAY_TEXT);
  tft.println("TVOC: ");
  tft.setTextColor(WHITE_TEXT, GRAY_BG);
  sprintf(buf, "%05u", TVOC);
  tft.print(buf);
  tft.println(" ppb");
  if (TVOC < 50) {
    bar_color = GREEN_BAR;
  } else if (TVOC < 100) {
    bar_color = YELLOW_BAR;
  } else if (TVOC < 500) {
    bar_color = ORANGE_BAR;
  } else {
    bar_color = RED_BAR;
  }
  drawBar(210, 0, 500, TVOC, bar_color, GRAY_BAR);

  tft.println();

  tft.setTextColor(GRAY_TEXT);
  tft.println("ECO2: ");
  tft.setTextColor(WHITE_TEXT, GRAY_BG);
  sprintf(buf, "%05u", ECO2);
  tft.print(buf);
  tft.println(" ppm");
  if (ECO2 < 600) {
    bar_color = BLUE_BAR;
  } else if (ECO2 < 800) {
    bar_color = GREEN_BAR;
  } else if (ECO2 < 1000) {
    bar_color = YELLOW_BAR;
  } else if (ECO2 < 1500) {
    bar_color = ORANGE_BAR;
  } else {
    bar_color = RED_BAR;
  }
  drawBar(282, 0, 1500, ECO2, bar_color, GRAY_BAR);

  delay(2000);
}

void drawBar(uint16_t y, uint16_t start, uint16_t end, uint16_t value, uint16_t color, uint16_t color_bg) {
  uint16_t w = tft.width();
  uint16_t drawW = map(value, start, end, 0, w);
  tft.fillRect(0, y, drawW, 10, color);
  tft.fillRect(drawW, y, w - drawW, 10, color_bg);
}
