#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "AsyncJson.h"
#include <stdarg.h>

// ─── Constants ───────────────────────────────────────────────────────────────
#define CONFIG_VER   13
#define NUM_TARGETS  3
#define LOG_SIZE     1024
#define SLOG_SIZE    2048
#define PING_HIST    30

// ─── Target configuration ────────────────────────────────────────────────────
struct Target {
    char name[32];
    char url[128];
    char discord[128];
    char ntfy_url[64];
    char ntfy_pri[16];
    char tg_token[50];
    char tg_id1[20];
    char tg_id2[20];
    char tg_id3[20];
    char http_on[128];
    char http_off[128];
    char msg_on[128];
    char msg_off[128];
    uint16_t interval;
    uint8_t  fail_thr;
    uint8_t  rec_thr;
};

// ─── Global config ───────────────────────────────────────────────────────────
char   cfg_ssid[32] = "hotspot123321";
char   cfg_pass[32] = "pw123456";
int    cfg_gmt      = 1;
Target tgt[NUM_TARGETS];

// ─── Runtime state ───────────────────────────────────────────────────────────
int           lastCode[NUM_TARGETS]              = {};
unsigned long pingMs[NUM_TARGETS]                = {};
unsigned long pingMin[NUM_TARGETS]               = {};
unsigned long pingMax[NUM_TARGETS]               = {};
unsigned long pingAvg[NUM_TARGETS]               = {};
unsigned long lastCheck[NUM_TARGETS]             = {};
uint8_t       failCnt[NUM_TARGETS]               = {};
uint8_t       sucCnt[NUM_TARGETS]                = {};
bool          isOnline[NUM_TARGETS]              = {true, true, true};
bool          forceCheck[NUM_TARGETS]            = {};
unsigned long totalChecks[NUM_TARGETS]           = {};
unsigned long onlineChecks[NUM_TARGETS]          = {};
unsigned long pingHistory[NUM_TARGETS][PING_HIST] = {};
uint8_t       pingHistIdx[NUM_TARGETS]           = {};
unsigned long bootMs                             = 0;
unsigned long lastWifiRetry                      = 0;

// ─── Log buffers ─────────────────────────────────────────────────────────────
char tgtLog[NUM_TARGETS][LOG_SIZE] = {};
char sysLog[SLOG_SIZE]             = {};
int  sysLogPos                     = 0;

Preferences    prefs;
AsyncWebServer srv(80);

// ─── Helpers ─────────────────────────────────────────────────────────────────
void safecp(char* dst, const char* src, size_t n) {
    strncpy(dst, src, n - 1);
    dst[n - 1] = '\0';
}

void getTime(char* buf, size_t n) {
    struct tm t;
    if (!getLocalTime(&t)) { strncpy(buf, "--:--:--", n); return; }
    strftime(buf, n, "%H:%M:%S", &t);
}

void syslog(const char* fmt, ...) {
    char tb[12], msg[256];
    getTime(tb, sizeof(tb));
    va_list a; va_start(a, fmt); vsnprintf(msg, sizeof(msg), fmt, a); va_end(a);
    Serial.printf("[%s] %s\n", tb, msg);
    int len = snprintf(NULL, 0, "[%s] %s\n", tb, msg);
    if (sysLogPos + len >= SLOG_SIZE) sysLogPos = 0;
    snprintf(sysLog + sysLogPos, SLOG_SIZE - sysLogPos, "[%s] %s\n", tb, msg);
    sysLogPos += len;
}

void prependLog(char* buf, const char* entry, size_t sz) {
    size_t el = strlen(entry), cl = strlen(buf);
    if (el >= sz) return;
    if (cl + el >= sz) cl = sz - el - 1;
    memmove(buf + el, buf, cl);
    memcpy(buf, entry, el);
    buf[cl + el] = '\0';
}

void urlEncode(char* dst, const char* src, size_t n) {
    size_t w = 0;
    while (*src && w + 4 < n) {
        char c = *src++;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') dst[w++] = c;
        else w += sprintf(dst + w, "%%%02X", (unsigned char)c);
    }
    dst[w] = '\0';
}

// ─── Config ──────────────────────────────────────────────────────────────────
void applyDefaults() {
    safecp(cfg_ssid, "hotspot123321", sizeof(cfg_ssid));
    safecp(cfg_pass, "pw123456",      sizeof(cfg_pass));
    cfg_gmt = 1;
    for (int i = 0; i < NUM_TARGETS; i++) {
        char n[12]; snprintf(n, sizeof(n), "Server %d", i + 1);
        safecp(tgt[i].name,     n,                               sizeof(tgt[i].name));
        safecp(tgt[i].url,      i == 0 ? "http://localhost" : "0", sizeof(tgt[i].url));
        safecp(tgt[i].discord,  "0",        sizeof(tgt[i].discord));
        safecp(tgt[i].ntfy_url, "0",        sizeof(tgt[i].ntfy_url));
        safecp(tgt[i].ntfy_pri, "default",  sizeof(tgt[i].ntfy_pri));
        safecp(tgt[i].tg_token, "0",        sizeof(tgt[i].tg_token));
        safecp(tgt[i].tg_id1,   "0",        sizeof(tgt[i].tg_id1));
        safecp(tgt[i].tg_id2,   "0",        sizeof(tgt[i].tg_id2));
        safecp(tgt[i].tg_id3,   "0",        sizeof(tgt[i].tg_id3));
        safecp(tgt[i].http_on,  "0",        sizeof(tgt[i].http_on));
        safecp(tgt[i].http_off, "0",        sizeof(tgt[i].http_off));
        safecp(tgt[i].msg_on,   "\xE2\x9C\x85 {NAME} is back online: {URL}", sizeof(tgt[i].msg_on));
        safecp(tgt[i].msg_off,  "\xF0\x9F\x9A\xA8 {NAME} OUTAGE: {URL} (Code: {CODE})", sizeof(tgt[i].msg_off));
        tgt[i].interval = 20;
        tgt[i].fail_thr = 3;
        tgt[i].rec_thr  = 2;
    }
}

void resetNVS() {
    prefs.begin("mon", false);
    prefs.clear();
    prefs.putInt("ver", CONFIG_VER);
    prefs.end();
}

void loadConfig() {
    prefs.begin("mon", true);
    prefs.getString("ssid", cfg_ssid, sizeof(cfg_ssid));
    prefs.getString("pass", cfg_pass, sizeof(cfg_pass));
    cfg_gmt = prefs.getInt("gmt", 1);
    for (int i = 0; i < NUM_TARGETS; i++) {
        char k[4]; snprintf(k, sizeof(k), "t%d", i);
        if (prefs.getBytesLength(k) == sizeof(Target))
            prefs.getBytes(k, &tgt[i], sizeof(Target));
    }
    prefs.end();
    if (strlen(cfg_ssid) == 0) applyDefaults();
}

void saveConfig() {
    prefs.begin("mon", false);
    prefs.putString("ssid", cfg_ssid);
    prefs.putString("pass", cfg_pass);
    prefs.putInt("gmt", cfg_gmt);
    prefs.putInt("ver", CONFIG_VER);
    for (int i = 0; i < NUM_TARGETS; i++) {
        char k[4]; snprintf(k, sizeof(k), "t%d", i);
        prefs.putBytes(k, &tgt[i], sizeof(Target));
    }
    prefs.end();
}

// ─── Notifications ───────────────────────────────────────────────────────────
void sendNotifications(int idx, const char* msg) {
    HTTPClient h;
    if (strcmp(tgt[idx].discord, "0") != 0 && strlen(tgt[idx].discord) > 10) {
        h.begin(tgt[idx].discord);
        h.addHeader("Content-Type", "application/json");
        char p[300]; snprintf(p, sizeof(p), "{\"content\":\"%s\"}", msg);
        h.POST(p); h.end();
    }
    if (strcmp(tgt[idx].ntfy_url, "0") != 0 && strlen(tgt[idx].ntfy_url) > 10) {
        h.begin(tgt[idx].ntfy_url);
        h.addHeader("Content-Type", "text/plain");
        h.addHeader("Priority", tgt[idx].ntfy_pri);
        h.POST(msg); h.end();
    }
    if (strlen(tgt[idx].tg_token) > 10 && strcmp(tgt[idx].tg_token, "0") != 0) {
        char enc[512]; urlEncode(enc, msg, sizeof(enc));
        const char* ids[] = { tgt[idx].tg_id1, tgt[idx].tg_id2, tgt[idx].tg_id3 };
        for (int j = 0; j < 3; j++) {
            if (strcmp(ids[j], "0") != 0 && strlen(ids[j]) > 1) {
                char url[512];
                snprintf(url, sizeof(url),
                    "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=%s",
                    tgt[idx].tg_token, ids[j], enc);
                h.begin(url); h.GET(); h.end();
            }
        }
    }
}

void httpGet(const char* url) {
    if (!url || strcmp(url, "0") == 0 || strlen(url) < 10) return;
    HTTPClient h; h.begin(url); h.GET(); h.end();
}

// ─── WiFi ────────────────────────────────────────────────────────────────────
void manageWifi() {
    if (WiFi.status() != WL_CONNECTED && millis() - lastWifiRetry > 10000) {
        lastWifiRetry = millis();
        syslog("WiFi lost. Reconnecting to %s...", cfg_ssid);
        WiFi.begin(cfg_ssid, cfg_pass);
    }
}

// ─── TCP-Ping (reines Netzwerk-RTT, ohne TLS-Overhead) ───────────────────────
unsigned long tcpPing(const char* url) {
    String u = String(url);
    int port = 80;
    String host;
    if (u.startsWith("https://")) {
        port = 443;
        host = u.substring(8);
    } else if (u.startsWith("http://")) {
        host = u.substring(7);
    } else {
        return 0;
    }
    int slash = host.indexOf('/');
    if (slash > 0) host = host.substring(0, slash);
    int colon = host.lastIndexOf(':');
    if (colon > 0) {
        port = host.substring(colon + 1).toInt();
        host = host.substring(0, colon);
    }

    WiFiClient client;
    client.setTimeout(5); // 5s Timeout
    unsigned long t0 = millis();
    bool ok = client.connect(host.c_str(), port);
    unsigned long elapsed = millis() - t0;
    client.stop();
    return ok ? elapsed : 0;
}

// ─── Monitoring ──────────────────────────────────────────────────────────────
void checkTarget(int i) {
    bool disabled = (strcmp(tgt[i].url, "0") == 0 || strlen(tgt[i].url) < 5);
    if (disabled) return;
    bool elapsed = (millis() - lastCheck[i]) >= (unsigned long)(tgt[i].interval * 1000UL);
    if (!forceCheck[i] && !elapsed) return;

    forceCheck[i] = false;
    lastCheck[i]  = millis();

    // TCP-Ping: misst reines Netzwerk-RTT (ohne TLS-Overhead)
    pingMs[i] = tcpPing(tgt[i].url);

    // HTTP-Check: prüft tatsächlichen Status/Antwortcode
    HTTPClient h;
    h.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    h.setTimeout(10000);
    h.begin(tgt[i].url);
    int code = h.GET();
    h.end();

    lastCode[i] = code;
    totalChecks[i]++;
    bool ok = (code >= 200 && code < 400);
    if (ok) onlineChecks[i]++;

    // Ping history (circular buffer, oldest overwritten)
    pingHistory[i][pingHistIdx[i]] = pingMs[i];
    pingHistIdx[i] = (pingHistIdx[i] + 1) % PING_HIST;

    if (pingMin[i] == 0 || pingMs[i] < pingMin[i]) pingMin[i] = pingMs[i];
    if (pingMs[i] > pingMax[i]) pingMax[i] = pingMs[i];
    pingAvg[i] = (pingAvg[i] == 0) ? pingMs[i] : (pingAvg[i] * 9 + pingMs[i]) / 10;

    char tb[12], entry[56];
    getTime(tb, sizeof(tb));

    if (ok) {
        failCnt[i] = 0;
        sucCnt[i]++;
        if (!isOnline[i] && sucCnt[i] >= tgt[i].rec_thr) {
            isOnline[i] = true;
            String m = tgt[i].msg_on;
            m.replace("{NAME}", tgt[i].name);
            m.replace("{URL}",  tgt[i].url);
            snprintf(entry, sizeof(entry), "[%s] ONLINE\n", tb);
            prependLog(tgtLog[i], entry, LOG_SIZE);
            sendNotifications(i, m.c_str());
            httpGet(tgt[i].http_on);
            syslog("Server %d ONLINE: %s", i + 1, tgt[i].url);
        }
    } else {
        sucCnt[i] = 0;
        failCnt[i]++;
        if (isOnline[i] && failCnt[i] >= tgt[i].fail_thr) {
            isOnline[i] = false;
            String m = tgt[i].msg_off;
            m.replace("{NAME}", tgt[i].name);
            m.replace("{URL}",  tgt[i].url);
            m.replace("{CODE}", String(code));
            snprintf(entry, sizeof(entry), "[%s] OFFLINE (code %d)\n", tb, code);
            prependLog(tgtLog[i], entry, LOG_SIZE);
            sendNotifications(i, m.c_str());
            httpGet(tgt[i].http_off);
            syslog("Server %d OFFLINE: %s (code %d)", i + 1, tgt[i].url, code);
        }
    }
    syslog("Server %d: code=%d ping=%lums uptime=%.1f%%",
        i + 1, code, pingMs[i],
        totalChecks[i] > 0 ? (float)onlineChecks[i] / totalChecks[i] * 100.0f : 0.0f);
}

// ─── HTML ────────────────────────────────────────────────────────────────────
#include "html.h"


// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    bootMs = millis();
    syslog("ESP32 Monitor V%d starting", CONFIG_VER);

    prefs.begin("mon", true);
    int storedVer = prefs.getInt("ver", 0);
    prefs.end();
    if (storedVer != CONFIG_VER) {
        syslog("Config version mismatch (%d vs %d), resetting NVS...", storedVer, CONFIG_VER);
        resetNVS();
        delay(500);
        ESP.restart();
    }

    pinMode(0, INPUT_PULLUP);
    delay(100);
    if (digitalRead(0) == LOW) {
        syslog("BOOT button held. Keep holding 5s for factory reset...");
        delay(5000);
        if (digitalRead(0) == LOW) {
            syslog("Factory reset triggered!");
            resetNVS();
            delay(500);
            ESP.restart();
        }
    }

    loadConfig();
    syslog("Config loaded. SSID: %s, GMT: %d", cfg_ssid, cfg_gmt);

    WiFi.begin(cfg_ssid, cfg_pass);
    syslog("Connecting to WiFi: %s", cfg_ssid);
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) delay(500);
    if (WiFi.status() == WL_CONNECTED) {
        syslog("WiFi connected. IP: %s", WiFi.localIP().toString().c_str());
        configTime((long)cfg_gmt * 3600L, 0, "pool.ntp.org");
    } else {
        syslog("WiFi connection failed. Will retry in background.");
    }

    // ── Routes ──────────────────────────────────────────────────────────────

    srv.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", INDEX_HTML);
    });

    srv.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/plain", sysLog);
    });

    srv.on("/api/system", HTTP_GET, [](AsyncWebServerRequest* req) {
        DynamicJsonDocument doc(512);
        doc["ip"]            = WiFi.localIP().toString();
        doc["rssi"]          = WiFi.RSSI();
        doc["free_heap"]     = ESP.getFreeHeap();
        doc["min_free_heap"] = ESP.getMinFreeHeap();
        doc["cpu_mhz"]       = getCpuFrequencyMhz();
        doc["chip_model"]    = ESP.getChipModel();
        doc["flash_size"]    = ESP.getFlashChipSize();
        doc["uptime_ms"]     = millis() - bootMs;
        doc["firmware_ver"]  = CONFIG_VER;
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    srv.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        DynamicJsonDocument doc(10240);
        JsonArray arr = doc.createNestedArray("targets");
        for (int i = 0; i < NUM_TARGETS; i++) {
            bool disabled = (strcmp(tgt[i].url, "0") == 0 || strlen(tgt[i].url) < 5);
            JsonObject t = arr.createNestedObject();
            t["name"]          = tgt[i].name;
            t["url"]           = tgt[i].url;
            t["enabled"]       = !disabled;
            t["online"]        = isOnline[i];
            t["http_code"]     = lastCode[i];
            t["ping_ms"]       = pingMs[i];
            t["ping_min"]      = pingMin[i];
            t["ping_max"]      = pingMax[i];
            t["ping_avg"]      = pingAvg[i];
            t["total_checks"]  = totalChecks[i];
            t["online_checks"] = onlineChecks[i];
            t["log"]           = tgtLog[i];
            // Ping history: oldest first (from current write index)
            JsonArray hist = t.createNestedArray("ping_history");
            for (int j = 0; j < PING_HIST; j++) {
                hist.add(pingHistory[i][(pingHistIdx[i] + j) % PING_HIST]);
            }
        }
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    srv.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* req) {
        DynamicJsonDocument doc(4096);
        doc["ssid"]       = cfg_ssid;
        doc["gmt_offset"] = cfg_gmt;
        JsonArray arr = doc.createNestedArray("targets");
        for (int i = 0; i < NUM_TARGETS; i++) {
            JsonObject t = arr.createNestedObject();
            t["name"]     = tgt[i].name;
            t["url"]      = tgt[i].url;
            t["interval"] = tgt[i].interval;
            t["fail_thr"] = tgt[i].fail_thr;
            t["rec_thr"]  = tgt[i].rec_thr;
            t["msg_on"]   = tgt[i].msg_on;
            t["msg_off"]  = tgt[i].msg_off;
            t["discord"]  = tgt[i].discord;
            t["ntfy_url"] = tgt[i].ntfy_url;
            t["ntfy_pri"] = tgt[i].ntfy_pri;
            t["tg_token"] = tgt[i].tg_token;
            t["tg_id1"]   = tgt[i].tg_id1;
            t["tg_id2"]   = tgt[i].tg_id2;
            t["tg_id3"]   = tgt[i].tg_id3;
            t["http_on"]  = tgt[i].http_on;
            t["http_off"] = tgt[i].http_off;
        }
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    // Settings POST — fields: cfg_ssid / cfg_pass / cfg_gmt / prefix_idx
    srv.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest* req) {
        for (int i = 0; i < (int)req->params(); i++) {
            auto* p = req->getParam(i);
            if (!p->isPost()) continue;
            const char* nm  = p->name().c_str();
            const char* val = p->value().c_str();

            if (strcmp(nm, "cfg_ssid") == 0) { safecp(cfg_ssid, val, sizeof(cfg_ssid)); continue; }
            if (strcmp(nm, "cfg_pass") == 0 && strlen(val) > 0) { safecp(cfg_pass, val, sizeof(cfg_pass)); continue; }
            if (strcmp(nm, "cfg_gmt")  == 0) { cfg_gmt = atoi(val); continue; }

            const char* last_ = strrchr(nm, '_');
            if (!last_) continue;
            int idx = atoi(last_ + 1);
            if (idx < 0 || idx >= NUM_TARGETS) continue;
            char prefix[32];
            size_t plen = (size_t)(last_ - nm);
            if (plen >= sizeof(prefix)) continue;
            memcpy(prefix, nm, plen);
            prefix[plen] = '\0';

            if      (strcmp(prefix, "name")     == 0) safecp(tgt[idx].name,     val, sizeof(tgt[idx].name));
            else if (strcmp(prefix, "url")      == 0) safecp(tgt[idx].url,      val, sizeof(tgt[idx].url));
            else if (strcmp(prefix, "interval") == 0) tgt[idx].interval  = (uint16_t)atoi(val);
            else if (strcmp(prefix, "fail")     == 0) tgt[idx].fail_thr  = (uint8_t)atoi(val);
            else if (strcmp(prefix, "rec")      == 0) tgt[idx].rec_thr   = (uint8_t)atoi(val);
            else if (strcmp(prefix, "msg_on")   == 0) safecp(tgt[idx].msg_on,   val, sizeof(tgt[idx].msg_on));
            else if (strcmp(prefix, "msg_off")  == 0) safecp(tgt[idx].msg_off,  val, sizeof(tgt[idx].msg_off));
            else if (strcmp(prefix, "discord")  == 0) safecp(tgt[idx].discord,  val, sizeof(tgt[idx].discord));
            else if (strcmp(prefix, "ntfy_url") == 0) safecp(tgt[idx].ntfy_url, val, sizeof(tgt[idx].ntfy_url));
            else if (strcmp(prefix, "ntfy_pri") == 0) safecp(tgt[idx].ntfy_pri, val, sizeof(tgt[idx].ntfy_pri));
            else if (strcmp(prefix, "tg_token") == 0) safecp(tgt[idx].tg_token, val, sizeof(tgt[idx].tg_token));
            else if (strcmp(prefix, "tg_id1")   == 0) safecp(tgt[idx].tg_id1,   val, sizeof(tgt[idx].tg_id1));
            else if (strcmp(prefix, "tg_id2")   == 0) safecp(tgt[idx].tg_id2,   val, sizeof(tgt[idx].tg_id2));
            else if (strcmp(prefix, "tg_id3")   == 0) safecp(tgt[idx].tg_id3,   val, sizeof(tgt[idx].tg_id3));
            else if (strcmp(prefix, "http_on")  == 0) safecp(tgt[idx].http_on,  val, sizeof(tgt[idx].http_on));
            else if (strcmp(prefix, "http_off") == 0) safecp(tgt[idx].http_off, val, sizeof(tgt[idx].http_off));
        }
        saveConfig();
        req->send(200, "text/plain", "OK");
        delay(500);
        ESP.restart();
    });

    srv.on("/api/check_now", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (req->hasParam("target", true)) {
            int idx = atoi(req->getParam("target", true)->value().c_str());
            if (idx >= 0 && idx < NUM_TARGETS) {
                forceCheck[idx] = true;
                syslog("Manual check triggered for server %d", idx + 1);
            }
        }
        req->send(200, "text/plain", "OK");
    });

    srv.on("/api/wifi_scan", HTTP_GET, [](AsyncWebServerRequest* req) {
        int n = WiFi.scanNetworks();
        DynamicJsonDocument doc(2048);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < n && i < 20; i++) {
            JsonObject net = arr.createNestedObject();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["ch"]   = WiFi.channel(i);
        }
        WiFi.scanDelete();
        String out; serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    srv.on("/updatefirmware", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            req->send(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
        },
        [](AsyncWebServerRequest* req, String fn, size_t idx,
           uint8_t* data, size_t len, bool final) {
            if (!idx) {
                syslog("OTA update started: %s", fn.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
            }
            if (len && Update.write(data, len) != len) Update.printError(Serial);
            if (final) {
                if (Update.end(true)) {
                    syslog("OTA OK: %u bytes. Restarting...", idx + len);
                    resetNVS();
                    delay(1000);
                    ESP.restart();
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );

    srv.begin();
    syslog("Web server started on port 80.");
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
    manageWifi();
    if (WiFi.status() == WL_CONNECTED) {
        for (int i = 0; i < NUM_TARGETS; i++) {
            checkTarget(i);
        }
    }
    delay(100);
}
