/*
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║   N O V A   S T R I K E  —  Arcade Space Shooter                     ║
 * ║   Xiao ESP32-S3  ·  SSD1306 0.96" OLED  ·  MAX98357A Speaker         ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║  WIRING                                                               ║
 * ║  ─────────────────────────────────────────────────────────────────── ║
 * ║  OLED  SDA  → GPIO5  (D4)                                            ║
 * ║  OLED  SCL  → GPIO6  (D5)                                            ║
 * ║                                                                       ║
 * ║  BTN_UP     → GPIO9  (D10)  [TTP223 or push switch + 10k pull-down]  ║
 * ║  BTN_DOWN   → GPIO1  (D6)   [TTP223 or push switch + 10k pull-down]  ║
 * ║  BTN_FIRE   → GPIO44 (D7)   [TTP223 or push switch + 10k pull-down]  ║
 * ║  BTN_BOMB   → GPIO3  (D2)   [TTP223 or push switch + 10k pull-down]  ║
 * ║                                                                       ║
 * ║  SPK BCLK   → GPIO7  (D8)                                            ║
 * ║  SPK LRC    → GPIO4  (D3)                                            ║
 * ║  SPK DIN    → GPIO2  (D1)                                            ║
 * ║                                                                       ║
 * ║  NOTE: GPIO2 reserved for SPK DIN — DOWN button on GPIO1 (D6)         ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║  CONTROLS                                                             ║
 * ║  UP    → move ship up          (hold for continuous)                  ║
 * ║  DOWN  → move ship down        (hold for continuous)                  ║
 * ║  FIRE  → shoot laser           (hold = rapid fire)                   ║
 * ║  BOMB  → smart bomb (3 lives)  clears all enemies on screen          ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║  GAMEPLAY                                                             ║
 * ║  · 3 enemy types: Drifter, Hunter, Kamikaze                         ║
 * ║  · Power-ups: Shield, TriShot, SpeedBoost, BombRefill               ║
 * ║  · Wave system — every 10 kills = new wave, enemies get faster      ║
 * ║  · Boss every 3 waves — massive sprite, multi-hit                   ║
 * ║  · Lives: 3  ·  Bombs: 3                                            ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║  Libraries (Arduino Library Manager):                                 ║
 * ║    Adafruit SSD1306  ·  Adafruit GFX Library                        ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>
#include <math.h>

// ═══════════════════════════════════════════════════════════
//  FORWARD DECLARATIONS
// ═══════════════════════════════════════════════════════════
void hitPlayer();
void returnToTitle();

// ═══════════════════════════════════════════════════════════
//  DISPLAY
// ═══════════════════════════════════════════════════════════
#define SW  128
#define SH   64
Adafruit_SSD1306 oled(SW, SH, &Wire, -1);

// ═══════════════════════════════════════════════════════════
//  BUTTONS  (HIGH = pressed — works for TTP223 & pull-down switches)
// ═══════════════════════════════════════════════════════════
#define BTN_UP    9   // GPIO9  D10
#define BTN_DOWN  1   // GPIO1  D6
#define BTN_FIRE  44  // GPIO44 D7
#define BTN_BOMB  3   // GPIO3  D2

struct Button {
  uint8_t pin;
  bool    cur, prev, edge, held;
  uint8_t holdCount;
} btns[4];

void initButtons() {
  uint8_t pins[] = { BTN_UP, BTN_DOWN, BTN_FIRE, BTN_BOMB };
  for (int i = 0; i < 4; i++) {
    btns[i].pin = pins[i];
    pinMode(pins[i], INPUT);
    btns[i] = {pins[i], false, false, false, false, 0};
  }
}

void readButtons() {
  for (int i = 0; i < 4; i++) {
    btns[i].prev = btns[i].cur;
    btns[i].cur  = digitalRead(btns[i].pin) == HIGH;
    btns[i].edge = btns[i].cur && !btns[i].prev;
    if (btns[i].cur) btns[i].holdCount++;
    else             btns[i].holdCount = 0;
    btns[i].held = (btns[i].holdCount > 6);
  }
}

#define UP_PRESSED   (btns[0].cur)
#define DOWN_PRESSED (btns[1].cur)
#define FIRE_EDGE    (btns[2].edge || (btns[2].held && frameCount % 4 == 0))
#define BOMB_EDGE    (btns[3].edge)
#define ANY_EDGE     (btns[0].edge||btns[1].edge||btns[2].edge||btns[3].edge)

// ═══════════════════════════════════════════════════════════
//  SPEAKER
// ═══════════════════════════════════════════════════════════
#define SPK_PORT  I2S_NUM_0
#define SPK_BCLK  7
#define SPK_LRC   4
#define SPK_DIN   2
#define SPK_RATE  16000

void setupSpeaker() {
  i2s_config_t c = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_TX),
    .sample_rate = SPK_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4, .dma_buf_len = 256,
    .use_apll = false, .tx_desc_auto_clear = true
  };
  i2s_pin_config_t p = { SPK_BCLK, SPK_LRC, SPK_DIN, I2S_PIN_NO_CHANGE };
  i2s_driver_install(SPK_PORT, &c, 0, NULL);
  i2s_set_pin(SPK_PORT, &p);
}

// Non-blocking tone queue
struct ToneNote { float freq; uint16_t dur; float vol; };
#define TONE_Q_SIZE 8
ToneNote toneQ[TONE_Q_SIZE];
int toneQHead = 0, toneQTail = 0;
bool tonePlaying = false;
int  toneSamplesLeft = 0;
float tonePhase = 0, toneInc = 0, toneVol = 0;
int16_t toneBuf[128];

void queueTone(float f, uint16_t ms, float v = 0.25f) {
  int next = (toneQTail + 1) % TONE_Q_SIZE;
  if (next == toneQHead) return; // full, drop
  toneQ[toneQTail] = {f, ms, v};
  toneQTail = next;
}

void tickAudio() {
  if (!tonePlaying) {
    if (toneQHead == toneQTail) return;
    ToneNote& n = toneQ[toneQHead];
    toneQHead = (toneQHead + 1) % TONE_Q_SIZE;
    toneSamplesLeft = SPK_RATE * n.dur / 1000;
    toneInc  = 2.0f * M_PI * n.freq / SPK_RATE;
    toneVol  = n.vol;
    tonePhase = 0;
    tonePlaying = true;
  }
  int chunk = min(64, toneSamplesLeft);
  for (int j = 0; j < chunk; j++) {
    float env = 1.0f;
    int rem = toneSamplesLeft - j;
    int att = 80, rel = 160;
    int total = SPK_RATE * 200 / 1000;
    if (j < att)   env = (float)j / att;
    if (rem < rel) env = (float)rem / rel;
    int16_t s = (int16_t)(sinf(tonePhase) * 32767 * toneVol * env);
    toneBuf[j*2] = toneBuf[j*2+1] = s;
    tonePhase += toneInc;
  }
  size_t w;
  i2s_write(SPK_PORT, toneBuf, chunk*4, &w, 0);
  toneSamplesLeft -= chunk;
  if (toneSamplesLeft <= 0) tonePlaying = false;
}

// Sound effects
void sfxShoot()     { queueTone(880,22,0.12f); }
void sfxHit()       { queueTone(220,35,0.3f); }
void sfxExplode()   { queueTone(180,60,0.4f); queueTone(120,80,0.35f); }
void sfxBomb()      { queueTone(440,40,0.4f); queueTone(330,40,0.4f); queueTone(220,80,0.45f); }
void sfxPowerup()   { queueTone(660,50,0.2f); queueTone(880,50,0.2f); queueTone(1320,80,0.2f); }
void sfxShield()    { queueTone(440,30,0.15f); }
void sfxBossDie()   { for(int i=0;i<5;i++) { queueTone(800-i*120, 60, 0.35f); } }
void sfxJingle()    { queueTone(523,80); queueTone(659,80); queueTone(784,80); queueTone(1047,160); }
void sfxGameOver()  { queueTone(392,120); queueTone(330,120); queueTone(262,240); }
void sfxWave()      { queueTone(1047,60,0.2f); queueTone(1319,60,0.2f); queueTone(1568,120,0.2f); }

// ═══════════════════════════════════════════════════════════
//  GAME CONSTANTS
// ═══════════════════════════════════════════════════════════
#define SHIP_X       8
#define SHIP_SPD     2
#define BULLET_SPD   5
#define MAX_BULLETS  8
#define MAX_ENEMIES  10
#define MAX_PARTICLES 24
#define MAX_STARS    25
#define MAX_POWERUPS  4

#define KILLS_PER_WAVE 10

// Enemy types
#define E_DRIFTER  0   // moves straight, slow
#define E_HUNTER   1   // homes on player Y slowly
#define E_KAMIKAZE 2   // dashes at player
#define E_BOSS     3   // large multi-hit

// Power-up types
#define PU_SHIELD    0
#define PU_TRISHOT   1
#define PU_SPEED     2
#define PU_BOMB      3

// ═══════════════════════════════════════════════════════════
//  GAME OBJECTS
// ═══════════════════════════════════════════════════════════
struct Ship {
  float y;
  int   lives;
  int   bombs;
  int   score;
  int   hiScore;
  bool  shielded;
  int   shieldTimer;
  bool  triShot;
  int   triShotTimer;
  bool  speedBoost;
  int   speedTimer;
  int   invincTimer;  // post-death grace
} ship;

struct Bullet {
  float x, y;
  int8_t dy;   // for trishot angles
  bool  active;
} bullets[MAX_BULLETS];

struct Enemy {
  float  x, y;
  int8_t type;
  int8_t hp;
  float  vx, vy;
  bool   active;
  int    animFrame;
} enemies[MAX_ENEMIES];

struct Particle {
  float x, y, vx, vy;
  uint8_t life, maxLife;
  uint8_t shape; // 0=dot, 1=dash
} particles[MAX_PARTICLES];

struct Star {
  float x;
  int8_t y, speed, brightness;
} stars[MAX_STARS];

struct PowerUp {
  float x, y;
  uint8_t type;
  bool active;
  int animTimer;
} powerups[MAX_POWERUPS];

struct Boss {
  float  x, y, vy;
  int    hp, maxHp;
  bool   active;
  int    animFrame;
  int    shootTimer;
  float  bulletY[3];
  float  bulletX[3];
  bool   bulletActive[3];
} boss;

// ═══════════════════════════════════════════════════════════
//  GAME STATE
// ═══════════════════════════════════════════════════════════
enum GameState { GS_BOOT, GS_TITLE, GS_PLAYING, GS_WAVE_CLEAR,
                 GS_BOSS_ENTRY, GS_DEAD, GS_SCORE, GS_GAMEOVER };
GameState gState = GS_BOOT;

unsigned long frameCount   = 0;
unsigned long lastActivity = 0;
int  wave          = 1;
int  killCount     = 0;     // kills this wave
int  enemySpawnTimer = 0;
int  waveAnimTimer = 0;
int  bossEntryTimer = 0;
int  deathAnimTimer = 0;
int  gameOverTimer  = 0;
bool newHiScore = false;

#define IDLE_MS 6000

// ═══════════════════════════════════════════════════════════
//  PIXEL ART SPRITES  (7×7 ship, 5×5 enemies, 13×9 boss)
// ═══════════════════════════════════════════════════════════

// Player ship (7 wide × 7 tall) — facing right
const uint8_t SPR_SHIP[7] = {
  0b0001000,
  0b0011000,
  0b1111110,
  0b1111111,
  0b1111110,
  0b0011000,
  0b0001000
};

// Thruster flame (alternating frames)
const uint8_t SPR_FLAME_A[3] = { 0b110, 0b100, 0b010 };
const uint8_t SPR_FLAME_B[3] = { 0b010, 0b110, 0b100 };

// Drifter enemy (5×5)
const uint8_t SPR_DRIFTER[5] = {
  0b01110,
  0b11111,
  0b11111,
  0b01110,
  0b00100
};

// Hunter enemy (5×5)
const uint8_t SPR_HUNTER[5] = {
  0b10001,
  0b01110,
  0b11111,
  0b01110,
  0b10001
};

// Kamikaze enemy (5×5)
const uint8_t SPR_KAMIKAZE[5] = {
  0b00100,
  0b01110,
  0b11111,
  0b11111,
  0b11111
};

// Boss (13 wide × 9 tall)
const uint16_t SPR_BOSS[9] = {
  0b0000100000100,
  0b0001110001110,
  0b0011111111110,
  0b0111111111111,
  0b1111111111111,
  0b0111111111111,
  0b0011111111110,
  0b0001110001110,
  0b0000100000100
};

void drawSprite5(int x, int y, const uint8_t* spr, bool invert = false) {
  for (int r = 0; r < 5; r++)
    for (int c = 0; c < 5; c++)
      if (spr[r] & (0x10 >> c))
        oled.drawPixel(x+c, y+r, invert ? SSD1306_BLACK : SSD1306_WHITE);
}

void drawShip(int x, int y, bool flash = false) {
  if (flash && (frameCount/3)%2==1) return;
  for (int r = 0; r < 7; r++)
    for (int c = 0; c < 7; c++)
      if (SPR_SHIP[r] & (0x40 >> c))
        oled.drawPixel(x+c, y+r, SSD1306_WHITE);
  // Thruster flame
  const uint8_t* fl = (frameCount/4)%2==0 ? SPR_FLAME_A : SPR_FLAME_B;
  for (int r = 0; r < 3; r++)
    for (int c = 0; c < 3; c++)
      if (fl[r] & (0x4 >> c))
        oled.drawPixel(x - 3 + c, y + 2 + r, SSD1306_WHITE);
  // Shield ring
  if (ship.shielded && (frameCount/2)%2==0)
    oled.drawCircle(x+3, y+3, 6, SSD1306_WHITE);
}

void drawBoss(int x, int y) {
  bool flashB = boss.hp < boss.maxHp/2 && (frameCount/4)%2==0;
  for (int r = 0; r < 9; r++)
    for (int c = 0; c < 13; c++)
      if (SPR_BOSS[r] & (0x1000 >> c))
        oled.drawPixel(x+c, y+r, flashB ? SSD1306_BLACK : SSD1306_WHITE);
  // Boss HP bar above it
  int bw = map(boss.hp, 0, boss.maxHp, 0, 30);
  oled.drawRect(SW-35, 1, 32, 4, SSD1306_WHITE);
  oled.fillRect(SW-35, 1, bw, 4, SSD1306_WHITE);
}

// ═══════════════════════════════════════════════════════════
//  PARTICLES
// ═══════════════════════════════════════════════════════════
void spawnExplosion(float x, float y, int count, bool big=false) {
  for (int i = 0; i < count; i++) {
    for (int p = 0; p < MAX_PARTICLES; p++) {
      if (particles[p].life > 0) continue;
      float ang = random(0, 628) / 100.0f;
      float spd = big ? random(8,22)*0.1f : random(5,15)*0.1f;
      particles[p] = {
        x, y,
        cosf(ang)*spd, sinf(ang)*spd,
        (uint8_t)(big ? random(20,35) : random(12,22)),
        (uint8_t)(big ? 35 : 22),
        (uint8_t)(random(0,2))
      };
      break;
    }
  }
}

void updateParticles() {
  for (int p = 0; p < MAX_PARTICLES; p++) {
    if (particles[p].life == 0) continue;
    particles[p].x  += particles[p].vx;
    particles[p].y  += particles[p].vy;
    particles[p].vy += 0.05f;
    particles[p].life--;
    int px=(int)particles[p].x, py=(int)particles[p].y;
    if (px<0||px>=SW||py<0||py>=SH) { particles[p].life=0; continue; }
    // Fade: draw only in first half of life
    if (particles[p].life > particles[p].maxLife/3) {
      oled.drawPixel(px, py, SSD1306_WHITE);
      if (particles[p].shape==1 && px+1<SW)
        oled.drawPixel(px+1, py, SSD1306_WHITE);
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  STARS  (3-layer parallax)
// ═══════════════════════════════════════════════════════════
void initStars() {
  for (int i = 0; i < MAX_STARS; i++)
    stars[i] = { (float)random(0,SW), (int8_t)random(0,SH),
                 (int8_t)random(1,4), (int8_t)random(0,3) };
}

void updateStars(float extraSpd = 0) {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x -= stars[i].speed + extraSpd;
    if (stars[i].x < 0) { stars[i].x = SW; stars[i].y = random(0,SH); }
    // Brightness levels: 1=always, 2=half, 3=quarter
    bool show = (stars[i].brightness == 0) ? true
              : (stars[i].brightness == 1) ? ((frameCount/3)%2==0)
              :                              ((frameCount/6)%2==0);
    if (show) oled.drawPixel((int)stars[i].x, stars[i].y, SSD1306_WHITE);
  }
}

// ═══════════════════════════════════════════════════════════
//  POWER-UPS
// ═══════════════════════════════════════════════════════════
void trySpawnPowerup(float x, float y) {
  if (random(0, 5) != 0) return; // 20% chance
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) {
      powerups[i] = { x, y, (uint8_t)random(0,4), true, 0 };
      return;
    }
  }
}

void drawPowerupIcon(int x, int y, uint8_t type, bool blink) {
  if (blink && (frameCount/6)%2==0) return;
  oled.drawRect(x, y, 7, 7, SSD1306_WHITE);
  const char* lbl[] = {"S","T","V","B"};
  oled.setTextSize(1);
  oled.setCursor(x+1, y+1);
  oled.print(lbl[type]);
}

void updatePowerups() {
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) continue;
    powerups[i].x -= 1.0f;
    powerups[i].animTimer++;
    if (powerups[i].x < -8) { powerups[i].active = false; continue; }
    drawPowerupIcon((int)powerups[i].x, (int)powerups[i].y, powerups[i].type,
                    powerups[i].animTimer > 120);

    // Collect
    float sx = SHIP_X, sy = ship.y;
    float dx = powerups[i].x - sx, dy = powerups[i].y - sy;
    if (dx*dx + dy*dy < 64) {
      powerups[i].active = false;
      sfxPowerup();
      switch (powerups[i].type) {
        case PU_SHIELD:  ship.shielded=true; ship.shieldTimer=300; break;
        case PU_TRISHOT: ship.triShot=true;  ship.triShotTimer=400; break;
        case PU_SPEED:   ship.speedBoost=true; ship.speedTimer=350; break;
        case PU_BOMB:    if(ship.bombs<5) ship.bombs++; break;
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  BULLETS
// ═══════════════════════════════════════════════════════════
void fireBullet() {
  // Find free slots
  auto shoot = [](float x, float y, int8_t dy) {
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (!bullets[i].active) {
        bullets[i] = { x, y, dy, true };
        return;
      }
    }
  };
  shoot(SHIP_X+7, ship.y+3, 0);
  if (ship.triShot) {
    shoot(SHIP_X+5, ship.y+1, -1);
    shoot(SHIP_X+5, ship.y+5, 1);
  }
  sfxShoot();
}

void updateBullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;
    bullets[i].x += BULLET_SPD;
    bullets[i].y += bullets[i].dy * 0.7f;
    if (bullets[i].x > SW) { bullets[i].active = false; continue; }
    // Draw: dashes look slick
    oled.drawFastHLine((int)bullets[i].x, (int)bullets[i].y, 4, SSD1306_WHITE);
  }
}

// ═══════════════════════════════════════════════════════════
//  ENEMIES
// ═══════════════════════════════════════════════════════════
float waveSpeed() { return 1.0f + wave * 0.25f; }

void spawnEnemy() {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) continue;
    uint8_t type = (wave < 2) ? E_DRIFTER :
                   (wave < 4) ? (uint8_t)random(0,2) :
                                (uint8_t)random(0,3);
    int hp = (type==E_HUNTER)?2 : (type==E_KAMIKAZE)?1 : 1;
    float vy = (type==E_DRIFTER) ? (random(0,3)-1)*0.3f : 0;
    enemies[i] = {
      (float)SW+2, (float)random(4,SH-10),
      (int8_t)type, (int8_t)hp,
      -waveSpeed(), vy,
      true, 0
    };
    return;
  }
}

void updateEnemies() {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;
    Enemy& e = enemies[i];
    e.animFrame++;

    float spd = waveSpeed();
    if (e.type == E_HUNTER) {
      // Slowly home on player Y
      float dy = (ship.y+3) - (e.y+2);
      e.vy = constrain(e.vy + dy*0.02f, -2.0f, 2.0f);
      e.vx = -spd;
    } else if (e.type == E_KAMIKAZE) {
      // After 40 frames: dash
      if (e.animFrame > 40) {
        float dx = (SHIP_X+3) - e.x;
        float dy = (ship.y+3) - e.y;
        float len = sqrtf(dx*dx+dy*dy);
        if (len > 0) { e.vx = dx/len * spd*2.2f; e.vy = dy/len * spd*2.2f; }
      } else {
        e.vx = -0.3f; // hover before dash
        e.vy = sinf(e.animFrame * 0.18f) * 0.8f;
      }
    }

    e.x += e.vx; e.y += e.vy;
    e.y = constrain(e.y, 0, SH-8);

    if (e.x < -10) { e.active = false; continue; }

    // Draw enemy
    const uint8_t* spr = (e.type==E_DRIFTER) ? SPR_DRIFTER :
                         (e.type==E_HUNTER)   ? SPR_HUNTER  : SPR_KAMIKAZE;
    bool inv = (e.type==E_KAMIKAZE && e.animFrame>35 && (e.animFrame/4)%2==0);
    drawSprite5((int)e.x, (int)e.y, spr, inv);

    // Bullet collision
    for (int b = 0; b < MAX_BULLETS; b++) {
      if (!bullets[b].active) continue;
      float bx=bullets[b].x, by=bullets[b].y;
      if (bx>=e.x && bx<=e.x+5 && by>=e.y && by<=e.y+5) {
        bullets[b].active = false;
        e.hp--;
        sfxHit();
        if (e.hp <= 0) {
          spawnExplosion(e.x+2, e.y+2, 8);
          sfxExplode();
          ship.score += (e.type==E_KAMIKAZE)?30 : (e.type==E_HUNTER)?20 : 10;
          trySpawnPowerup(e.x, e.y);
          e.active = false;
          killCount++;
        }
        break;
      }
    }

    // Ship collision
    if (ship.invincTimer == 0) {
      float sx=SHIP_X, sy=ship.y;
      if (e.x<sx+7 && e.x+5>sx && e.y<sy+7 && e.y+5>sy) {
        if (ship.shielded) {
          ship.shielded=false; sfxShield();
          spawnExplosion(e.x, e.y, 6);
          e.active=false;
        } else {
          hitPlayer();
          e.active=false;
        }
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  BOSS
// ═══════════════════════════════════════════════════════════
void spawnBoss() {
  int bossHp = 8 + wave * 4;
  boss = { (float)SW+5, (float)(SH/2)-4, 1.0f,
           bossHp, bossHp, true, 0, 60,
           {0,0,0}, {0,0,0}, {false,false,false} };
}

void updateBoss() {
  if (!boss.active) return;
  boss.animFrame++;

  // Entry slide
  if (boss.x > SW - 18) { boss.x -= 1.5f; return; }

  // Bounce up/down
  boss.y += boss.vy;
  if (boss.y < 2 || boss.y > SH-12) boss.vy = -boss.vy;

  // Boss shoots 3 bullets periodically
  boss.shootTimer--;
  if (boss.shootTimer <= 0) {
    boss.shootTimer = max(25, 60 - wave*5);
    for (int i = 0; i < 3; i++) {
      boss.bulletActive[i] = true;
      boss.bulletX[i] = boss.x;
      boss.bulletY[i] = boss.y + 1 + i*3;
    }
  }

  // Move boss bullets
  for (int i = 0; i < 3; i++) {
    if (!boss.bulletActive[i]) continue;
    boss.bulletX[i] -= 3.0f;
    if (boss.bulletX[i] < 0) { boss.bulletActive[i]=false; continue; }
    oled.drawPixel((int)boss.bulletX[i], (int)boss.bulletY[i], SSD1306_WHITE);

    // Hit ship
    if (ship.invincTimer==0) {
      float sx=SHIP_X, sy=ship.y;
      if (boss.bulletX[i]<sx+7 && boss.bulletX[i]>sx-1 &&
          boss.bulletY[i]<sy+7 && boss.bulletY[i]>sy-1) {
        boss.bulletActive[i] = false;
        if (ship.shielded) { ship.shielded=false; sfxShield(); }
        else hitPlayer();
      }
    }
  }

  drawBoss((int)boss.x, (int)boss.y);

  // Bullet hits boss
  for (int b = 0; b < MAX_BULLETS; b++) {
    if (!bullets[b].active) continue;
    if (bullets[b].x>=boss.x && bullets[b].x<=boss.x+13 &&
        bullets[b].y>=boss.y && bullets[b].y<=boss.y+9) {
      bullets[b].active=false;
      boss.hp--;
      sfxHit();
      spawnExplosion(bullets[b].x, bullets[b].y, 4);
      if (boss.hp <= 0) {
        spawnExplosion(boss.x+6, boss.y+4, 20, true);
        sfxBossDie();
        ship.score += 500 + wave*100;
        boss.active = false;
        wave++;
        killCount = 0;
        waveAnimTimer = 90;
        gState = GS_WAVE_CLEAR;
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  SMART BOMB
// ═══════════════════════════════════════════════════════════
void triggerBomb() {
  if (ship.bombs <= 0) return;
  ship.bombs--;
  sfxBomb();
  // Expanding ring animation then kill all
  for (int r = 0; r < 40; r+=4) {
    oled.clearDisplay();
    updateStars(2);
    drawShip(SHIP_X, (int)ship.y);
    oled.drawCircle(SHIP_X+3, (int)ship.y+3, r, SSD1306_WHITE);
    if (r > 10) oled.drawCircle(SHIP_X+3, (int)ship.y+3, r-8, SSD1306_WHITE);
    oled.display();
    delay(12);
  }
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;
    spawnExplosion(enemies[i].x+2, enemies[i].y+2, 6);
    ship.score += 5;
    enemies[i].active = false;
    killCount++;
  }
  if (boss.active) {
    boss.hp -= 3;
    spawnExplosion(boss.x+6, boss.y+4, 10);
    if (boss.hp <= 0) {
      spawnExplosion(boss.x+6, boss.y+4, 20, true);
      sfxBossDie();
      ship.score += 500 + wave*100;
      boss.active = false;
      wave++;
      killCount = 0;
      waveAnimTimer = 90;
      gState = GS_WAVE_CLEAR;
    }
  }
}

// ═══════════════════════════════════════════════════════════
//  HIT PLAYER
// ═══════════════════════════════════════════════════════════
void hitPlayer() {
  ship.lives--;
  sfxExplode();
  spawnExplosion(SHIP_X+3, ship.y+3, 12, true);
  ship.invincTimer = 120;
  if (ship.lives <= 0) {
    if (ship.score > ship.hiScore) { ship.hiScore=ship.score; newHiScore=true; }
    gState = GS_DEAD;
    deathAnimTimer = 0;
    sfxGameOver();
  }
}

// ═══════════════════════════════════════════════════════════
//  HUD
// ═══════════════════════════════════════════════════════════
void drawHUD() {
  // Lives (little ship icons)
  for (int i = 0; i < ship.lives; i++) {
    oled.fillTriangle(2+i*8, SH-1, 4+i*8, SH-5, 6+i*8, SH-1, SSD1306_WHITE);
  }
  // Bombs (dots)
  for (int i = 0; i < ship.bombs; i++) {
    oled.fillCircle(SW-4 - i*6, SH-3, 2, SSD1306_WHITE);
  }
  // Score
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(42, SH-8);
  oled.print(ship.score);
  // Wave
  oled.setCursor(SW/2-8, 1);
  oled.print("W"); oled.print(wave);
  // Power-up timers (tiny icons top-left)
  int px = 1;
  if (ship.shielded)   { oled.setCursor(px,1); oled.print("S"); px+=8; }
  if (ship.triShot)    { oled.setCursor(px,1); oled.print("T"); px+=8; }
  if (ship.speedBoost) { oled.setCursor(px,1); oled.print("V"); }
}

// ═══════════════════════════════════════════════════════════
//  INIT GAME
// ═══════════════════════════════════════════════════════════
void initGame() {
  ship = { (float)(SH/2-3), 3, 3, 0, ship.hiScore,
           false,0, false,0, false,0, 0 };
  wave=1; killCount=0;
  newHiScore=false;
  memset(bullets,   0, sizeof(bullets));
  memset(enemies,   0, sizeof(enemies));
  memset(particles, 0, sizeof(particles));
  memset(powerups,  0, sizeof(powerups));
  boss.active = false;
  enemySpawnTimer = 0;
  lastActivity = millis();
  gState = GS_PLAYING;
}

// ═══════════════════════════════════════════════════════════
//  SCREENS
// ═══════════════════════════════════════════════════════════

// ── TITLE ──────────────────────────────────────────────────
void drawTitle() {
  oled.clearDisplay();
  updateStars(0.3f);

  // Animated ship flying across
  int sx = (frameCount*2) % (SW+20) - 10;
  drawShip(sx, SH/2-3);

  // Title text with pixel shadow
  oled.setTextSize(2); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(14, 6);
  oled.print("NOVA");
  oled.setCursor(8, 22);
  oled.print("STRIKE");

  // Animated underline
  for (int x=8; x<120; x+=3) {
    int yy = 38 + (int)(2*sinf(x*0.08f + frameCount*0.12f));
    oled.drawPixel(x, yy, SSD1306_WHITE);
  }

  oled.setTextSize(1);
  if ((frameCount/20)%2==0) {
    oled.setCursor(14, 44);
    oled.print("[ TOUCH FIRE TO START ]");
  }
  if (ship.hiScore > 0) {
    oled.setCursor(28, 56);
    oled.print("BEST: "); oled.print(ship.hiScore);
  }

  // Idle countdown
  unsigned long idle = millis() - lastActivity;
  if (idle > 2500) {
    int bw = map(constrain(idle,2500,(unsigned long)IDLE_MS), 2500, IDLE_MS, 80, 0);
    oled.drawRect(24,55,80,4,SSD1306_WHITE);
    oled.fillRect(24,55,bw,4,SSD1306_WHITE);
  }

  oled.display();
  if (btns[2].edge) { lastActivity=millis(); sfxJingle(); initGame(); }
  if (millis()-lastActivity > (unsigned long)IDLE_MS) { lastActivity=millis(); } // just reset
}

// ── PLAYING ────────────────────────────────────────────────
void updatePlaying() {
  // Power-up timers
  if (ship.shieldTimer>0   && --ship.shieldTimer==0) ship.shielded=false;
  if (ship.triShotTimer>0  && --ship.triShotTimer==0) ship.triShot=false;
  if (ship.speedTimer>0    && --ship.speedTimer==0)   ship.speedBoost=false;
  if (ship.invincTimer>0)  ship.invincTimer--;

  // Movement
  float spd = ship.speedBoost ? SHIP_SPD*1.7f : SHIP_SPD;
  if (UP_PRESSED)   ship.y = max(0.0f, ship.y - spd);
  if (DOWN_PRESSED) ship.y = min((float)(SH-8), ship.y + spd);

  if (FIRE_EDGE) fireBullet();
  if (BOMB_EDGE) triggerBomb();

  // Enemy spawning (more frequent each wave)
  enemySpawnTimer--;
  int spawnRate = max(15, 60 - wave*8);
  if (enemySpawnTimer <= 0 && !boss.active) {
    spawnEnemy();
    enemySpawnTimer = spawnRate;
  }

  // Wave clear → boss
  if (killCount >= KILLS_PER_WAVE && !boss.active && gState==GS_PLAYING) {
    if (wave % 3 == 0) {
      // Boss wave
      gState = GS_BOSS_ENTRY;
      bossEntryTimer = 80;
      spawnBoss();
    } else {
      wave++;
      killCount = 0;
      waveAnimTimer = 70;
      gState = GS_WAVE_CLEAR;
      sfxWave();
    }
  }

  // Draw
  oled.clearDisplay();
  updateStars(ship.speedBoost ? 1.5f : 0);
  updateBullets();
  updateEnemies();
  updateBoss();
  updatePowerups();
  updateParticles();
  drawShip(SHIP_X, (int)ship.y, ship.invincTimer>0);
  drawHUD();
  oled.display();
  tickAudio();
}

// ── WAVE CLEAR ─────────────────────────────────────────────
void updateWaveClear() {
  oled.clearDisplay();
  updateStars(1.0f);
  updateParticles();

  // Radiate lines from center
  for (int i = 0; i < 8; i++) {
    float ang = i * M_PI / 4.0f + frameCount * 0.05f;
    int r = (90 - waveAnimTimer) * 0.5f;
    oled.drawLine(SW/2, SH/2,
                  SW/2 + (int)(cosf(ang)*r),
                  SH/2 + (int)(sinf(ang)*r), SSD1306_WHITE);
  }

  oled.setTextSize(2); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(10, 18);
  oled.print("WAVE ");
  oled.print(wave);
  oled.setTextSize(1);
  oled.setCursor(32, 38);
  oled.print("CLEARED!");

  oled.display();
  tickAudio();
  waveAnimTimer--;
  if (waveAnimTimer <= 0) gState = GS_PLAYING;
}

// ── BOSS ENTRY ─────────────────────────────────────────────
void updateBossEntry() {
  oled.clearDisplay();
  updateStars(0.5f);

  // Flashing warning
  if ((bossEntryTimer/8)%2==0) {
    oled.setTextSize(2); oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(4, 18);
    oled.print("! BOSS !");
  }
  // Boss slides in
  drawBoss((int)boss.x, (int)boss.y);
  if (boss.x > SW-18) boss.x -= 1.5f;

  oled.display();
  tickAudio();
  bossEntryTimer--;
  if (bossEntryTimer <= 0) { gState=GS_PLAYING; sfxWave(); }
}

// ── DEATH ANIM ─────────────────────────────────────────────
void updateDead() {
  oled.clearDisplay();
  updateStars(0);
  updateParticles();
  deathAnimTimer++;

  // Shockwave ring
  int ring = min(deathAnimTimer*2, 64);
  oled.drawCircle(SHIP_X+3, (int)ship.y+3, ring, SSD1306_WHITE);
  if (ring > 15) oled.drawCircle(SHIP_X+3, (int)ship.y+3, ring-12, SSD1306_WHITE);

  if (deathAnimTimer > 40) {
    oled.setTextSize(2); oled.setTextColor(SSD1306_WHITE);
    if ((deathAnimTimer/6)%2==0) {
      oled.setCursor(16, 14);
      oled.print("SHIP LOST");
    }
    oled.setTextSize(1);
    oled.setCursor(20, 36);
    oled.print("Score: "); oled.print(ship.score);
    oled.setCursor(26, 48);
    oled.print("TOUCH to continue");
  }

  oled.display();
  tickAudio();

  if (deathAnimTimer > 50 && ANY_EDGE) {
    lastActivity = millis();
    gState = GS_SCORE;
  }
  if (millis()-lastActivity > (unsigned long)IDLE_MS)
    { lastActivity=millis(); gState=GS_SCORE; }
}

// ── SCORE SCREEN ───────────────────────────────────────────
void updateScore() {
  oled.clearDisplay();
  updateStars(0.2f);

  static int scoreTick = 0;
  scoreTick++;
  if (gState != GS_SCORE) { scoreTick=0; return; }

  // Animated score count-up
  int shown = min(ship.score, scoreTick * 8);

  // Medal icon (simple star)
  int cx=64, cy=14;
  for (int i=0;i<5;i++) {
    float a=i*M_PI*2/5 - M_PI/2;
    float a2=a+M_PI/5;
    oled.drawLine(cx+(int)(cosf(a)*10), cy+(int)(sinf(a)*10),
                  cx+(int)(cosf(a2)*5), cy+(int)(sinf(a2)*5), SSD1306_WHITE);
    float a3=a2+M_PI/5;
    oled.drawLine(cx+(int)(cosf(a2)*5), cy+(int)(sinf(a2)*5),
                  cx+(int)(cosf(a3)*10), cy+(int)(sinf(a3)*10), SSD1306_WHITE);
  }

  oled.setTextSize(2); oled.setTextColor(SSD1306_WHITE);
  char buf[12]; sprintf(buf,"%06d",shown);
  oled.setCursor(16, 26);
  oled.print(buf);

  oled.setTextSize(1);
  oled.setCursor(14, 44);
  oled.print("BEST: "); oled.print(ship.hiScore);

  if (newHiScore && (scoreTick/10)%2==0) {
    oled.setCursor(28, 54);
    oled.print("* NEW RECORD *");
  } else if (!newHiScore) {
    if ((scoreTick/14)%2==0) {
      oled.setCursor(16, 54);
      oled.print("FIRE = RETRY");
    }
  }

  // Idle bar
  unsigned long idle = millis() - lastActivity;
  if (idle > 2000) {
    int bw = map(constrain(idle,2000,(unsigned long)IDLE_MS), 2000, IDLE_MS, 96, 0);
    oled.drawRect(16, 8, 96, 3, SSD1306_WHITE);
    oled.fillRect(16, 8, bw, 3, SSD1306_WHITE);
  }

  oled.display();
  tickAudio();

  if (btns[2].edge) { lastActivity=millis(); initGame(); }
  if (millis()-lastActivity > (unsigned long)IDLE_MS) {
    returnToTitle();
  }
}

// ── RETURN TO TITLE ANIMATION ──────────────────────────────
void returnToTitle() {
  // Hyperspace effect: streaking stars
  for (int t = 0; t < 40; t++) {
    oled.clearDisplay();
    for (int i = 0; i < MAX_STARS; i++) {
      int len = stars[i].speed * (t/3 + 1);
      oled.drawFastHLine(stars[i].x, stars[i].y, min(len,20), SSD1306_WHITE);
    }
    oled.display();
    delay(15);
    for (int i = 0; i < MAX_STARS; i++) {
      stars[i].x -= stars[i].speed * 3;
      if (stars[i].x < 0) stars[i].x = SW;
    }
  }

  // "NOVA STRIKE" slides in letter by letter
  oled.clearDisplay(); oled.display();
  const char* title = "NOVA STRIKE";
  for (int ch = 0; ch < 11; ch++) {
    oled.clearDisplay();
    updateStars(0.5f);
    oled.setTextSize(2); oled.setTextColor(SSD1306_WHITE);
    char partial[12] = {0};
    strncpy(partial, title, ch+1);
    int tw = (ch+1) * 12;
    oled.setCursor((SW-tw)/2, 22);
    oled.print(partial);
    oled.display();
    delay(60);
  }
  delay(200);

  initStars();
  lastActivity = millis();
  gState = GS_TITLE;
}

// ── BOOT ───────────────────────────────────────────────────
void bootAnimation() {
  // Scanline fill
  for (int y = 0; y < SH; y++) {
    oled.drawFastHLine(0, y, SW, SSD1306_WHITE);
    if (y%4==0) oled.display();
    delay(8);
  }
  delay(100);

  // Wipe to "NOVA"
  oled.clearDisplay();
  oled.setTextSize(3); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(10, 16); oled.print("NOVA");
  oled.display(); delay(300);

  // Flash + "STRIKE"
  for (int i=0;i<3;i++) {
    oled.invertDisplay(true);  delay(80);
    oled.invertDisplay(false); delay(80);
  }
  oled.setTextSize(2); oled.setCursor(8, 40);
  oled.print("STRIKE!"); oled.display();
  sfxJingle();
  delay(800);
  oled.clearDisplay(); oled.display();
}

// ═══════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  Wire.begin(5, 6);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed"); for(;;);
  }
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  initButtons();
  setupSpeaker();
  initStars();

  ship.hiScore = 0;
  ship.lives   = 3;
  ship.bombs   = 3;

  bootAnimation();

  lastActivity = millis();
  gState = GS_TITLE;
}

// ═══════════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════════
void loop() {
  frameCount++;
  readButtons();

  if (ANY_EDGE) lastActivity = millis();

  switch (gState) {
    case GS_TITLE:      drawTitle();       delay(28); break;
    case GS_PLAYING:    updatePlaying();   delay(18); break;
    case GS_WAVE_CLEAR: updateWaveClear(); delay(20); break;
    case GS_BOSS_ENTRY: updateBossEntry(); delay(20); break;
    case GS_DEAD:       updateDead();      delay(20); break;
    case GS_SCORE:      updateScore();     delay(22); break;
    default: break;
  }
}
