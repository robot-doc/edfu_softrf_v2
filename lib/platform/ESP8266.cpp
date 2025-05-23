/*
 * Platform_ESP8266.cpp
 * Copyright (C) 2018-2025 Linar Yusupov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#if defined(ESP8266)

#include <Wire.h>
#include <ESP8266TrueRandom.h>
#include <SPI.h>

#include "../system/SoC.h"
#include "../driver/Sound.h"
#include "../driver/EEPROM.h"
#include "../driver/RF.h"
#include "../driver/WiFi.h"
#include "../driver/LED.h"
#include "../driver/GNSS.h"
#include "../driver/Battery.h"

#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>

// RFM95W pin mapping
lmic_pinmap lmic_pins = {
    .nss = SOC_GPIO_PIN_SS,
    .txe = LMIC_UNUSED_PIN,
    .rxe = LMIC_UNUSED_PIN,
    .rst = SOC_GPIO_PIN_RST,
    .dio = { LMIC_UNUSED_PIN, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN},
    .busy = SOC_GPIO_PIN_TXE,
    .tcxo = LMIC_UNUSED_PIN,
};

#if defined(USE_EXP_SW_SERIAL)
Exp_SoftwareSerial swSer(SOC_GPIO_PIN_GNSS_RX, SOC_GPIO_PIN_GNSS_TX, false, 256);
#else
SoftwareSerial swSer;
#endif

Adafruit_NeoPixel *strip;

void ICACHE_FLASH_ATTR user_init()
{   
  // init gpio subsytem
  gpio_init();
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10);
}

static void ESP8266_setup()
{
  Serial.begin(SERIAL_OUT_BR, SERIAL_OUT_BITS);

#if defined(USE_RADIOLIB)
  lmic_pins.dio[0] = SOC_GPIO_PIN_DIO0;
#endif /* USE_RADIOLIB */

  // Parameter 1 = number of pixels in strip
  // Parameter 2 = Arduino pin number (most are valid)
  // Parameter 3 = pixel type flags, add together as needed:
  //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
  //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
  //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
  //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

  strip = new Adafruit_NeoPixel(PIX_NUM, SOC_GPIO_PIN_LED, NEO_GRB + NEO_KHZ800);
}

static void ESP8266_post_init()
{

}

static void ESP8266_loop()
{

}

static void ESP8266_fini(int reason)
{
  if (strip) delete strip;
}

static void ESP8266_reset()
{
  ESP.restart();
}

static uint32_t ESP8266_getChipId()
{
#if !defined(SOFTRF_ADDRESS)
  uint32_t id = ESP.getChipId();

  return DevID_Mapper(id);
#else
  return (SOFTRF_ADDRESS & 0xFFFFFFFFU );
#endif
}

static void* ESP8266_getResetInfoPtr()
{
  return (void *) ESP.getResetInfoPtr();
}

static String ESP8266_getResetInfo()
{
  return ESP.getResetInfo();
}

static String ESP8266_getResetReason()
{
  return ESP.getResetReason();
}

static uint32_t ESP8266_getFreeHeap()
{
  return ESP.getFreeHeap();
}

static long ESP8266_random(long howsmall, long howBig)
{
  return ESP8266TrueRandom.random(howsmall, howBig);
}

static void ESP8266_Sound_test(int var)
{
  if (SOC_GPIO_PIN_BUZZER != SOC_UNUSED_PIN && settings->volume != BUZZER_OFF) {
//    Serial_GNSS_In.enableRx(false);

    if (var == REASON_DEFAULT_RST ||
        var == REASON_EXT_SYS_RST ||
        var == REASON_SOFT_RESTART) {
      tone(SOC_GPIO_PIN_BUZZER, 440, 500);delay(500);
      tone(SOC_GPIO_PIN_BUZZER, 640, 500);delay(500);
      tone(SOC_GPIO_PIN_BUZZER, 840, 500);delay(500);
      tone(SOC_GPIO_PIN_BUZZER, 1040, 500);
    } else if (var == REASON_WDT_RST) {
      tone(SOC_GPIO_PIN_BUZZER, 440, 500);delay(500);
      tone(SOC_GPIO_PIN_BUZZER, 1040, 500);delay(500);
      tone(SOC_GPIO_PIN_BUZZER, 440, 500);delay(500);
      tone(SOC_GPIO_PIN_BUZZER, 1040, 500);
    } else {
      tone(SOC_GPIO_PIN_BUZZER, 1040, 500);delay(500);
      tone(SOC_GPIO_PIN_BUZZER, 840, 500);delay(500);
      tone(SOC_GPIO_PIN_BUZZER, 640, 500);delay(500);
      tone(SOC_GPIO_PIN_BUZZER, 440, 500);
    }
    delay(600);
    noTone(SOC_GPIO_PIN_BUZZER);
    pinMode(SOC_GPIO_PIN_BUZZER, INPUT);

//    Serial_GNSS_In.enableRx(true);
  }
}

static void ESP8266_Sound_tone(int hz, uint8_t volume)
{
  if (SOC_GPIO_PIN_BUZZER != SOC_UNUSED_PIN && volume != BUZZER_OFF) {
    if (hz > 0) {
      tone(SOC_GPIO_PIN_BUZZER, hz, ALARM_TONE_MS);
    } else {
      noTone(SOC_GPIO_PIN_BUZZER);
      pinMode(SOC_GPIO_PIN_BUZZER, INPUT);
    }
  }
}

static uint32_t ESP8266_maxSketchSpace()
{
  return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
}

static void ESP8266_WiFi_set_param(int ndx, int value)
{
  switch (ndx)
  {
  case WIFI_PARAM_TX_POWER:
    WiFi.setOutputPower(value);
    break;
  case WIFI_PARAM_DHCP_LEASE_TIME:
    if (WiFi.getMode() == WIFI_AP) {
      wifi_softap_set_dhcps_lease_time((uint32) (value * 60)); /* in minutes */
    }
    break;
  default:
    break;
  }
}

static IPAddress ESP8266_WiFi_get_broadcast()
{
  struct ip_info ipinfo;
  IPAddress broadcastIp;

  if (WiFi.getMode() == WIFI_STA) {
    wifi_get_ip_info(STATION_IF, &ipinfo);
  } else {
    wifi_get_ip_info(SOFTAP_IF, &ipinfo);
  }
  broadcastIp = ~ipinfo.netmask.addr | ipinfo.ip.addr;

  return broadcastIp;
}

static void ESP8266_WiFi_transmit_UDP(int port, byte *buf, size_t size)
{
  IPAddress ClientIP;
  struct station_info *stat_info;
  WiFiMode_t mode = WiFi.getMode();

  switch (mode)
  {
  case WIFI_STA:
    ClientIP = ESP8266_WiFi_get_broadcast();

    Serial_GNSS_In.enableRx(false);

    if (Uni_Udp) {
      Uni_Udp->beginPacket(ClientIP, port);
      Uni_Udp->write(buf, size);
      Uni_Udp->endPacket();
    }

    Serial_GNSS_In.enableRx(true);

    break;
  case WIFI_AP:
    stat_info = wifi_softap_get_station_info();

    while (stat_info != NULL) {
      ClientIP = stat_info->ip.addr;

      Serial_GNSS_In.enableRx(false);

      if (Uni_Udp) {
        Uni_Udp->beginPacket(ClientIP, port);
        Uni_Udp->write(buf, size);
        Uni_Udp->endPacket();
      }

      Serial_GNSS_In.enableRx(true);

      stat_info = STAILQ_NEXT(stat_info, next);
    }
    wifi_softap_free_station_info();
    break;
  case WIFI_OFF:
  default:
    break;
  }
}

static void ESP8266_WiFiUDP_stopAll()
{
  WiFiUDP::stopAll();
}

static bool ESP8266_WiFi_hostname(String aHostname)
{
  return WiFi.hostname(aHostname);
}

static int ESP8266_WiFi_clients_count()
{
  struct station_info *stat_info;
  int clients = 0;
  WiFiMode_t mode = WiFi.getMode();

  switch (mode)
  {
  case WIFI_AP:
    stat_info = wifi_softap_get_station_info();

    while (stat_info != NULL) {
      clients++;

      stat_info = STAILQ_NEXT(stat_info, next);
    }
    wifi_softap_free_station_info();

    return clients;
  case WIFI_STA:
  default:
    return -1; /* error */
  }
}

static bool ESP8266_EEPROM_begin(size_t size)
{
  EEPROM.begin(size);
  return true;
}

static void ESP8266_EEPROM_extension(int cmd)
{
  if (cmd == EEPROM_EXT_LOAD) {
    /* AUTO and UK RF bands are deprecated since Release v1.3 */
    if (settings->band == RF_BAND_AUTO || settings->band == RF_BAND_UK) {
      settings->band = RF_BAND_EU;
    }
  }
}

static void ESP8266_SPI_begin()
{
  SPI.begin();
}

static void ESP8266_swSer_begin(unsigned long baud)
{
#if defined(USE_EXP_SW_SERIAL)
  Serial_GNSS_In.begin(baud);
#else
  Serial_GNSS_In.begin(baud, SWSERIAL_8N1, SOC_GPIO_PIN_GNSS_RX, SOC_GPIO_PIN_GNSS_TX, false, 256);
#endif
}

static void ESP8266_swSer_enableRx(boolean arg)
{
  Serial_GNSS_In.enableRx(arg);
}

static byte ESP8266_Display_setup()
{
  return DISPLAY_NONE;
}

static void ESP8266_Display_loop()
{

}

static void ESP8266_Display_fini(int reason)
{

}

static void ESP8266_Battery_setup()
{

}

static float ESP8266_Battery_param(uint8_t param)
{
  float rval;

  switch (param)
  {
  case BATTERY_PARAM_THRESHOLD:
    rval = BATTERY_THRESHOLD_NIMHX2;
    break;

  case BATTERY_PARAM_CUTOFF:
    rval = BATTERY_CUTOFF_NIMHX2;
    break;

  case BATTERY_PARAM_CHARGE:
    /* TBD */

    rval = 100;
    break;

  case BATTERY_PARAM_VOLTAGE:
  default:
    rval = analogRead (SOC_GPIO_PIN_BATTERY) / SOC_A0_VOLTAGE_DIVIDER ;
    break;
  }

  return rval;
}

void ESP8266_GNSS_PPS_Interrupt_handler()
{
  PPS_TimeMarker = millis();
}

static unsigned long ESP8266_get_PPS_TimeMarker()
{
  return PPS_TimeMarker;
}

static bool ESP8266_Baro_setup()
{

  if ((hw_info.rf != RF_IC_SX1276 && hw_info.rf != RF_IC_SX1262) ||
      RF_SX12XX_RST_is_connected) {
    return false;
  }

#if DEBUG
    Serial.println(F("INFO: RESET pin of SX12xx radio is not connected to MCU."));
#endif

  Wire.pins(SOC_GPIO_PIN_SDA, SOC_GPIO_PIN_SCL);

  return true;
}

static void ESP8266_UATSerial_begin(unsigned long baud)
{
  UATSerial.begin(baud);
}

static void ESP8266_UATModule_restart()
{
  /* TBD */
}

static void ESP8266_WDT_setup()
{
  /* TBD */
}

static void ESP8266_WDT_fini()
{
  /* TBD */
}

static void ESP8266_Button_setup()
{
  /* TODO */
}

static void ESP8266_Button_loop()
{
  /* TODO */
}

static void ESP8266_Button_fini()
{
  /* TODO */
}

const SoC_ops_t ESP8266_ops = {
  SOC_ESP8266,
  "ESP8266",
  ESP8266_setup,
  ESP8266_post_init,
  ESP8266_loop,
  ESP8266_fini,
  ESP8266_reset,
  ESP8266_getChipId,
  ESP8266_getResetInfoPtr,
  ESP8266_getResetInfo,
  ESP8266_getResetReason,
  ESP8266_getFreeHeap,
  ESP8266_random,
  ESP8266_Sound_test,
  ESP8266_Sound_tone,
  ESP8266_maxSketchSpace,
  ESP8266_WiFi_set_param,
  ESP8266_WiFi_transmit_UDP,
  ESP8266_WiFiUDP_stopAll,
  ESP8266_WiFi_hostname,
  ESP8266_WiFi_clients_count,
  ESP8266_EEPROM_begin,
  ESP8266_EEPROM_extension,
  ESP8266_SPI_begin,
  ESP8266_swSer_begin,
  ESP8266_swSer_enableRx,
  NULL, /* ESP8266 has no built-in Bluetooth */
  NULL, /* ESP8266 has no built-in USB */
  NULL,
  ESP8266_Display_setup,
  ESP8266_Display_loop,
  ESP8266_Display_fini,
  ESP8266_Battery_setup,
  ESP8266_Battery_param,
  ESP8266_GNSS_PPS_Interrupt_handler,
  ESP8266_get_PPS_TimeMarker,
  ESP8266_Baro_setup,
  ESP8266_UATSerial_begin,
  ESP8266_UATModule_restart,
  ESP8266_WDT_setup,
  ESP8266_WDT_fini,
  ESP8266_Button_setup,
  ESP8266_Button_loop,
  ESP8266_Button_fini,
  NULL
};

#endif /* ESP8266 */
