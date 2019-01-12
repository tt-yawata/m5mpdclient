/*
    Name:       m5mpdclient.ino
    Created:    2018/12/10
    Author:     tt.yawata@gmail.com
*/

#include <M5Stack.h>

#include "freertos/task.h"
#include "time.h"

// 日本語表示
// https://qiita.com/taront/items/7900c88b9e9782c33b08
#include <sdfonts.h>
#include <sdfontsConfig.h>
#define SD_PN 4

// Wifi
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiMulti.h>

WiFiClient client;
const char ssid[] = "Your SSID";
const char pass[] = "Your PASS";

//mpd server
// by IP address
//const char mpdserver[] = "IP:192.168.xxx.xxx";
//by server name
const char mpdserver[] = "moode";
//char mpdserver[] = "volumio";
//const char mpdserver[] = "smpd";

//mpdport
uint16_t mpdport = 6600;

// mDNS
#include <ESPmDNS.h>

IPAddress mpdaddr;
const char hostname[] = "m5mpdclint";
WiFiMulti WiFiMulti;


// Timer
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// IR remote controller
//https://qiita.com/tt-yawata/items/92adb5ddb19ff1d07178
#include "M5StackIRdecode.h"

const rmt_channel_t channel = RMT_CHANNEL_0;
const gpio_num_t irPin = GPIO_NUM_21;
RingbufHandle_t buffer;
String ircode = "";

M5StackIRdecode  irDec;

// IR remote command for
// https://www.akiba-led.jp/product/1489
const String ir_play = "N00ffc23d"; // pause
const String ir_stop = "N00ff6897";
const String ir_next = "N00ff02fd";
const String ir_prev = "N00ff22dd";
const String ir_home = "N00ffa25d";
const String ir_end = "N00ffe21d";
const String ir_repeat = "N00ff906f";
const String ir_random = "N00ff629d";
const String ir_volup = "N00ffa857";
const String ir_voldown = "N00ffe01f";


// Grobal valiables

String artist, partist;
String title, ptitle;
String file, pfile;
String name, pname;

static volatile uint16_t shiftreset_cnt = 0, mpd_poll_cnt = 0;
byte shift = 0, pshift = -1;
boolean randomf,prandomf = false, repeatf,prepeatf = false;
int16_t volume, pvolume = -2;
int16_t playlistlength, pplaylistlength = -1,  id, pid = -1;
String  mpd_stat, pmpd_stat="";

//-----------------------------<O>-----------------------------
// Mpd icons

void draw_play(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {

  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillTriangle(x, y, x + 29, y + 14, x, y + 14, fgcolor);
  M5.Lcd.fillTriangle(x, y + 15, x + 29, y + 15, x, y + 29, fgcolor);
}

void draw_next(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {

  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillTriangle(x, y,    x + 15, y + 14, x, y + 14, fgcolor);
  M5.Lcd.fillTriangle(x, y + 15, x + 15, y + 15, x, y + 29, fgcolor);

  M5.Lcd.fillTriangle(x + 15, y,    x + 29, y + 14, x + 15, y + 14, fgcolor);
  M5.Lcd.fillTriangle(x + 15, y + 15, x + 29, y + 15, x + 15, y + 29, fgcolor);
}

void draw_prev(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {

  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);

  M5.Lcd.fillTriangle(x + 15, y,    x,   y + 14, x + 15, y + 14, fgcolor);
  M5.Lcd.fillTriangle(x + 15, y + 15, x,   y + 15, x + 15, y + 29, fgcolor);

  M5.Lcd.fillTriangle(x + 29, y,    x + 15, y + 14, x + 29, y + 14, fgcolor);
  M5.Lcd.fillTriangle(x + 29, y + 15, x + 15, y + 15, x + 29, y + 29, fgcolor);
}

void draw_top(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillTriangle(x, y, x + 24, y + 14, x, y + 14, fgcolor);
  M5.Lcd.fillTriangle(x, y + 15, x + 24, y + 15, x, y + 29, fgcolor);
  M5.Lcd.fillRoundRect(x + 24, y, 6, 30, 2, WHITE);
}

void draw_end(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillRoundRect(x, y, 6, 30, 2, fgcolor);
  M5.Lcd.fillTriangle(x + 6, y + 14, x + 29, y, x + 29, y + 14, fgcolor);
  M5.Lcd.fillTriangle(x + 6, y + 15, x + 29, y + 15, x + 29, y + 29, fgcolor);

}

void draw_stop(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillRoundRect(x, y, 30, 30, 2, fgcolor);
}

void draw_pause(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillRoundRect(x, y, 12, 30, 2, fgcolor);
  M5.Lcd.fillRoundRect(x + 18, y, 12, 30, 2, fgcolor);
}

void draw_vup(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillCircle(x + 14, y + 15, 14, fgcolor);
  M5.Lcd.fillCircle(x + 14, y + 15, 12, bgcolor);
  M5.Lcd.fillCircle(x + 14, y + 15, 10, fgcolor);
  M5.Lcd.fillCircle(x + 14, y + 15, 8, bgcolor);
  M5.Lcd.fillCircle(x + 14, y + 15, 6, fgcolor);
  M5.Lcd.fillCircle(x + 14, y + 15, 4, bgcolor);
  M5.Lcd.fillRect(x, y, 15, 30, bgcolor);
  M5.Lcd.fillRoundRect(x, y + 10, 12, 11, 2, fgcolor);
  M5.Lcd.fillTriangle(x, y + 14, x + 12, y + 14, x + 12, y + 1, fgcolor);
  M5.Lcd.fillTriangle(x, y + 15, x + 12, y + 15, x + 12, y + 29, fgcolor);

}
void draw_vdn(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillCircle(x + 14, y + 15, 6, fgcolor);
  M5.Lcd.fillCircle(x + 14, y + 15, 4, bgcolor);
  M5.Lcd.fillRect(x, y, 15, 30, bgcolor);
  M5.Lcd.fillRoundRect(x, y + 10, 12, 11, 2, fgcolor);
  M5.Lcd.fillTriangle(x, y + 14, x + 12, y + 14, x + 12, y + 1, fgcolor);
  M5.Lcd.fillTriangle(x, y + 15, x + 12, y + 15, x + 12, y + 29, fgcolor);
}

void draw_repeat(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillRoundRect(x, y + 5, 30, 18, 8, fgcolor);
  M5.Lcd.fillRoundRect(x + 3, y + 8, 24, 12, 4, bgcolor);
  M5.Lcd.fillTriangle(x + 16, y + 2, x + 16, y + 11, x + 24, y + 5, fgcolor);
  M5.Lcd.fillTriangle(x + 12, y + 17, x + 12, y + 25, x + 6, y + 20, fgcolor);
}

void draw_norepeat(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillRoundRect(x, y + 13, 28, 3, 1, fgcolor);
  M5.Lcd.fillRect(x + 27, y + 8, 3, 14, fgcolor);
  M5.Lcd.fillTriangle(x + 19, y + 2 + 8, x + 19, y + 11 + 8, x + 27, y + 5 + 8, fgcolor);
}

void draw_shuffle(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillRoundRect(x, y + 6, 16, 16, 8, fgcolor);
  M5.Lcd.fillRoundRect(x + 3, y + 6 + 3, 10, 10, 4, bgcolor);
  M5.Lcd.fillRoundRect(x + 14, y + 6, 16, 16, 8, fgcolor);
  M5.Lcd.fillRoundRect(x + 14 + 3, y + 6 + 3, 10, 10, 4, bgcolor);
  M5.Lcd.fillRect(x, y + 6, 8, 16, fgcolor);
  M5.Lcd.fillRect(x, y + 9, 8, 10, bgcolor);
  M5.Lcd.fillRect(x + 22, y + 6, 8, 16, fgcolor);
  M5.Lcd.fillRect(x + 22, y + 9, 8, 10, bgcolor);
  M5.Lcd.fillTriangle(x + 22, y + 3, x + 22, y + 12, x + 29, y + 6, fgcolor);
  M5.Lcd.fillTriangle(x + 22, y + 3 + 13, x + 22, y + 12 + 13, x + 29, y + 6 + 13, fgcolor);
}

void draw_noshuffle(int16_t x, int16_t y, uint16_t fgcolor, uint16_t bgcolor) {
  M5.Lcd.fillRect(x, y, 30, 30, bgcolor);
  M5.Lcd.fillRoundRect(x, y + 13, 28, 3, 1, fgcolor);
  M5.Lcd.fillTriangle(x + 19, y + 2 + 8, x + 19, y + 11 + 8, x + 27, y + 5 + 8, fgcolor);
}

//-----------------------------<O>-----------------------------
// <<日本語表示>>

// buf(in) : フォント格納アドレス
// ビットパターン表示
// d: 8ビットパターンデータ
void fontDisp(uint16_t x, uint16_t y, uint8_t* buf) {
  uint16_t txt_color = TFT_WHITE;
  uint16_t bg_color = TFT_BLACK;

  uint8_t bn = SDfonts.getRowLength();               // 1行当たりのバイト数取得

  // Serial.print(SDfonts.getWidth(), DEC);            // フォントの幅の取得
  // Serial.print("x");
  // Serial.print(SDfonts.getHeight(), DEC);           // フォントの高さの取得
  // Serial.print(" ");
  // Serial.println((uint16_t)SDfonts.getCode(), HEX); // 直前し処理したフォントのUTF16コード表示

  for (uint8_t i = 0; i < SDfonts.getLength(); i += bn ) {
    for (uint8_t j = 0; j < bn; j++) {
      for (uint8_t k = 0; k < 8; k++) {
        if (buf[i + j] & 0x80 >> k) {
          M5.Lcd.drawPixel(x + 8 * j + k , y + i / bn, txt_color);
          //        } else {
          //          M5.Lcd.drawPixel(x + 8 * j + k , y + i / bn, bg_color);
        }
      }
    }
  }
}

// 指定した文字列を指定したサイズで表示する
// pUTF8(in) UTF8文字列
// sz(in)    フォントサイズ(8,10,12,14,16,20,24)
//void fontDump(uint16_t x, uint16_t y, char* pUTF8, uint8_t sz) {
void SfontDump(uint16_t x, uint16_t y, String s, uint8_t sz) {
  uint8_t buf[MAXFONTLEN]; // フォントデータ格納アドレス(最大24x24/8 = 72バイト)
  char sbuf[1024];
  char *pUTF8;

  s.toCharArray(sbuf, 1024);
  pUTF8 = &sbuf[0];
  SDfonts.open();                                   // フォントのオープン
  SDfonts.setFontSize(sz);                          // フォントサイズの設定
  uint16_t xpos = 0, szh;
  szh = sz / 2; // 半角
  while ( pUTF8 = SDfonts.getFontData(buf, pUTF8) ) { // フォントの取得
    fontDisp(x + xpos, y, buf);                 // フォントパターンの表示
    if (*(pUTF8 - 1) < 0x80) {
      xpos = xpos + szh; //半角
    } else {
      xpos = xpos + sz; //全角
    }
    if ((x + xpos) > 320 - sz) {
      xpos = 0;
      y = y + sz;
    }

  }
  //  Serial.println("");
  SDfonts.close();                                  // フォントのクローズ
}

//===================================< O >===================================
//LCD表示
void update_screen() {

  get_mpdstatus();
  get_currentsong();

  // current song
  if (name.length() > 0) { // Radio station
    M5.Lcd.fillRect(0, 175, 188, 20, BLACK); // erase play id
    if (!name.equals(pname)) {
      M5.Lcd.fillRect(0, 50, 320, 44, BLACK);
      SfontDump(0, 50, name, 20);
    }
    if (!title.equals(ptitle)) {
      M5.Lcd.fillRect(0, 100, 320, 64, BLACK);
      SfontDump(0, 100, title, 20);
    }
  } else { // file
    if (title.length() < 1) { // file
      if (!file.equals(pfile)) {
        M5.Lcd.fillRect(0, 50, 320, 44, BLACK);
        M5.Lcd.fillRect(0, 100, 320, 64, BLACK);
        SfontDump(0, 50, file, 20);
      }
    } else {
      if (!artist.equals(partist)) {
        M5.Lcd.fillRect(0, 50, 320, 44, BLACK);
        SfontDump(0, 50, artist, 20);
      }
      if (!title.equals(ptitle)) {
        M5.Lcd.fillRect(0, 100, 320, 64, BLACK);
        SfontDump(0, 100, title, 20);
      }
    }

    // play id
    if ((id != pid) || (playlistlength != pplaylistlength)) {
      M5.Lcd.fillRect(0, 175, 188, 18, BLACK); // erase play id
      M5.Lcd.setCursor(0, 175);
      M5.Lcd.printf("Playing:%d/%d", id, playlistlength);
    }
    
  }
  if (volume != pvolume) {
    M5.Lcd.fillRect(190, 175, 120, 20, BLACK); // volume
    M5.Lcd.setCursor(190, 175);
    M5.Lcd.printf("Volume:%d", volume);
  }

  pname = name;
  ptitle = title;
  pfile = file;
  partist = artist;
  pid = id;
  pplaylistlength = playlistlength;
  pvolume = volume;

  if ((shift != pshift)||
     (!mpd_stat.equals(pmpd_stat))||
     (randomf!=prandomf)||
     (repeatf!=prepeatf)) {
    M5.Lcd.fillRect(0, 195, 120, 40, BLACK); // erase menu
    M5.Lcd.fillRect(120, 195, 80, 40, DARKCYAN);
    M5.Lcd.fillRect(215, 195, 80, 40, DARKCYAN);
    M5.Lcd.setCursor(30, 205);
    switch (shift) {
      case 0:
        M5.Lcd.print("Play");
        if ( mpd_stat.equals("play")) {
          draw_pause(145, 200, WHITE, DARKCYAN);
        } else if (mpd_stat.equals("pause")) {
          draw_play(145, 200, BLUE, DARKCYAN);
        } else if (mpd_stat.equals("stop")) {
          draw_play(145, 200, WHITE, DARKCYAN);
        }
        draw_stop(240, 200, WHITE, DARKCYAN);
        break;
      case 1:
        M5.Lcd.print("Skip");
        draw_prev(145, 200, WHITE, DARKCYAN);
        draw_next(240, 200, WHITE, DARKCYAN);
        break;
      case 2:
        M5.Lcd.print("Top/End");
        draw_end(145, 200, WHITE, DARKCYAN);
        draw_top(240, 200, WHITE, DARKCYAN);
        break;
      case 3:
        M5.Lcd.print("Rnd/Rep");
        if (randomf) {
          draw_shuffle(145, 200, DARKCYAN, YELLOW);
        } else {
          draw_shuffle(145, 200, WHITE, DARKCYAN);
        }
        if (repeatf) {
          draw_repeat(240, 200, DARKCYAN, YELLOW);
        } else {
          draw_repeat(240, 200, WHITE, DARKCYAN);
        }
        break;
      case 4:
        M5.Lcd.print("Volume");
        draw_vdn(145, 200, WHITE, DARKCYAN);
        draw_vup(240, 200, WHITE, DARKCYAN);
        break;
    }
  }
  pshift = shift;
  pmpd_stat=mpd_stat;
  prandomf=randomf;
  prepeatf=repeatf;
}
//===================================< O >===================================
// mpd状態取得
void get_mpdstatus(void) {
  String resp,val;

  client.println("status");

  while (1) {
    resp = get_mpdresponse();
    if (resp.equals("OK")) {
      return;
    } else if(resp.equals("TOUT")){
      Serial.println("mpd timeout.");
      return;
    }

    val = resp.substring(resp.indexOf(' ')+1);
    if (resp.startsWith("state")) {
      mpd_stat = val;
    } else if (resp.startsWith("volume:")) {
      volume = val.toInt();
    } else if (resp.startsWith("playlistlength:")) {
      playlistlength = val.toInt();
    } else if (resp.startsWith("random: 0")) {
      randomf = false;
    } else if (resp.startsWith("random:")) {
      randomf = true;
    } else if (resp.startsWith("repeat: 0")) {
      repeatf = false;
    } else if (resp.startsWith("repeat:")) {
      repeatf = true;
    }
  }
}

//-----------------------------<O>-----------------------------
// 演奏中の楽曲情報取得

void get_currentsong(void) {
  String resp, val;

  client.println("currentsong");

  artist = "";
  title = "";
  file = "";
  name = "";

  while (1) {
    resp = get_mpdresponse();
    if (resp.equals("OK")) {
      return;
    } else if(resp.equals("TOUT")){
      Serial.println("mpd timeout.");
      return;
    }
    
    val = resp.substring(resp.indexOf(' ')+1);
    if (resp.startsWith("Id:")) {
      id = val.toInt();
    } else if (resp.startsWith("Artist:")) {
      artist = val;
    } else if (resp.startsWith("Title:")) {
      title = val;
    } else if (resp.startsWith("Name:")) {
      name = val;
    } else if (resp.startsWith("file:")) {
      file = val.substring(val.lastIndexOf('/') + 1, val.lastIndexOf('.'));
    }
  }
}

//-----------------------------<O>-----------------------------
// send mpd command

void send_command(String cmd) {
  client.println(cmd);
  get_mpdack();
}

// mpd ack
void get_mpdack() {
  String resp;
  while (1) {
    resp = get_mpdresponse();
    if (resp.startsWith("OK") || resp.startsWith("ACK")|| resp.startsWith("TOUT")) {
      return;
    }
  }
}
//-----------------------------<O>-----------------------------
String get_mpdresponse(void) {
  String response;
  int16_t totimer;

  totimer=40; // 40x50ms = 2sec
  while (1) {
    while (!client.available()) {
      if(totimer<1){
        return "TOUT";
      }
      M5.update();
      delay(50);
      totimer--;
    }
    char c = client.read();
    if (c == '\n') {
      return response;
    }
    response.concat(c);
  }
}

//===================================< O >===================================
// Timer interrupt

void set_shiftreset(uint16_t ms) {
  portENTER_CRITICAL_ISR(&timerMux);
  shiftreset_cnt = ms;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void set_mpd_poll(uint16_t ms) {
  portENTER_CRITICAL_ISR(&timerMux);
  mpd_poll_cnt = ms;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  if (shiftreset_cnt > 0) {
    shiftreset_cnt--;
  }
  if (mpd_poll_cnt > 0) {
    mpd_poll_cnt--;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

//===================================< O >===================================
void init_rmt(rmt_channel_t channel, gpio_num_t irPin) {
  rmt_config_t rmtConfig;

  rmtConfig.rmt_mode = RMT_MODE_RX;
  rmtConfig.channel = channel;
  rmtConfig.clk_div = 80;
  rmtConfig.gpio_num = irPin;
  rmtConfig.mem_block_num = 1;

  rmtConfig.rx_config.filter_en = 1;
  rmtConfig.rx_config.filter_ticks_thresh = 255;
  rmtConfig.rx_config.idle_threshold = 10000;

  rmt_config(&rmtConfig);
  rmt_driver_install(rmtConfig.channel, 2048, 0);

  rmt_get_ringbuf_handle(channel, &buffer);
  rmt_rx_start(channel, 1);
}

// Start IR remote controller task
void rmt_task(void *arg) {
  rmt_item32_t * item;
  size_t rxSize = 0;
  while (1) {
    item = (rmt_item32_t *)xRingbufferReceive(buffer, &rxSize, 10000);
    if (item) {
      if ((irDec.parseData(item, rxSize)) && ircode == "") {
        ircode = irDec._ircode;
      }
      vRingbufferReturnItem(buffer, (void*) item);
    }
  }
}
//===================================< O >===================================
// The setup() function runs once each time the micro-controller starts
void setup() {

  // init lcd, not init sd card, init serial(115200bps)
  M5.begin(true, false, true);

  M5.Lcd.clear(BLACK);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setTextSize(2);
  SDfonts.init(SD_PN);

  // WiFi connection
  WiFiMulti.addAP(ssid, pass);
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.printf(".");
  }
  M5.Lcd.println("wifi connected");
  // M5.Lcd.print("IP address: ");
  // M5.Lcd.println(WiFi.localIP());

  // resolve mpd host

  if (!strncmp(mpdserver, "IP:", 3)) {
    while (1) {
      if (client.connect(mpdserver + 3, mpdport)) {
        M5.Lcd.println("mpd connected");
        get_mpdack();
        break;
      } else {
        M5.Lcd.print(".");
      }
    }
  } else {
    MDNS.begin(hostname);
    while (1) {
      mpdaddr = MDNS.queryHost(mpdserver);
      Serial.println(mpdaddr);
      // mpd connection
      if (client.connect(mpdaddr, mpdport)) {
        MDNS.end();
        M5.Lcd.println("mpd connected");
        get_mpdack();
        break;
      } else {
        M5.Lcd.print(".");
      }
    }
  }

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000, true);
  timerAlarmEnable(timer);

  // IR remote
  init_rmt(channel, irPin);
  // xTaskCreate(rmt_task, "rmt_task", 8192, NULL, 1, NULL);
  xTaskCreatePinnedToCore(rmt_task, "rmtTask", 4096, NULL, 1, NULL, 1);

  // 画面初期設定

  update_screen();
  if (mpd_stat.equals("stop")) {
    send_command("play");
    send_command("stop");
  }
  update_screen();
}


//===================================< O >===================================
// Add the main program code into the continuous loop() function
void loop() {
  M5.update();

  // MPD polling 

  if (mpd_poll_cnt == 0) {
    set_mpd_poll(2000); // 2 sec
    update_screen();
  }

  if (M5.BtnA.wasPressed()) {
    shift++;
    if (shift > 4) {
      shift = 0;
    }
    update_screen();
  }

  // スイッチが押されている間はshift reset延期
  if (M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnC.isPressed()) {
    set_shiftreset(5000); // 5sec
  }
  if ((shiftreset_cnt == 0) && (shift != 0)) {
    shift = 0;
    update_screen();
  }

  //  Switch
  switch (shift) {
    case 0: // normal play/pause, stop
      if (M5.BtnB.wasPressed()) {
        if (mpd_stat.equals("pause") || mpd_stat.equals("stop")) {
          send_command("play");
        } else { // Pause
          send_command("pause");
        }
        update_screen();
      } else if (M5.BtnC.wasPressed()) {
        send_command("stop");
        update_screen();
      }
      break;
    case 1: // shift 1 FF , REV
      if (M5.BtnB.wasPressed()) {
        send_command("play");
        send_command("previous");
        if (mpd_stat.equals("stop")) {
          send_command("stop");
        }
        update_screen();
      } else if (M5.BtnC.wasPressed()) {
        send_command("play");
        send_command("next");
        if (mpd_stat.equals("stop")) {
          send_command("stop");
        }
        update_screen();
      }
      break;
    case 2: // shift 2  Home/End
      if (M5.BtnB.wasPressed()) {
        send_command("play 0");
        if (mpd_stat.equals("stop")) {
          send_command("stop");
        }
        update_screen();
      } else if (M5.BtnC.wasPressed()) {
        String cmd = "play ";
        cmd.concat(String(playlistlength - 1));
        send_command(cmd);
        if (mpd_stat.equals("stop")) {
          send_command("stop");
        }
        update_screen();
      }
      break;
    case 3: // random / repeat
      if (M5.BtnB.wasPressed()) {
        if (randomf) {
          send_command("random 0");
        } else {
          send_command("random 1");
        }
        update_screen();
      } else if (M5.BtnC.wasPressed()) {
        if (repeatf) {
          send_command("repeat 0");
        } else {
          send_command("repeat 1");
        }
        update_screen();
      }

      break;
    case 4: // shift 2 VolUp, Vol Down
      if (M5.BtnB.wasPressed()) {
        send_command("volume -5");
        update_screen();
      } else if (M5.BtnC.wasPressed()) {
        send_command("volume 5");
        update_screen();
      }
      break;
  }

  // IR remote
  if (ircode.length()) {
    Serial.println(ircode);
    if (ircode.equals(ir_play)) {
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      if (mpd_stat.equals("pause") || mpd_stat.equals("stop")) {
        send_command("play");
      } else { // Pause
        send_command("pause");
      }
      update_screen();
    } else if (ircode.equals(ir_stop)) {
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      send_command("stop");
      update_screen();
    } else if (ircode.equals(ir_prev)) {
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      shift = 1;
      send_command("play");
      send_command("previous");
      if (mpd_stat.equals("stop")) {
        send_command("stop");
      }
      update_screen();
    } else if (ircode.equals(ir_next)) {
      shift = 1;
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      send_command("play");
      send_command("next");
      if (mpd_stat.equals("stop")) {
        send_command("stop");
      }
      update_screen();
    } else if (ircode.equals(ir_home)) {
      shift = 2;
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      send_command("play 0");
      if (mpd_stat.equals("stop")) {
        send_command("stop");
      }
      update_screen();
    } else if (ircode.equals(ir_end)) {
      shift = 2;
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      String cmd = "play ";
      cmd.concat(String(playlistlength - 1));
      send_command(cmd);
      if (mpd_stat.equals("stop")) {
        send_command("stop");
      }
      update_screen();
    } else if (ircode.equals(ir_random)) {
      shift = 3;
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      if (randomf) {
        send_command("random 0");
      } else {
        send_command("random 1");
      }
      update_screen();
    } else if (ircode.equals(ir_repeat)) {
      shift = 3;
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      if (repeatf) {
        send_command("repeat 0");
      } else {
        send_command("repeat 1");
      }
      update_screen();
    } else if (ircode.equals(ir_volup)) {
      shift = 4;
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      send_command("volume 5");
      update_screen();
    } else if (ircode.equals(ir_voldown)) {
      shift = 4;
      M5.Lcd.fillCircle(15, 210, 8, WHITE);
      send_command("volume -5");
      update_screen();
    }

    set_shiftreset(800);
    ircode = "";
    M5.Lcd.fillCircle(15, 210, 8, BLACK);
  }
  
}
