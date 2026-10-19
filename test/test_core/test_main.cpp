// Tests de la logique pure (sans carte). Lancer : pio test -e native
#include <unity.h>

#include <string>
#include <vector>

#include "../../src/CexiiMqttCore.cpp"

using namespace cexii;

static std::vector<std::string> g_issues;
static void hook(Severity, ErrorCode, const char* src, const char* msg) {
  g_issues.push_back(std::string(src) + ": " + msg);
}

void setUp() {
  g_issues.clear();
  setIssueHook(hook);
}
void tearDown() {}

// --- username ---------------------------------------------------------------
void test_username_ok() {
  char u[64];
  TEST_ASSERT_TRUE(buildUsername(Board::Esp32PoeIso, "controlbouton", u, sizeof(u)));
  TEST_ASSERT_EQUAL_STRING("esp32poeiso_controlbouton", u);
  TEST_ASSERT_TRUE(buildUsername(Board::Esp32, "Cam1", u, sizeof(u)));
  TEST_ASSERT_EQUAL_STRING("esp32_cam1", u);  // mis en minuscules
}

void test_username_limits() {
  char u[64];
  TEST_ASSERT_TRUE(buildUsername(Board::Esp32, "abcdefghijklmnopqrstuvwx", u, sizeof(u)));  // 24
  TEST_ASSERT_FALSE(buildUsername(Board::Esp32, "abcdefghijklmnopqrstuvwxy", u, sizeof(u)));  // 25
  TEST_ASSERT_FALSE(buildUsername(Board::Esp32, "", u, sizeof(u)));
  TEST_ASSERT_FALSE(buildUsername(Board::Esp32, nullptr, u, sizeof(u)));
  TEST_ASSERT_FALSE(buildUsername(Board::Esp32, "mon nom", u, sizeof(u)));
  TEST_ASSERT_FALSE(buildUsername(Board::Esp32, "bouton_1", u, sizeof(u)));
  TEST_ASSERT_FALSE(buildUsername(Board::Esp32, "bout\xC3\xA9", u, sizeof(u)));  // accent
  TEST_ASSERT_EQUAL(6, g_issues.size());
}

// --- topics -----------------------------------------------------------------
void test_topic_ok() {
  Topic t = makeTopic(Zone::Jam, "Lumiere", "bande1", "couleur");
  TEST_ASSERT_TRUE(t.valid());
  TEST_ASSERT_EQUAL_STRING("saillance/jam/lumiere/bande1/couleur", t.c_str());
}

void test_topic_steps() {
  Topic t = TopicNeedsModule(Zone::Ecrans).module("fumee").component("machine").property("debit");
  TEST_ASSERT_TRUE(t.valid());
  TEST_ASSERT_EQUAL_STRING("saillance/ecrans/fumee/machine/debit", t.c_str());
}

void test_topic_invalid() {
  TEST_ASSERT_FALSE(makeTopic(Zone::Jam, "treize_chars_", "a", "b").valid());  // 13
  TEST_ASSERT_TRUE(makeTopic(Zone::Jam, "douze_chars_", "a", "b").valid());    // 12
  TEST_ASSERT_FALSE(makeTopic(Zone::Jam, "a/b", "a", "b").valid());
  TEST_ASSERT_FALSE(makeTopic(Zone::Jam, "a", "#", "b").valid());
  TEST_ASSERT_FALSE(makeTopic(Zone::Jam, "a", "b", "+").valid());
  TEST_ASSERT_FALSE(makeTopic(Zone::Jam, "a", "", "b").valid());
  TEST_ASSERT_FALSE(makeTopic(Zone::Jam, nullptr, "a", "b").valid());
  TEST_ASSERT_FALSE(makeTopic(Zone::Count_, "a", "b", "c").valid());
  Topic bad = TopicNeedsModule(Zone::Jam).module("a b").component("c").property("d");
  TEST_ASSERT_FALSE(bad.valid());
  TEST_ASSERT_TRUE(g_issues.size() >= 8);
}

void test_zone_names_are_valid_segments() {
  for (uint8_t i = 0; i < (uint8_t)Zone::Count_; i++)
    TEST_ASSERT_EQUAL(Check::Ok, checkSegment(zoneName((Zone)i)));
}

void test_sys_topic() {
  char t[96];
  TEST_ASSERT_TRUE(buildSysTopic("esp32_x", "infos", t, sizeof(t)));
  TEST_ASSERT_EQUAL_STRING("saillance/sys/esp32_x/infos", t);
}

// --- keys -------------------------------------------------------------------
void test_keys() {
  TEST_ASSERT_EQUAL(Check::Ok, checkKey("r"));
  TEST_ASSERT_EQUAL(Check::Ok, checkKey("abcdef"));
  TEST_ASSERT_EQUAL(Check::TooLong, checkKey("abcdefg"));
  TEST_ASSERT_EQUAL(Check::BadChar, checkKey("a-b"));
  TEST_ASSERT_EQUAL(Check::Empty, checkKey(""));
}

// --- envoi float ------------------------------------------------------------
void test_float_payload() {
  FloatChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 30, SendMode::Event);
  TEST_ASSERT_TRUE(c.addKey("r"));
  TEST_ASSERT_TRUE(c.addKey("g"));
  TEST_ASSERT_FALSE(c.addKey("r"));  // doublon
  c.set("r", 0.5f);
  c.set("g", 1.0f);
  char buf[CEXII_PAYLOAD_MAX];
  size_t n = c.build(buf, sizeof(buf), 1700000000123ULL);
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_EQUAL_STRING(
      "{\"v\":1,\"ts\":1700000000123,\"seq\":0,\"val\":[0.5,1],\"f\":\"float\",\"label\":[\"r\",\"g\"]}", buf);
  n = c.build(buf, sizeof(buf), 5);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"seq\":1"));  // le compteur avance
}

void test_float_clamp_and_unknown_key() {
  FloatChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);
  c.addKey("r");
  c.set("r", 3.0f);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, c.values[0]);
  c.set("r", -2.0f);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, c.values[0]);
  c.set("zzz", 0.5f);
  c.set("r", NAN);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, c.values[0]);
}

void test_text_payload() {
  TextChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 5, SendMode::Event);
  c.set("say \"hi\"\n");
  char buf[CEXII_PAYLOAD_MAX];
  TEST_ASSERT_TRUE(c.build(buf, sizeof(buf), 42) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"v\":1,\"ts\":42,\"seq\":0,\"val\":\"say \\\"hi\\\"\\u000a\",\"f\":\"string\"}", buf);
}

void test_buffer_too_small() {
  FloatChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);
  c.addKey("r");
  char tiny[16];
  TEST_ASSERT_EQUAL(0, c.build(tiny, sizeof(tiny), 1));
  TEST_ASSERT_EQUAL(0, c.seq);  // pas incrémenté si échec
}

// --- cadence ----------------------------------------------------------------
void test_rate_event_mode() {
  FloatChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);  // 100 ms
  c.addKey("r");
  TEST_ASSERT_FALSE(c.due(0));  // rien de changé
  c.set("r", 0.5f);
  TEST_ASSERT_TRUE(c.due(0));  // premier envoi immédiat
  c.markSent(0);
  c.set("r", 0.6f);
  TEST_ASSERT_FALSE(c.due(50));   // trop tôt : on attend
  TEST_ASSERT_TRUE(c.due(100));   // la dernière valeur part au prochain créneau
  c.markSent(100);
  TEST_ASSERT_FALSE(c.due(1000));  // plus de changement : silence
  c.set("r", 0.6f);                // même valeur : pas un événement
  TEST_ASSERT_FALSE(c.due(1000));
}

void test_rate_continuous_mode() {
  FloatChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 30, SendMode::Continuous);
  TEST_ASSERT_EQUAL(34, c.intervalMs);  // 30 Hz arrondi vers le haut : jamais > 30 Hz
  c.addKey("r");
  int sent = 0;
  for (uint32_t t = 0; t < 10000; t++) {
    if (c.due(t)) {
      c.markSent(t);
      sent++;
    }
  }
  TEST_ASSERT_INT_WITHIN(2, 294, sent);  // ~29.4 Hz pendant 10 s
}

void test_rate_limits_clamped() {
  OutChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 0, SendMode::Continuous);
  TEST_ASSERT_EQUAL(1000, c.intervalMs);
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 200, SendMode::Continuous);
  TEST_ASSERT_EQUAL(34, c.intervalMs);
  TEST_ASSERT_EQUAL(2, g_issues.size());
}

// --- réception --------------------------------------------------------------
void test_parse_float() {
  const char* j = "{\"v\":1,\"ts\":1700000000123,\"seq\":7,\"val\":[0.25,2],\"f\":\"float\",\"label\":[\"x\",\"y\"]}";
  Message m;
  TEST_ASSERT_TRUE(parseMessage(j, strlen(j), m));
  TEST_ASSERT_FALSE(m.isText);
  TEST_ASSERT_EQUAL(2, m.count);
  TEST_ASSERT_EQUAL_UINT64(1700000000123ULL, m.ts);
  TEST_ASSERT_EQUAL(7, m.seq);
  TEST_ASSERT_EQUAL_FLOAT(0.25f, m.get("x"));
  TEST_ASSERT_EQUAL_FLOAT(1.0f, m.get("y"));  // 2 ramené à 1
  TEST_ASSERT_EQUAL_FLOAT(0.5f, m.get("nope", 0.5f));
  TEST_ASSERT_TRUE(m.has("x"));
}

void test_parse_text_and_garbage() {
  const char* j = "{\"v\":1,\"ts\":1,\"seq\":0,\"val\":\"bonjour\",\"f\":\"string\"}";
  Message m;
  TEST_ASSERT_TRUE(parseMessage(j, strlen(j), m));
  TEST_ASSERT_TRUE(m.isText);
  TEST_ASSERT_EQUAL_STRING("bonjour", m.text);
  TEST_ASSERT_FALSE(parseMessage("pas du json", 11, m));
  TEST_ASSERT_FALSE(parseMessage("[1,2]", 5, m));
  const char* bad = "{\"val\":[\"a\"],\"f\":\"float\"}";
  TEST_ASSERT_FALSE(parseMessage(bad, strlen(bad), m));
}

void test_roundtrip() {
  FloatChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 30, SendMode::Event);
  c.addKey("hue");
  c.addKey("sat");
  c.set("hue", 0.123f);
  c.set("sat", 0.9f);
  char buf[CEXII_PAYLOAD_MAX];
  size_t n = c.build(buf, sizeof(buf), 99);
  Message m;
  TEST_ASSERT_TRUE(parseMessage(buf, n, m));
  TEST_ASSERT_EQUAL_FLOAT(0.123f, m.get("hue"));
  TEST_ASSERT_EQUAL_FLOAT(0.9f, m.get("sat"));
}

// --- système ----------------------------------------------------------------
void test_sys_payloads() {
  char buf[CEXII_PAYLOAD_MAX];
  buildErrorPayload(buf, sizeof(buf), 10, 3, "wifi", "lien perdu", Severity::Warning, ErrorCode::NetLinkLost);
  TEST_ASSERT_EQUAL_STRING("{\"v\":1,\"ts\":10,\"seq\":3,\"src\":\"wifi\",\"msg\":\"lien perdu\",\"sv\":1,\"t\":200}", buf);
  InfosData d;
  d.app = "jam_logique_v0.2.1.toe";
  d.fps = 59.8f; d.fpsMin = 41.2f; d.msMax = 24.3f; d.droppedFrames = 3;
  d.cpu = 18; d.ram = 61; d.gpu = 37; d.gpuMem = 52; d.ac = 1; d.performMode = 0; d.lat = 4; d.reconnects = 0;
  buildInfosPayload(buf, sizeof(buf), 1792101066169ULL, 42, d);
  TEST_ASSERT_EQUAL_STRING(
      "{\"v\":1,\"ts\":1792101066169,\"seq\":42,\"app\":\"jam_logique_v0.2.1.toe\",\"fps\":59.8,\"fpsmin\":41.2,"
      "\"msmax\":24.3,\"df\":3,\"cpu\":18,\"ram\":61,\"gpu\":37,\"gpumem\":52,\"ac\":1,\"performModeL\":0,"
      "\"lat\":4,\"rc\":0}",
      buf);
  // champs non disponibles : -1, app vide
  InfosData none;
  buildInfosPayload(buf, sizeof(buf), 10, 4, none);
  TEST_ASSERT_EQUAL_STRING(
      "{\"v\":1,\"ts\":10,\"seq\":4,\"app\":\"\",\"fps\":-1,\"fpsmin\":-1,\"msmax\":-1,\"df\":0,\"cpu\":-1,\"ram\":-1,"
      "\"gpu\":-1,\"gpumem\":-1,\"ac\":1,\"performModeL\":1,\"lat\":-1,\"rc\":0}",
      buf);
  buildPingPayload(buf, sizeof(buf), 1792101066169ULL, 42);
  TEST_ASSERT_EQUAL_STRING("{\"v\":1,\"ts\":1792101066169,\"seq\":42}", buf);
}

// --- table ------------------------------------------------------------------
void test_table_payload() {
  TableChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);
  const char* r1[] = {"a", "0.1", "on"};
  TEST_ASSERT_TRUE(c.addRow(r1, 3));
  TEST_ASSERT_TRUE(c.setCell(1, 0, "b"));
  TEST_ASSERT_TRUE(c.setCell(1, 1, "0.5"));
  TEST_ASSERT_TRUE(c.setCell(1, 2, "off"));
  char buf[CEXII_TABLE_MAX_BYTES + 1];
  size_t n = c.build(buf, sizeof(buf), 1792101066169ULL);
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_EQUAL_STRING(
      "{\"v\":1,\"ts\":1792101066169,\"seq\":0,\"val\":[[\"a\",\"0.1\",\"on\"],[\"b\",\"0.5\",\"off\"]],\"f\":\"table\"}", buf);
}

void test_table_limits() {
  TableChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);
  TEST_ASSERT_TRUE(c.setCell(63, 15, "x"));    // dernière case permise
  TEST_ASSERT_FALSE(c.setCell(64, 0, "x"));    // 65e rangée
  TEST_ASSERT_FALSE(c.setCell(0, 16, "x"));    // 17e colonne
  TableChannel full;
  full.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);
  const char* r[] = {"a"};
  for (int i = 0; i < 64; i++) TEST_ASSERT_TRUE(full.addRow(r, 1));
  TEST_ASSERT_FALSE(full.addRow(r, 1));
  const char* wide[17] = {"a","a","a","a","a","a","a","a","a","a","a","a","a","a","a","a","a"};
  TableChannel t2;
  t2.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);
  TEST_ASSERT_FALSE(t2.addRow(wide, 17));
}

void test_table_size_limit() {
  TableChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);
  std::string big(200, 'x');
  for (int y = 0; y < 64; y++)
    for (int x = 0; x < 3; x++) c.setCell(y, x, big.c_str());  // ~38 Ko
  char buf[CEXII_TABLE_MAX_BYTES + 1];
  TEST_ASSERT_EQUAL(0, c.build(buf, sizeof(buf), 1));
  TEST_ASSERT_EQUAL(0, c.seq);
}

void test_table_change_detection() {
  TableChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);
  c.setCell(0, 0, "a");
  c.markSent(0);
  c.setCell(0, 0, "a");
  TEST_ASSERT_FALSE(c.dirty);
  c.setCell(0, 0, "b");
  TEST_ASSERT_TRUE(c.dirty);
}

void test_parse_table() {
  const char* j = "{\"v\":1,\"ts\":1,\"seq\":4,\"val\":[[\"a\",\"0.1\",\"on\"],[\"b\",0.5]],\"f\":\"table\"}";
  Message m;
  TEST_ASSERT_TRUE(parseMessage(j, strlen(j), m));
  TEST_ASSERT_TRUE(m.isTable);
  TEST_ASSERT_EQUAL(2, m.rowCount());
  TEST_ASSERT_EQUAL(3, m.colCount(0));
  TEST_ASSERT_EQUAL(2, m.colCount(1));
  TEST_ASSERT_EQUAL_STRING("on", m.cell(0, 2));
  TEST_ASSERT_EQUAL_STRING("0.5", m.cell(1, 1));  // nombre toléré, converti en texte
  TEST_ASSERT_EQUAL_FLOAT(0.1f, m.cellFloat(0, 1));
  TEST_ASSERT_EQUAL_STRING("", m.cell(5, 5));
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, m.cellFloat(0, 0, -1.0f));  // "a" n'est pas un nombre
}

void test_parse_table_rejects() {
  Message m;
  std::string rows = "[";
  for (int i = 0; i < 65; i++) rows += (i ? ",[\"a\"]" : "[\"a\"]");
  rows += "]";
  std::string j = "{\"val\":" + rows + ",\"f\":\"table\"}";
  TEST_ASSERT_FALSE(parseMessage(j.c_str(), j.size(), m));  // 65 rangées
  const char* wide = "{\"val\":[[\"1\",\"2\",\"3\",\"4\",\"5\",\"6\",\"7\",\"8\",\"9\",\"a\",\"b\",\"c\",\"d\",\"e\",\"f\",\"g\",\"h\"]],\"f\":\"table\"}";
  TEST_ASSERT_FALSE(parseMessage(wide, strlen(wide), m));  // 17 colonnes
  const char* flat = "{\"val\":[\"a\",\"b\"],\"f\":\"table\"}";
  TEST_ASSERT_FALSE(parseMessage(flat, strlen(flat), m));  // pas une liste de rangées
  const char* obj = "{\"val\":[[{\"a\":1}]],\"f\":\"table\"}";
  TEST_ASSERT_FALSE(parseMessage(obj, strlen(obj), m));
}

void test_table_roundtrip() {
  TableChannel c;
  c.configure(makeTopic(Zone::Jam, "a", "b", "c"), 10, SendMode::Event);
  c.setCell(2, 1, "say \"hi\"");
  char buf[CEXII_TABLE_MAX_BYTES + 1];
  size_t n = c.build(buf, sizeof(buf), 5);
  Message m;
  TEST_ASSERT_TRUE(parseMessage(buf, n, m));
  TEST_ASSERT_EQUAL(3, m.rowCount());
  TEST_ASSERT_EQUAL_STRING("say \"hi\"", m.cell(2, 1));
  TEST_ASSERT_EQUAL_STRING("", m.cell(0, 0));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_username_ok);
  RUN_TEST(test_username_limits);
  RUN_TEST(test_topic_ok);
  RUN_TEST(test_topic_steps);
  RUN_TEST(test_topic_invalid);
  RUN_TEST(test_zone_names_are_valid_segments);
  RUN_TEST(test_sys_topic);
  RUN_TEST(test_keys);
  RUN_TEST(test_float_payload);
  RUN_TEST(test_float_clamp_and_unknown_key);
  RUN_TEST(test_text_payload);
  RUN_TEST(test_buffer_too_small);
  RUN_TEST(test_rate_event_mode);
  RUN_TEST(test_rate_continuous_mode);
  RUN_TEST(test_rate_limits_clamped);
  RUN_TEST(test_parse_float);
  RUN_TEST(test_parse_text_and_garbage);
  RUN_TEST(test_roundtrip);
  RUN_TEST(test_sys_payloads);
  RUN_TEST(test_table_payload);
  RUN_TEST(test_table_limits);
  RUN_TEST(test_table_size_limit);
  RUN_TEST(test_table_change_detection);
  RUN_TEST(test_parse_table);
  RUN_TEST(test_parse_table_rejects);
  RUN_TEST(test_table_roundtrip);
  return UNITY_END();
}
