// ============================================================================
//  ТЕНИ РАССВЕТА (Shadows of Dawn) - 2D-файтинг в духе Shadow Fight 2
//  Свой сюжет, свои персонажи. C++17 + SDL2 (нужна версия >= 2.0.18).
//  Внешних файлов нет: графика рисуется кодом, шрифт (с кириллицей) встроен.
//
//  Сборка (Linux/macOS):
//    g++ -std=c++17 -O2 main.cpp -o shadows $(sdl2-config --cflags --libs)
//  Сборка (Windows, MSYS2/MinGW):
//    g++ -std=c++17 -O2 main.cpp -o shadows.exe -lmingw32 -lSDL2main -lSDL2
//
//  Управление:
//    A/D или стрелки - ходьба     W/стрелка вверх - прыжок    S/стрелка вниз - блок
//    J или Z - удар рукой         K или X - удар ногой
//    L или C - удар оружием       U или V - магия (нужно 40 энергии)
//    Enter/Пробел - выбор, диалоги    Esc - выход
// ============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <random>
#include <string>
#include <vector>
#ifndef HEADLESS
#include <SDL.h>
#endif
using namespace std;

constexpr int W = 960, H = 540;
constexpr float GROUND = 450.f;

struct Col {
  uint8_t r, g, b, a;
  Col(uint8_t R = 0, uint8_t G = 0, uint8_t B = 0, uint8_t A = 255) : r(R), g(G), b(B), a(A) {}
};
struct Pt { float x, y; };

// ---- графические примитивы (реализация внизу, в платформенной части) -------
static float gOX = 0, gOY = 0;  // смещение камеры (тряска экрана)
void gRect(float x, float y, float w, float h, Col c);
void gTri(float x1, float y1, float x2, float y2, float x3, float y3, Col c);
void gDisc(float x, float y, float r, Col c);
void gLine(float x1, float y1, float x2, float y2, float w, Col c);

static mt19937 rng(7);
static float rnd() { return uniform_real_distribution<float>(0.f, 1.f)(rng); }
static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static Col alpha(Col c, int a) { c.a = (uint8_t)clampf((float)a, 0, 255); return c; }
static Col lerpC(Col a, Col b, float t) {
  return Col((uint8_t)(a.r + (b.r - a.r) * t), (uint8_t)(a.g + (b.g - a.g) * t),
             (uint8_t)(a.b + (b.b - a.b) * t), (uint8_t)(a.a + (b.a - a.a) * t));
}

// ============================================================================
//  ШРИФТ 5x7 (латиница, цифры, кириллица; строчные рисуются заглавными)
// ============================================================================
struct Glyph { char32_t c; uint8_t r[7]; };
static const Glyph FONT_DATA[] = {
  {'A',{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}}, {'B',{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
  {'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}}, {'D',{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
  {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}}, {'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
  {'G',{0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}}, {'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
  {'I',{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}}, {'J',{0x07,0x02,0x02,0x02,0x02,0x12,0x0C}},
  {'K',{0x11,0x12,0x14,0x18,0x14,0x12,0x11}}, {'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
  {'M',{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}}, {'N',{0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
  {'O',{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}}, {'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
  {'Q',{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}}, {'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
  {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}}, {'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
  {'U',{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}}, {'V',{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
  {'W',{0x11,0x11,0x11,0x15,0x15,0x1B,0x11}}, {'X',{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
  {'Y',{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}}, {'Z',{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
  {'0',{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}}, {'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
  {'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}}, {'3',{0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}},
  {'4',{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}}, {'5',{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
  {'6',{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}}, {'7',{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
  {'8',{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}}, {'9',{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
  {' ',{0,0,0,0,0,0,0}}, {'.',{0,0,0,0,0,0x0C,0x0C}}, {',',{0,0,0,0,0x0C,0x04,0x08}},
  {'!',{0x04,0x04,0x04,0x04,0x04,0,0x04}}, {'?',{0x0E,0x11,0x01,0x02,0x04,0,0x04}},
  {':',{0,0x0C,0x0C,0,0x0C,0x0C,0}}, {'-',{0,0,0,0x1F,0,0,0}}, {'+',{0,0x04,0x04,0x1F,0x04,0x04,0}},
  {'/',{0x01,0x01,0x02,0x04,0x08,0x10,0x10}}, {'%',{0x18,0x19,0x02,0x04,0x08,0x13,0x03}},
  {'(',{0x02,0x04,0x08,0x08,0x08,0x04,0x02}}, {')',{0x08,0x04,0x02,0x02,0x02,0x04,0x08}},
  {'>',{0x08,0x04,0x02,0x01,0x02,0x04,0x08}}, {'<',{0x02,0x04,0x08,0x10,0x08,0x04,0x02}},
  {'\'',{0x04,0x04,0x08,0,0,0,0}}, {'"',{0x0A,0x0A,0,0,0,0,0}}, {'*',{0,0x15,0x0E,0x1F,0x0E,0x15,0}},
  // кириллица, не совпадающая с латиницей
  {0x401,{0x0A,0x1F,0x10,0x1E,0x10,0x10,0x1F}}, {0x411,{0x1F,0x10,0x10,0x1E,0x11,0x11,0x1E}},
  {0x413,{0x1F,0x10,0x10,0x10,0x10,0x10,0x10}}, {0x414,{0x0E,0x0A,0x0A,0x0A,0x0A,0x1F,0x11}},
  {0x416,{0x15,0x15,0x15,0x0E,0x15,0x15,0x15}}, {0x417,{0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}},
  {0x418,{0x11,0x11,0x13,0x15,0x19,0x11,0x11}}, {0x419,{0x0A,0x11,0x13,0x15,0x19,0x11,0x11}},
  {0x41B,{0x07,0x09,0x09,0x09,0x09,0x09,0x11}}, {0x41F,{0x1F,0x11,0x11,0x11,0x11,0x11,0x11}},
  {0x423,{0x11,0x11,0x0A,0x04,0x04,0x08,0x10}}, {0x424,{0x04,0x0E,0x15,0x15,0x15,0x0E,0x04}},
  {0x426,{0x12,0x12,0x12,0x12,0x12,0x1F,0x01}}, {0x427,{0x11,0x11,0x11,0x0F,0x01,0x01,0x01}},
  {0x428,{0x15,0x15,0x15,0x15,0x15,0x15,0x1F}}, {0x429,{0x15,0x15,0x15,0x15,0x15,0x1F,0x01}},
  {0x42A,{0x18,0x08,0x08,0x0E,0x09,0x09,0x0E}}, {0x42B,{0x11,0x11,0x11,0x19,0x15,0x15,0x19}},
  {0x42C,{0x10,0x10,0x10,0x1E,0x11,0x11,0x1E}}, {0x42D,{0x0E,0x11,0x01,0x0F,0x01,0x11,0x0E}},
  {0x42E,{0x12,0x15,0x15,0x1D,0x15,0x15,0x12}}, {0x42F,{0x0F,0x11,0x11,0x0F,0x05,0x09,0x11}},
};
static const array<uint8_t, 7>* glyph(char32_t c) {
  static map<char32_t, array<uint8_t, 7>> M;
  if (M.empty()) {
    for (auto& g : FONT_DATA) { array<uint8_t, 7> a; copy(begin(g.r), end(g.r), a.begin()); M[g.c] = a; }
    // кириллические буквы, неотличимые от латинских: А В Е К М Н О Р С Т Х
    const char32_t al[][2] = {{0x410,'A'},{0x412,'B'},{0x415,'E'},{0x41A,'K'},{0x41C,'M'},{0x41D,'H'},
                              {0x41E,'O'},{0x420,'P'},{0x421,'C'},{0x422,'T'},{0x425,'X'}};
    for (auto& p : al) M[p[0]] = M[p[1]];
  }
  if (c >= 'a' && c <= 'z') c -= 32;
  else if (c >= 0x430 && c <= 0x44F) c -= 0x20;
  else if (c == 0x451) c = 0x401;
  else if (c == 0x2014) c = '-';
  else if (c == 0xAB || c == 0xBB) c = '"';
  auto it = M.find(c);
  return it == M.end() ? nullptr : &it->second;
}
static u32string u32(const string& s) {
  u32string r;
  for (size_t i = 0; i < s.size();) {
    unsigned char c = (unsigned char)s[i]; char32_t cp; int n;
    if (c < 0x80) { cp = c; n = 1; } else if ((c >> 5) == 6) { cp = c & 31; n = 2; }
    else if ((c >> 4) == 14) { cp = c & 15; n = 3; } else { cp = c & 7; n = 4; }
    for (int k = 1; k < n && i + k < s.size(); k++) cp = (cp << 6) | ((unsigned char)s[i + k] & 63);
    i += n; r.push_back(cp);
  }
  return r;
}
// align: 0 - слева, 1 - по центру, 2 - справа
static void text(const u32string& s, float x, float y, int sc, Col c, int align = 0) {
  float w = (float)s.size() * 6 * sc - sc;
  if (align == 1) x -= w / 2; else if (align == 2) x -= w;
  for (char32_t ch : s) {
    if (auto g = glyph(ch))
      for (int r = 0; r < 7; r++)
        for (int k = 0; k < 5;) {
          if (!(((*g)[r] >> (4 - k)) & 1)) { k++; continue; }
          int run = 0; while (k + run < 5 && (((*g)[r] >> (4 - k - run)) & 1)) run++;
          gRect(x + k * sc, y + r * sc, (float)run * sc, (float)sc, c); k += run;
        }
    x += 6 * sc;
  }
}
static void text(const string& s, float x, float y, int sc, Col c, int align = 0) { text(u32(s), x, y, sc, c, align); }
static vector<u32string> wrap(const u32string& s, size_t maxc) {
  vector<u32string> out; u32string line, word;
  auto flush = [&]() {
    if (!line.empty() && line.size() + 1 + word.size() > maxc) { out.push_back(line); line = word; }
    else { if (!line.empty()) line += U' '; line += word; }
    word.clear();
  };
  for (char32_t c : s) { if (c == U' ') flush(); else word += c; }
  flush(); if (!line.empty()) out.push_back(line);
  return out;
}

// ============================================================================
//  ДАННЫЕ: персонажи, этапы, сюжет
// ============================================================================
struct Stats {
  const char* name; float hp, speed, dmg, reach, scale; Col glow;
  int wt;  // тип оружия: 0 меч, 1 кинжал, 2 молот, 3 коса, 4 посох
  float aggr, blk, think; bool magic, boss;
};
struct Line { const char* who; const char* txt; };
struct Stage { const char* place; Stats foe; Col sky1, sky2, sun; int deco; vector<Line> pre, post; };

static const Stats HERO = {"АШЕН", 100, 235, 1.f, 95, 1.f, Col(255, 205, 90), 0, 0, 0, 0, true, false};
static const Stats LAM  = {"ЛЭМ", 1, 0, 0, 70, .92f, Col(255, 225, 160), 4, 0, 0, 0, false, false};

static const vector<Line> INTRO = {
  {"РАССКАЗЧИК", "Триста лет назад Владыка Ночи Морвейн разбил Печать Рассвета на пять осколков. С тех пор солнце не всходило."},
  {"РАССКАЗЧИК", "Мир замёрз в вечных сумерках. Люди забыли, как выглядит свет."},
  {"ЛЭМ", "Ашен, твой Фонарь ещё горит. Значит, Хранители не исчезли."},
  {"АШЕН", "Я соберу осколки, Лэм. Все пять. Даже если придётся пройти через каждого из его стражей."},
  {"ЛЭМ", "Тогда иди. Первый осколок у Лиса по прозвищу Тихий Шаг. Ищи его у Пепельных Врат."},
};
static const vector<Line> ENDING = {
  {"РАССКАЗЧИК", "Пять осколков слились воедино. Печать вспыхнула, и впервые за триста лет над миром поднялось солнце."},
  {"ЛЭМ", "Ты сделал это, Ашен. Теперь Фонарь можно погасить."},
  {"АШЕН", "Нет. Пусть горит. Ночь ещё вернётся, и кто-то должен её встретить."},
  {"РАССКАЗЧИК", "КОНЕЦ. Спасибо за игру!"},
};
static const vector<Stage> STAGES = {
  {"ПЕПЕЛЬНЫЕ ВРАТА",
   {"ЛИС ТИХИЙ ШАГ", 75, 270, .8f, 68, .95f, Col(255, 150, 50), 1, .60f, .12f, .30f, false, false},
   Col(40, 18, 30), Col(210, 110, 60), Col(255, 170, 80), 1,
   {{"ЛИС", "Огонёк пришёл за осколком? Попробуй поймать то, чего не видишь."}},
   {{"ЛИС", "Ха... Ты быстрее, чем я думал. Забирай свой осколок."},
    {"АШЕН", "Один из пяти. Фонарь стал горячее."}}},
  {"МОСТ ЭХА",
   {"БРАН КАМЕННЫЙ", 130, 150, 1.25f, 92, 1.25f, Col(120, 220, 210), 2, .50f, .30f, .38f, false, false},
   Col(15, 25, 45), Col(90, 130, 160), Col(200, 230, 255), 0,
   {{"БРАН", "Я стою на этом мосту сто лет. Никто не прошёл мимо моего молота."},
    {"АШЕН", "Тогда я буду первым."}},
   {{"БРАН", "Морвейну нужна была не сила, а страх. Иди, Хранитель. Мост свободен."}}},
  {"СУМЕРЕЧНЫЙ ЛЕС",
   {"СЕСТРА ВОРОНА", 100, 250, .9f, 82, 1.f, Col(190, 100, 255), 4, .65f, .22f, .24f, true, false},
   Col(25, 10, 45), Col(120, 50, 130), Col(230, 170, 255), 2,
   {{"ВОРОНА", "Тише, светлячок. В моём лесу свет не любят."},
    {"АШЕН", "Мне нужен осколок, ведьма."}},
   {{"ВОРОНА", "Он обещал мне новое солнце. А ты правда несёшь его."},
    {"ВОРОНА", "Берегись Дракара. Он не знает жалости."}}},
  {"ЗАЛ ЖАТВЫ",
   {"ДРАКАР ЖНЕЦ", 130, 190, 1.1f, 130, 1.15f, Col(255, 70, 70), 3, .66f, .26f, .25f, false, false},
   Col(35, 5, 10), Col(150, 25, 30), Col(255, 90, 70), 1,
   {{"ДРАКАР", "Я срезал столько Фонарей... Твой будет сто двадцатым."}},
   {{"ДРАКАР", "Коса выпала из рук. Впервые за триста лет я вижу рассвет в чьих-то глазах."}}},
  {"ТРОН НОЧИ",
   {"МОРВЕЙН, ВЛАДЫКА НОЧИ", 200, 250, 1.2f, 115, 1.1f, Col(235, 220, 255), 0, .75f, .33f, .20f, true, true},
   Col(3, 3, 12), Col(40, 30, 80), Col(225, 225, 255), 0,
   {{"МОРВЕЙН", "Четыре осколка у тебя. Это впечатляет, Хранитель."},
    {"АШЕН", "Ты разбил Печать. Верни солнцу его путь."},
    {"МОРВЕЙН", "Я был первым Хранителем. Я видел, что рассвет сжигает всё, что тень сумела сберечь."},
    {"АШЕН", "Тогда я принесу рассвет, который греет, а не жжёт."}},
   {{"МОРВЕЙН", "Значит, так тому и быть..."}}},
  {"РАССВЕТ", HERO, Col(255, 150, 90), Col(255, 225, 160), Col(255, 250, 220), 0, {}, {}},
};

// ============================================================================
//  БОЕВАЯ МОДЕЛЬ
// ============================================================================
enum St { IDLE, WALK, JUMP, PUNCH, KICK, WEAPON, MAGIC, BLOCK, HIT, DEAD };
struct Atk { float wind, act, rec, dmg, kb, gain; };  // замах, активная фаза, восстановление
static const Atk ATKS[4] = {{.07f, .08f, .12f, 6, 140, 8}, {.13f, .09f, .20f, 10, 230, 10},
                            {.18f, .12f, .28f, 15, 320, 14}, {.30f, .05f, .30f, 0, 0, 0}};
struct Input { int mv = 0; bool jump = false, block = false, punch = false, kick = false, weapon = false, magic = false; };
struct Fighter {
  const Stats* s = nullptr; bool player = false;
  float x = 0, y = 0, vx = 0, vy = 0; int face = 1;
  float hp = 1, maxhp = 1, en = 0, dmgMul = 1, magMul = 1;
  St st = IDLE; float t = 0, anim = 0, stun = 0, armor = 0; bool landed = false, fired = false;
  Input in;
  bool attacking() const { return st >= PUNCH && st <= MAGIC; }
};
struct Proj { float x, y, vx, dmg, life; Fighter* own; Col c; };
struct Spark { float x, y, vx, vy, life, maxlife; Col c; float r; };
struct AIState { int plan = 0; float timer = 0; int atk = 0; };
struct Keys {
  bool l = 0, r = 0, u = 0, d = 0, pUp = 0, pDown = 0, pOk = 0; bool tap[4] = {0, 0, 0, 0};
  void clearTaps() { pUp = pDown = pOk = 0; for (auto& t : tap) t = 0; }
};
enum Scene { TITLE, DIALOG, FIGHT, RESULT, SHOP };
enum Next { N_PRE, N_FIGHT, N_SHOP, N_TITLE };

struct Game {
  Scene sc = TITLE; Next nx = N_PRE; int stage = 0, bg = 4, coins = 0, lv[3] = {0, 0, 0};
  Fighter p, e; AIState ai; vector<Proj> pr; vector<Spark> sp;
  float timeScale = 1, slowT = 0, shake = 0, clock = 0, endT = 0, introT = 0, buf[4] = {0, 0, 0, 0};
  bool won = false; int reward = 0;
  const vector<Line>* dl = nullptr; size_t di = 0; float rev = 0; int sel = 0;
  void init();
  void setupDuo(const Stats* foe);
  void dialog(const vector<Line>* d, Next n, int bgIdx, const Stats* foe);
  void afterDialog();
  void startFight();
  void simulate(float dt);
  void update(float rdt, const Keys& k);
  void draw();
};
static Game G;

static void sparks(float x, float y, Col c, int n) {
  for (int i = 0; i < n; i++) {
    float a = rnd() * 6.283f, v = 80 + rnd() * 260;
    G.sp.push_back({x, y, cosf(a) * v, sinf(a) * v - 80, .35f + rnd() * .3f, .65f, c, 2 + rnd() * 3});
  }
}
static void hurt(Fighter& d, Fighter& a, float dmg, float kb, Col sc) {
  float dir = d.x >= a.x ? 1.f : -1.f;
  bool blocked = d.st == BLOCK && (a.x - d.x) * d.face > 0;
  float hy = GROUND - d.y - 55 * d.s->scale;
  if (blocked) {
    d.hp -= dmg * .12f; d.vx = dir * 90; sparks(d.x - dir * 22, hy, Col(200, 230, 255), 8);
    G.shake = max(G.shake, 2.f); d.en = min(100.f, d.en + 3);
  } else {
    d.hp -= dmg;
    if (d.armor <= 0) { d.st = HIT; d.t = 0; d.stun = .24f + min(.2f, dmg * .01f); d.vx = dir * kb; }
    else d.vx = dir * kb * .3f;  // «броня» после оглушения: защита от бесконечных серий
    sparks(d.x, hy, sc, 14); G.shake = max(G.shake, min(9.f, dmg * .4f));
  }
  a.en = min(100.f, a.en + (blocked ? 3.f : dmg * .5f));
  if (d.hp <= 0) {
    d.hp = 0; d.st = DEAD; d.t = 0; d.vx = dir * 260; G.timeScale = .3f; G.slowT = 1.f; G.shake = 10;
    sparks(d.x, hy, sc, 36);
  }
}
static void tryHit(Fighter& a, Fighter& d) {
  if (!a.attacking() || a.st == MAGIC || a.landed || d.st == DEAD || a.st == DEAD) return;
  const Atk& A = ATKS[a.st - PUNCH];
  if (a.t < A.wind || a.t > A.wind + A.act) return;
  float reach = a.st == PUNCH ? 60.f : a.st == KICK ? 85.f : a.s->reach;
  float dx = (d.x - a.x) * a.face;
  if (dx < -15 || dx > reach + 20 || fabsf(a.y - d.y) > 75) return;
  a.landed = true;
  hurt(d, a, A.dmg * a.s->dmg * (a.player ? a.dmgMul : 1.f), A.kb, a.s->glow);
}
static void updateFighter(Fighter& f, Fighter& o, float dt) {
  f.anim += dt; f.t += dt; f.armor = max(0.f, f.armor - dt);
  f.en = min(100.f, f.en + ((f.s->magic && !f.player) ? 9.f : 2.5f) * dt);
  if (f.st == DEAD) { f.vx -= f.vx * 4 * dt; f.x = clampf(f.x + f.vx * dt, 40, W - 40); return; }
  bool air = f.y > 0;
  if (!f.attacking() && f.st != HIT) f.face = (o.x >= f.x) ? 1 : -1;
  float spd = f.s->speed * ((f.s->boss && f.hp < f.maxhp * .5f) ? 1.25f : 1.f);
  if (f.st == HIT) {
    f.vx -= f.vx * 6 * dt;
    if (f.t >= f.stun) { f.st = air ? JUMP : IDLE; f.armor = .55f; }
  } else if (f.attacking()) {
    const Atk& A = ATKS[f.st - PUNCH];
    if (f.st == MAGIC && !f.fired && f.t >= A.wind) {
      f.fired = true; float S = f.s->scale;
      float dmg = f.player ? 22.f * f.magMul : 14.f * f.s->dmg;
      G.pr.push_back({f.x + f.face * 45.f, GROUND - f.y - 60 * S, f.face * 540.f, dmg, 3.f, &f, f.s->glow});
    }
    if (f.t >= A.wind + A.act + A.rec) f.st = air ? JUMP : IDLE;
    f.vx -= f.vx * 8 * dt;
  } else {
    Input& in = f.in;
    if (!air && in.block) { f.st = BLOCK; f.vx = 0; }
    else {
      if (f.st == BLOCK) f.st = IDLE;
      int atk = in.punch ? PUNCH : in.kick ? KICK : in.weapon ? WEAPON : (in.magic && f.en >= 40) ? MAGIC : -1;
      if (atk >= 0 && (!air || atk == KICK || atk == WEAPON)) {
        f.st = (St)atk; f.t = 0; f.landed = f.fired = false;
        if (atk == MAGIC) f.en -= 40;
        f.vx = f.face * (atk == WEAPON ? 110.f : atk == KICK ? 70.f : 50.f);
      } else if (!air) {
        if (in.jump) { f.vy = 540; f.st = JUMP; f.vx = in.mv * spd; }
        else { f.vx = in.mv * spd * ((in.mv == f.face || in.mv == 0) ? 1.f : .7f); f.st = in.mv ? WALK : IDLE; }
      }
    }
  }
  if (f.y > 0 || f.vy > 0) {
    f.vy -= 1500 * dt; f.y += f.vy * dt;
    if (f.y <= 0) { f.y = 0; f.vy = 0; if (f.st == JUMP) f.st = IDLE; }
  }
  f.x = clampf(f.x + f.vx * dt, 50, W - 50);
}
// ИИ противника (в тестах им же управляется «игрок-бот»)
static void think(Fighter& e, const Fighter& p, AIState& a, float dt) {
  e.in = Input{};
  float d = fabsf(p.x - e.x); int toward = p.x > e.x ? 1 : -1; float rch = e.s->reach * .9f;
  bool boss2 = e.s->boss && e.hp < e.maxhp * .5f;
  a.timer -= dt;
  bool threat = p.attacking() && p.st != MAGIC && d < rch + 60;
  for (auto& q : G.pr) if (q.own != &e && fabsf(q.x - e.x) < 240 && (q.vx > 0) == (e.x > q.x)) threat = true;
  if (threat && a.plan != 4 && rnd() < e.s->blk * dt * 8) { a.plan = 4; a.timer = .4f; }
  if (a.timer <= 0) {
    a.timer = e.s->think * (.6f + rnd() * .8f) * (boss2 ? .7f : 1.f);
    float r = rnd(), ag = e.s->aggr + (boss2 ? .1f : 0.f);
    if (e.s->magic && e.en >= 40 && d > 140 && r < .45f) a.plan = 5;
    else if (d > rch) a.plan = (r < ag) ? 1 : (rnd() < .5f ? 0 : 2);
    else if (r < ag) { a.plan = 3; float q = rnd(); a.atk = (d < 55 && q < .3f) ? PUNCH : (d < 80 && q < .55f) ? KICK : WEAPON; }
    else { a.plan = rnd() < .5f ? 4 : 2; if (a.plan == 4) a.timer = .35f; }
  }
  if (a.plan == 2 && (e.x < 75 || e.x > W - 75)) a.plan = 0;
  switch (a.plan) {
    case 1: e.in.mv = toward; break;
    case 2: e.in.mv = -toward; break;
    case 3:
      if (d > rch + 10) e.in.mv = toward;
      else { e.in.punch = a.atk == PUNCH; e.in.kick = a.atk == KICK; e.in.weapon = a.atk == WEAPON; a.plan = 0; a.timer = e.s->think * .7f; }
      break;
    case 4: e.in.block = true; break;
    case 5: e.in.magic = true; a.plan = 0; break;
  }
}

// ---- скелет и поза бойца (процедурная анимация) ----------------------------
struct Skel { Pt hip, neck, head, sh, el[2], hd[2], kn[2], ft[2]; float S; };
static Skel skel(const Fighter& f, float tt) {
  Skel k; float S = f.s->scale; int fc = f.face; k.S = S;
  float lean = .12f, a1[2] = {1.0f, .7f}, a2[2] = {2.3f, 2.0f}, l1[2] = {.3f, -.3f}, l2[2] = {.1f, -.1f}, ph = f.anim;
  auto mix = [](float a, float b, float t) { return a + (b - a) * t; };
  float e = 0, w = 0, sw = 0, rc = 0; int phase = 0;
  if (f.attacking() && f.st != MAGIC) {
    const Atk& A = ATKS[f.st - PUNCH];
    if (tt < A.wind) { w = tt / A.wind; e = -.4f * w; phase = 0; }
    else if (tt < A.wind + A.act) { sw = (tt - A.wind) / A.act; e = -.4f + 1.4f * sw; phase = 1; }
    else { rc = clampf((tt - A.wind - A.act) / A.rec, 0, 1); e = 1 - rc; phase = 2; }
  }
  switch (f.st) {
    case WALK: { float s = sinf(ph * 11); l1[0] = .65f * s; l1[1] = -.65f * s; l2[0] = l1[0] - .45f; l2[1] = l1[1] - .45f;
      lean = .16f; a1[0] = 1.0f + .3f * s; a1[1] = .7f - .3f * s; break; }
    case JUMP: l1[0] = 1.0f; l1[1] = .2f; l2[0] = -.1f; l2[1] = -.7f; a1[0] = 1.3f; a1[1] = 1.0f; lean = .05f; break;
    case PUNCH: a1[0] = mix(1.0f, 1.55f, e); a2[0] = mix(2.3f, 1.55f, e); lean = .12f + .28f * e; break;
    case KICK: l1[0] = mix(.3f, 1.55f, e); l2[0] = mix(.1f, 1.5f, e); lean = .12f - .35f * e; break;
    case WEAPON: {
      float a = phase == 0 ? mix(1.0f, -2.4f, w) : phase == 1 ? mix(-2.4f, 1.75f, sw) : mix(1.75f, 1.0f, rc);
      a1[0] = a; a2[0] = a + (phase == 0 ? .5f : .15f);
      lean = .1f + .3f * (phase == 1 ? sw : phase == 2 ? 1 - rc : 0.f); break; }
    case MAGIC: a1[0] = a1[1] = 1.5f; a2[0] = a2[1] = 1.55f; lean = -.04f; break;
    case BLOCK: a1[0] = .5f; a1[1] = .3f; a2[0] = 2.9f; a2[1] = 2.7f; lean = -.1f; l1[0] = .7f; l1[1] = -.5f; l2[0] = -.1f; l2[1] = .1f; break;
    case HIT: lean = -.5f; a1[0] = -.4f; a1[1] = -.7f; a2[0] = -.9f; a2[1] = -1.2f; l1[0] = .5f; l1[1] = -.2f; l2[0] = .1f; l2[1] = -.4f; break;
    default: { float b = sinf(ph * 4.f); lean += b * .03f; a1[0] += b * .06f; a1[1] -= b * .06f; l1[0] += b * .03f; }
  }
  float TH = 26 * S, SH = 26 * S, TO = 38 * S, UA = 20 * S, FA = 20 * S;
  auto dir = [&](float a) { return Pt{sinf(a) * fc, cosf(a)}; };
  float fy[2]; for (int i = 0; i < 2; i++) fy[i] = cosf(l1[i]) * TH + cosf(l2[i]) * SH;
  k.hip = {f.x, GROUND - f.y - max(fy[0], fy[1])};
  for (int i = 0; i < 2; i++) {
    Pt d1 = dir(l1[i]), d2 = dir(l2[i]);
    k.kn[i] = {k.hip.x + d1.x * TH, k.hip.y + d1.y * TH}; k.ft[i] = {k.kn[i].x + d2.x * SH, k.kn[i].y + d2.y * SH};
  }
  k.neck = {k.hip.x + sinf(lean) * fc * TO, k.hip.y - cosf(lean) * TO};
  k.head = {k.neck.x + sinf(lean * 1.3f) * fc * 11 * S, k.neck.y - cosf(lean * 1.3f) * 11 * S};
  k.sh = {k.neck.x - sinf(lean) * fc * 5 * S, k.neck.y + 6 * S};
  for (int i = 0; i < 2; i++) {
    Pt d1 = dir(a1[i]), d2 = dir(a2[i]);
    k.el[i] = {k.sh.x + d1.x * UA, k.sh.y + d1.y * UA}; k.hd[i] = {k.el[i].x + d2.x * FA, k.el[i].y + d2.y * FA};
  }
  return k;
}
static void limb(Pt a, Pt b, float w, Col c) { gLine(a.x, a.y, b.x, b.y, w, c); gDisc(a.x, a.y, w / 2, c); gDisc(b.x, b.y, w / 2, c); }
static void drawWeapon(Pt el, Pt hd, const Fighter& f, float amul, bool ghost) {
  const Stats& s = *f.s; float S = s.scale; int fc = f.face;
  float dx = hd.x - el.x, dy = hd.y - el.y, l = sqrtf(dx * dx + dy * dy); if (l < 1e-3f) return;
  dx /= l; dy /= l;
  float L = s.reach * .62f; Pt tip = {hd.x + dx * L, hd.y + dy * L}, base = {hd.x - dx * 8 * S, hd.y - dy * 8 * S};
  Col gl = s.glow, core = lerpC(gl, Col(255, 255, 255), .55f);
  gLine(hd.x, hd.y, tip.x, tip.y, 10 * S, alpha(gl, (int)(46 * amul)));
  if (ghost) return;
  float nx = -dy * fc, ny = dx * fc;
  gLine(base.x, base.y, hd.x, hd.y, 4 * S, Col(30, 24, 30));
  switch (s.wt) {
    case 0: gLine(hd.x, hd.y, tip.x, tip.y, 3.5f * S, core); gLine(hd.x - nx * 8 * S, hd.y - ny * 8 * S, hd.x + nx * 8 * S, hd.y + ny * 8 * S, 3 * S, gl); break;
    case 1: gLine(hd.x, hd.y, tip.x, tip.y, 3.f * S, core); break;
    case 2: gLine(hd.x, hd.y, tip.x, tip.y, 3.f * S, Col(40, 34, 40)); gLine(tip.x - nx * 16 * S, tip.y - ny * 16 * S, tip.x + nx * 16 * S, tip.y + ny * 16 * S, 14 * S, core); break;
    case 3: gLine(hd.x, hd.y, tip.x, tip.y, 3.f * S, Col(40, 34, 40));
            gLine(tip.x, tip.y, tip.x + nx * 30 * S - dx * 6 * S, tip.y + ny * 30 * S - dy * 6 * S, 4.5f * S, core);
            gLine(tip.x + nx * 30 * S - dx * 6 * S, tip.y + ny * 30 * S - dy * 6 * S, tip.x + nx * 36 * S - dx * 24 * S, tip.y + ny * 36 * S - dy * 24 * S, 3.f * S, core); break;
    default: gLine(hd.x, hd.y, tip.x, tip.y, 3.f * S, Col(40, 34, 40)); gDisc(tip.x, tip.y, 14 * S, alpha(gl, 50)); gDisc(tip.x, tip.y, 7 * S, core); break;
  }
}
static void drawFighter(const Fighter& f) {
  const Stats& s = *f.s; float S = s.scale; int fc = f.face; Col blk(9, 9, 14), back(20, 20, 30), gl = s.glow;
  gLine(f.x - 26 * S, GROUND + 3, f.x + 26 * S, GROUND + 3, 7, Col(0, 0, 0, (uint8_t)(110 - min(90.f, f.y * .8f))));
  if (f.st == DEAD) {
    float d = f.vx >= 0 ? 1.f : -1.f; Pt hip{f.x, GROUND - 8 * S}, neck{hip.x + d * 36 * S, hip.y - 3 * S}, head{neck.x + d * 13 * S, neck.y};
    Pt k1{hip.x - d * 26 * S, GROUND - 6 * S}, k2{hip.x - d * 50 * S, GROUND - 5 * S};
    limb(hip, k1, 8 * S, blk); limb(k1, k2, 8 * S, blk); limb(hip, neck, 12 * S, blk);
    limb(neck, {neck.x - d * 10 * S, GROUND - 14 * S}, 6 * S, blk); gDisc(head.x, head.y, 9.5f * S, blk);
    gLine(f.x - d * 60 * S, GROUND - 3, f.x - d * (60 + s.reach * .5f) * S, GROUND - 3, 4 * S, alpha(gl, 200));
    return;
  }
  Skel k = skel(f, f.t);
  if (s.boss) for (int i = 3; i >= 1; i--) gDisc((k.hip.x + k.neck.x) / 2, (k.hip.y + k.neck.y) / 2, 38 * S + i * 14, alpha(gl, 10));
  limb(k.sh, k.el[1], 6.5f * S, back); limb(k.el[1], k.hd[1], 6.5f * S, back);
  limb(k.hip, k.kn[1], 8 * S, back); limb(k.kn[1], k.ft[1], 8 * S, back);
  limb(k.hip, k.kn[0], 8 * S, blk); limb(k.kn[0], k.ft[0], 8 * S, blk);
  limb(k.hip, k.neck, 12 * S, blk);
  float w1 = sinf(G.clock * 7 + f.x * .02f) * 4; Pt b{k.neck.x - fc * 16 * S, k.neck.y + 6 * S + w1}, c{b.x - fc * 14 * S, b.y + 10 * S + w1 * .8f};
  limb(k.neck, b, 4 * S, gl); limb(b, c, 3 * S, alpha(gl, 200));
  gDisc(k.head.x, k.head.y, 9.5f * S, blk); gDisc(k.head.x + fc * 4 * S, k.head.y - S, 1.9f * S, gl);
  if (f.player) { Pt m{(k.hip.x + k.neck.x) / 2 + fc * 5 * S, (k.hip.y + k.neck.y) / 2}; gDisc(m.x, m.y, 15 * S, alpha(gl, 40)); gDisc(m.x, m.y, 4 * S, gl); }
  if (f.st == WEAPON && f.t > ATKS[2].wind && f.t < ATKS[2].wind + ATKS[2].act + .06f)
    for (int i = 4; i >= 1; i--) { Skel g = skel(f, max(0.f, f.t - .02f * i)); drawWeapon(g.el[0], g.hd[0], f, 1.6f / i, true); }
  limb(k.sh, k.el[0], 6.5f * S, blk); limb(k.el[0], k.hd[0], 6.5f * S, blk);
  drawWeapon(k.el[0], k.hd[0], f, 1.f, false);
  if (f.st == MAGIC) { Pt h = k.hd[0]; float r = 6 + 10 * clampf(f.t / ATKS[3].wind, 0, 1); gDisc(h.x, h.y, r * 2, alpha(gl, 50)); gDisc(h.x, h.y, r, gl); }
}

// ============================================================================
//  ОТРИСОВКА МИРА И ИНТЕРФЕЙСА
// ============================================================================
static void drawBG(const Stage& st, float clock) {
  for (int i = 0; i < 27; i++) gRect(0, i * 20.f, W, 21, lerpC(st.sky1, st.sky2, i / 26.f));
  float sx = W * .72f, sy = 150;
  for (int i = 5; i >= 1; i--) gDisc(sx, sy, 46.f + i * 22, alpha(st.sun, 14));
  gDisc(sx, sy, 46, st.sun);
  Col far = lerpC(st.sky2, Col(0, 0, 0), .45f), near = lerpC(st.sky2, Col(0, 0, 0), .75f);
  switch (st.deco) {
    case 0:
      for (int i = 0; i < 8; i++) { float bx = i * 140.f - 40 + (i * 37) % 60, h = 90.f + (i * 53) % 80; gTri(bx - 120, GROUND, bx, GROUND - h, bx + 120, GROUND, far); }
      for (int i = 0; i < 6; i++) { float bx = i * 190.f + 40, h = 50.f + (i * 71) % 50; gTri(bx - 130, GROUND, bx, GROUND - h, bx + 130, GROUND, near); }
      break;
    case 1:
      for (int i = 0; i < 7; i++) { float bx = 40.f + i * 150, t = (float)((i * 29) % 50); gRect(bx, GROUND - 210 + t, 34, 300, far); gRect(bx - 8, GROUND - 218 + t, 50, 12, far); }
      gRect(0, GROUND - 60, W, 70, near);
      break;
    default:
      for (int i = 0; i < 14; i++) {
        float bx = i * 75.f + (i * 31) % 40, h = 130.f + (i * 47) % 70;
        for (int k = 0; k < 3; k++) gTri(bx - 30 + k * 5, GROUND - k * h * .28f, bx, GROUND - h * .45f - k * h * .28f, bx + 30 - k * 5, GROUND - k * h * .28f, i % 2 ? far : near);
      }
  }
  for (int i = 0; i < 26; i++) {  // тлеющие искры в воздухе
    float x = fmodf(i * 97.f + clock * (10 + (i % 5) * 4), (float)W), y = fmodf(H - (clock * (20 + (i % 7) * 6) + i * 53), (float)H);
    gDisc(x, y, 1.6f, alpha(st.sun, 120));
  }
  gRect(0, GROUND, W, H - GROUND, Col(8, 8, 12)); gRect(0, GROUND, W, 3, lerpC(st.sky2, Col(255, 255, 255), .12f));
}
static void bar(float x, float y, float w, float h, float v, Col fg, bool rtl = false) {
  gRect(x - 2, y - 2, w + 4, h + 4, Col(0, 0, 0, 180)); gRect(x, y, w, h, Col(45, 45, 58));
  float ww = w * clampf(v, 0, 1); gRect(rtl ? x + w - ww : x, y, ww, h, fg);
}
static Col speakerCol(const char* who, Col foe) {
  if (!strcmp(who, "АШЕН")) return HERO.glow;
  if (!strcmp(who, "ЛЭМ") || !strcmp(who, "РАССКАЗЧИК")) return Col(200, 200, 215);
  return foe;
}

// ============================================================================
//  ЛОГИКА ИГРЫ
// ============================================================================
void Game::setupDuo(const Stats* foe) {
  auto mk = [&](Fighter& f, const Stats* s, float x, bool pl) { f = Fighter(); f.s = s; f.player = pl; f.x = x; f.maxhp = f.hp = s->hp; f.face = pl ? 1 : -1; };
  mk(p, &HERO, 280, true); mk(e, foe, 680, false);
  p.maxhp = p.hp = HERO.hp + 20.f * lv[1]; p.dmgMul = 1 + .15f * lv[0]; p.magMul = 1 + .25f * lv[2];
}
void Game::init() { setupDuo(&STAGES[4].foe); bg = 4; sc = TITLE; }
void Game::dialog(const vector<Line>* d, Next n, int bgIdx, const Stats* foe) { dl = d; di = 0; rev = 0; nx = n; bg = bgIdx; setupDuo(foe); sc = DIALOG; }
void Game::afterDialog() {
  switch (nx) {
    case N_PRE: dialog(&STAGES[stage].pre, N_FIGHT, stage, &STAGES[stage].foe); break;
    case N_FIGHT: startFight(); break;
    case N_SHOP:
      if (stage >= 4) dialog(&ENDING, N_TITLE, 5, &LAM);
      else { stage++; bg = stage; sc = SHOP; sel = 0; setupDuo(&STAGES[stage].foe); }
      break;
    case N_TITLE: stage = 0; init(); break;
  }
}
void Game::startFight() {
  bg = stage; setupDuo(&STAGES[stage].foe); pr.clear(); sp.clear(); ai = AIState();
  introT = 1.6f; endT = 0; timeScale = 1; slowT = 0; shake = 0; fill(begin(buf), end(buf), 0.f); sc = FIGHT;
}
void Game::simulate(float dt) {
  updateFighter(p, e, dt); updateFighter(e, p, dt);
  tryHit(p, e); tryHit(e, p);
  for (auto& q : pr) {
    q.x += q.vx * dt; q.life -= dt; Fighter& d = (q.own == &p) ? e : p;
    if (d.st != DEAD && fabsf(q.x - d.x) < 28 * d.s->scale && d.y < 110) { hurt(d, *q.own, q.dmg, 260, q.c); q.life = 0; }
    if (q.x < -50 || q.x > W + 50) q.life = 0;
  }
  pr.erase(remove_if(pr.begin(), pr.end(), [](const Proj& q) { return q.life <= 0; }), pr.end());
  for (auto& s : sp) { s.x += s.vx * dt; s.y += s.vy * dt; s.vy += 700 * dt; s.life -= dt; }
  sp.erase(remove_if(sp.begin(), sp.end(), [](const Spark& s) { return s.life <= 0; }), sp.end());
  float d = e.x - p.x;  // бойцы не проходят друг сквозь друга
  if (fabsf(d) < 40 && p.y < 40 && e.y < 40 && p.st != DEAD && e.st != DEAD) {
    float push = (40 - fabsf(d)) * .5f, sgn = d >= 0 ? 1.f : -1.f;
    p.x = clampf(p.x - sgn * push, 50, W - 50); e.x = clampf(e.x + sgn * push, 50, W - 50);
  }
}
void Game::update(float rdt, const Keys& k) {
  clock += rdt; shake = max(0.f, shake - rdt * 14);
  if (slowT > 0) { slowT -= rdt; if (slowT <= 0) timeScale = 1; }
  switch (sc) {
    case TITLE:
      p.anim += rdt; e.anim += rdt;
      if (k.pOk) { coins = 0; lv[0] = lv[1] = lv[2] = 0; stage = 0; dialog(&INTRO, N_PRE, 4, &LAM); }
      break;
    case DIALOG: {
      p.anim += rdt; e.anim += rdt;
      size_t n = u32((*dl)[di].txt).size(); rev += rdt * 45;
      if (k.pOk) { if (rev < n) rev = (float)n; else if (++di >= dl->size()) afterDialog(); else rev = 0; }
      break; }
    case FIGHT: {
      float dt = rdt * timeScale;
      for (int i = 0; i < 4; i++) { buf[i] = max(0.f, buf[i] - rdt); if (k.tap[i]) buf[i] = .18f; }
      bool live = introT <= 0 && p.st != DEAD && e.st != DEAD;
      p.in = Input{};
      if (live) {
        p.in.mv = (k.r ? 1 : 0) - (k.l ? 1 : 0); p.in.jump = k.u; p.in.block = k.d;
        p.in.punch = buf[0] > 0; p.in.kick = buf[1] > 0; p.in.weapon = buf[2] > 0; p.in.magic = buf[3] > 0;
      }
      if (live) think(e, p, ai, dt); else e.in = Input{};
      bool was = p.attacking();
      simulate(dt);
      if (!was && p.attacking()) fill(begin(buf), end(buf), 0.f);
      if (introT > 0) introT -= rdt;
      if (p.st == DEAD || e.st == DEAD) endT += rdt;
      if (endT > 2.2f) {
        won = e.st == DEAD && p.st != DEAD;
        reward = won ? 120 + 70 * stage + (p.hp >= p.maxhp ? 60 : 0) : 0; coins += reward; sc = RESULT;
      }
      break; }
    case RESULT:
      if (k.pOk) { if (won) dialog(&STAGES[stage].post, N_SHOP, stage, &STAGES[stage].foe); else startFight(); }
      break;
    case SHOP:
      p.anim += rdt;
      if (k.pUp) sel = (sel + 3) % 4;
      if (k.pDown) sel = (sel + 1) % 4;
      if (k.pOk) {
        if (sel == 3) dialog(&STAGES[stage].pre, N_FIGHT, stage, &STAGES[stage].foe);
        else { int price = 100 + 90 * lv[sel]; if (lv[sel] < 5 && coins >= price) { coins -= price; lv[sel]++; setupDuo(&STAGES[stage].foe); } }
      }
      break;
  }
}
void Game::draw() {
  gOX = gOY = 0;
  if (sc == FIGHT) { gOX = sinf(clock * 95) * shake; gOY = cosf(clock * 77) * shake * .6f; }
  drawBG(STAGES[bg], clock);
  switch (sc) {
    case TITLE: {
      drawFighter(p); drawFighter(e);
      gRect(0, 0, W, H, Col(0, 0, 0, 60));
      text("ТЕНИ РАССВЕТА", W / 2 + 4, 74, 7, Col(0, 0, 0, 200), 1);
      text("ТЕНИ РАССВЕТА", W / 2, 70, 7, Col(255, 225, 150), 1);
      text("ПОСЛЕДНИЙ ХРАНИТЕЛЬ ФОНАРЯ", W / 2, 140, 3, Col(220, 200, 230), 1);
      if (sinf(clock * 4) > -.3f) text("НАЖМИТЕ ENTER", W / 2, 250, 4, Col(255, 255, 255), 1);
      text("A/D - ХОДИТЬ   W - ПРЫЖОК   S - БЛОК", W / 2, 478, 2, Col(200, 200, 215), 1);
      text("J - РУКА   K - НОГА   L - ОРУЖИЕ   U - МАГИЯ", W / 2, 500, 2, Col(200, 200, 215), 1);
      break; }
    case DIALOG: {
      drawFighter(p); drawFighter(e);
      const Line& ln = (*dl)[di]; Col sc2 = speakerCol(ln.who, e.s->glow);
      gRect(40, 330, 880, 190, Col(0, 0, 0, 215)); gRect(40, 330, 880, 3, alpha(sc2, 200));
      text(ln.who, 62, 345, 3, sc2);
      auto lines = wrap(u32(ln.txt), 45); int cnt = (int)rev;
      for (size_t i = 0; i < lines.size(); i++) {
        int n = min<int>((int)lines[i].size(), max(0, cnt)); cnt -= (int)lines[i].size() + 1;
        if (n > 0) text(lines[i].substr(0, n), 62, 388 + i * 30.f, 3, Col(240, 240, 245));
      }
      text("ENTER - ДАЛЕЕ", 900, 500, 2, Col(150, 150, 165), 2);
      break; }
    case SHOP: {
      drawFighter(p);
      gRect(0, 0, W, H, Col(0, 0, 0, 150));
      text("КУЗНИЦА ХРАНИТЕЛЯ", W / 2, 34, 5, Col(255, 225, 150), 1);
      text("МОНЕТЫ: " + to_string(coins), W / 2, 86, 3, Col(255, 205, 90), 1);
      const char* nm[3] = {"МЕЧ", "БРОНЯ", "МАГИЯ"}; const char* ds[3] = {"+15% УРОНА ОРУЖИЕМ И УДАРАМИ", "+20 ЗДОРОВЬЯ", "+25% СИЛЫ МАГИЧЕСКОГО ЗАРЯДА"};
      for (int i = 0; i < 3; i++) {
        float y = 140 + i * 78.f; bool s = sel == i; int price = 100 + 90 * lv[i];
        gRect(120, y, 720, 66, s ? Col(70, 60, 30, 220) : Col(20, 20, 30, 200));
        text(string(s ? "> " : "  ") + nm[i], 140, y + 10, 4, s ? Col(255, 225, 150) : Col(200, 200, 215));
        text(ds[i], 140, y + 44, 2, Col(170, 170, 185));
        for (int k = 0; k < 5; k++) gRect(560 + k * 26.f, y + 12, 20, 20, k < lv[i] ? Col(255, 205, 90) : Col(60, 60, 75));
        text(lv[i] >= 5 ? string("МАКС") : to_string(price) + " МОНЕТ", 820, y + 42, 2, (lv[i] < 5 && coins >= price) ? Col(255, 230, 150) : Col(150, 150, 160), 2);
      }
      bool s = sel == 3; gRect(120, 380, 720, 56, s ? Col(70, 60, 30, 220) : Col(20, 20, 30, 200));
      text(string(s ? "> " : "  ") + "К СЛЕДУЮЩЕМУ БОЮ", 140, 396, 4, s ? Col(255, 225, 150) : Col(200, 200, 215));
      text("W/S - ВЫБОР   ENTER - КУПИТЬ", W / 2, 490, 2, Col(150, 150, 165), 1);
      break; }
    case FIGHT:
    case RESULT: {
      drawFighter(e); drawFighter(p);
      for (auto& q : pr) { for (int i = 3; i >= 1; i--) gDisc(q.x - q.vx * .02f * i, q.y, 10.f + i * 5, alpha(q.c, 30)); gDisc(q.x, q.y, 11, q.c); gDisc(q.x, q.y, 5, Col(255, 255, 255)); }
      for (auto& s : sp) gDisc(s.x, s.y, s.r * (s.life / s.maxlife), alpha(s.c, (int)(255 * s.life / s.maxlife)));
      gOX = gOY = 0;
      bar(30, 22, 370, 18, p.hp / p.maxhp, Col(240, 190, 70)); text(HERO.name, 30, 46, 2, Col(235, 235, 240));
      bar(30, 68, 200, 8, p.en / 100, p.en >= 40 ? Col(90, 170, 255) : Col(60, 100, 160)); text("МАГИЯ", 238, 66, 1, Col(150, 170, 200));
      bar(W - 400.f, 22, 370, 18, e.hp / e.maxhp, e.s->glow, true); text(e.s->name, W - 30, 46, 2, Col(235, 235, 240), 2);
      text("ЭТАП " + to_string(stage + 1) + "/5", W / 2, 24, 2, Col(200, 200, 215), 1);
      text("МОНЕТЫ: " + to_string(coins), W / 2, 46, 2, Col(255, 205, 90), 1);
      if (sc == FIGHT && introT > 0) {
        text(STAGES[stage].place, W / 2, 190, 5, Col(255, 255, 255, (uint8_t)clampf(introT * 500, 0, 255)), 1);
        if (introT < .7f) text("БОЙ!", W / 2, 250, 9, Col(255, 200, 80, (uint8_t)clampf(introT * 600, 0, 255)), 1);
      }
      if (sc == RESULT) {
        gRect(0, 0, W, H, Col(0, 0, 0, 170));
        text(won ? "ПОБЕДА!" : "ПОРАЖЕНИЕ", W / 2, 150, 9, won ? Col(255, 225, 150) : Col(220, 80, 80), 1);
        if (won) text("НАГРАДА: " + to_string(reward) + " МОНЕТ" + (p.hp >= p.maxhp ? " (БЕЗ ЕДИНОЙ ЦАРАПИНЫ +60)" : ""), W / 2, 260, 3, Col(255, 205, 90), 1);
        else text("ТЬМА СИЛЬНЕЕ... НО ФОНАРЬ ЕЩЁ ГОРИТ", W / 2, 260, 3, Col(200, 200, 215), 1);
        text("ENTER - ДАЛЕЕ", W / 2, 340, 3, Col(255, 255, 255), 1);
      }
      break; }
  }
}

// ============================================================================
//  ПЛАТФОРМА: SDL2
// ============================================================================
#ifdef HEADLESS
void gRect(float, float, float, float, Col) {}
void gTri(float, float, float, float, float, float, Col) {}
void gDisc(float, float, float, Col) {}
void gLine(float, float, float, float, float, Col) {}
#else
static SDL_Renderer* R;
static SDL_Vertex vtx(float x, float y, Col c) {
  SDL_Vertex v; v.position.x = x + gOX; v.position.y = y + gOY;
  v.color.r = c.r; v.color.g = c.g; v.color.b = c.b; v.color.a = c.a; v.tex_coord.x = 0; v.tex_coord.y = 0; return v;
}
void gRect(float x, float y, float w, float h, Col c) {
  SDL_SetRenderDrawColor(R, c.r, c.g, c.b, c.a); SDL_FRect r = {x + gOX, y + gOY, w, h}; SDL_RenderFillRectF(R, &r);
}
void gTri(float x1, float y1, float x2, float y2, float x3, float y3, Col c) {
  SDL_Vertex v[3] = {vtx(x1, y1, c), vtx(x2, y2, c), vtx(x3, y3, c)}; SDL_RenderGeometry(R, nullptr, v, 3, nullptr, 0);
}
void gDisc(float x, float y, float r, Col c) {
  const int N = 16; SDL_Vertex v[N + 1]; int idx[N * 3]; v[0] = vtx(x, y, c);
  for (int i = 0; i < N; i++) { float a = i * 6.2831853f / N; v[i + 1] = vtx(x + cosf(a) * r, y + sinf(a) * r, c); idx[i * 3] = 0; idx[i * 3 + 1] = i + 1; idx[i * 3 + 2] = (i + 1) % N + 1; }
  SDL_RenderGeometry(R, nullptr, v, N + 1, idx, N * 3);
}
void gLine(float x1, float y1, float x2, float y2, float w, Col c) {
  float dx = x2 - x1, dy = y2 - y1, l = sqrtf(dx * dx + dy * dy); if (l < .001f) return;
  float nx = -dy / l * w / 2, ny = dx / l * w / 2;
  SDL_Vertex v[4] = {vtx(x1 + nx, y1 + ny, c), vtx(x1 - nx, y1 - ny, c), vtx(x2 + nx, y2 + ny, c), vtx(x2 - nx, y2 - ny, c)};
  int idx[6] = {0, 1, 2, 2, 1, 3}; SDL_RenderGeometry(R, nullptr, v, 4, idx, 6);
}

int main(int, char**) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
  SDL_Window* win = SDL_CreateWindow("Тени Рассвета", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_RESIZABLE);
  R = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!win || !R) return 1;
  SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND); SDL_RenderSetLogicalSize(R, W, H);
  G.init();
  Keys k; bool run = true; Uint32 last = SDL_GetTicks(); float acc = 0; const float STEP = 1.f / 60.f;
  while (run) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) run = false;
      else if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
        switch (ev.key.keysym.sym) {
          case SDLK_ESCAPE: run = false; break;
          case SDLK_j: case SDLK_z: k.tap[0] = true; k.pOk = true; break;
          case SDLK_k: case SDLK_x: k.tap[1] = true; break;
          case SDLK_l: case SDLK_c: k.tap[2] = true; break;
          case SDLK_u: case SDLK_v: k.tap[3] = true; break;
          case SDLK_UP: case SDLK_w: k.pUp = true; break;
          case SDLK_DOWN: case SDLK_s: k.pDown = true; break;
          case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE: k.pOk = true; break;
          default: break;
        }
      }
    }
    const Uint8* ks = SDL_GetKeyboardState(nullptr);
    k.l = ks[SDL_SCANCODE_A] || ks[SDL_SCANCODE_LEFT]; k.r = ks[SDL_SCANCODE_D] || ks[SDL_SCANCODE_RIGHT];
    k.u = ks[SDL_SCANCODE_W] || ks[SDL_SCANCODE_UP]; k.d = ks[SDL_SCANCODE_S] || ks[SDL_SCANCODE_DOWN];
    Uint32 now = SDL_GetTicks(); acc += min(.1f, (now - last) / 1000.f); last = now;
    while (acc >= STEP) { G.update(STEP, k); k.clearTaps(); acc -= STEP; }
    SDL_SetRenderDrawColor(R, 0, 0, 0, 255); SDL_RenderClear(R); G.draw(); SDL_RenderPresent(R);
  }
  SDL_DestroyRenderer(R); SDL_DestroyWindow(win); SDL_Quit();
  return 0;
}
#endif
