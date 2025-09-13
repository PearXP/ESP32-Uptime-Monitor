#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <Update.h>
#include <ArduinoJson.h>

// --- Configuration Version ---
// IMPORTANT: Increment this to force a reset to defaults after an update with breaking changes.
const int CONFIG_VERSION = 10; // Version 1.0
const int NUM_TARGETS = 3;
const int EEPROM_SIZE = 2048; // Increased size for 3 targets
const int CONFIG_VERSION_ADDRESS = 2044; // Store version at the very end of EEPROM

// --- Data Structure for a single target ---
struct TargetConfig {
  char weburl[128];
  char discord_webhook_url[128];
  char ntfy_url[64];
  char ntfy_priority[16];
  char telegram_bot_token[50];
  char telegram_chat_id[16];
  char http_get_url_on[128];
  char http_get_url_off[128];
};

// --- Default Configuration (Fallback values) ---
char default_ssid[32] = "hotspot123321";
char default_password[32] = "pw123456";
int  default_gmt_offset = 1; // Default UTC+1 (CET)

// --- Global variables for operation ---
char ssid[32];
char password[32];
int gmt_offset;
TargetConfig targets[NUM_TARGETS];

// Runtime state variables
long gmtOffset_sec;
const char* ntpServer = "pool.ntp.org";
int httpCode[NUM_TARGETS] = {0};
int lastHttpCode[NUM_TARGETS] = {0};
unsigned long pingTime[NUM_TARGETS] = {0};
unsigned long minpingTime[NUM_TARGETS] = {0};
unsigned long maxpingTime[NUM_TARGETS] = {0};
bool isFirstCheck = true;
String logMessages[NUM_TARGETS]; // Separate log for each target

AsyncWebServer server(80);

// --- HTML for the Firmware Update Page ---
const char* UPDATE_HTML = R"rawliteral(
<form method='POST' action='/updatefirmware' enctype='multipart/form-data' id='upload_form'>
  <h2>Firmware Update</h2>
  <p>Upload the new .bin file here. <strong>Warning:</strong> After the update, all settings will be reset to their default values.</p>
  <input type='file' name='update' id='file' onchange='sub(this)' style='display:none'>
  <label id='file-input' for='file'>Choose File...</label>
  <input type='submit' class='btn' value='Start Update'>
  <br><br>
  <div id='prg'>Progress: 0%</div>
  <br>
  <div id='prgbar'><div id='bar'></div></div>
</form>
<script>
  function sub(obj) {
    var fileName = obj.value.split('\\\\').pop();
    document.getElementById('file-input').innerHTML = fileName;
  }
  document.getElementById('upload_form').onsubmit = function(e) {
    e.preventDefault();
    var form = document.getElementById('upload_form');
    var data = new FormData(form);
    var xhr = new XMLHttpRequest();
    xhr.open('POST', '/updatefirmware', true);
    xhr.upload.onprogress = function(evt) {
      if (evt.lengthComputable) {
        var per = Math.round((evt.loaded / evt.total) * 100);
        document.getElementById('prg').innerHTML = 'Progress: ' + per + '%';
        document.getElementById('bar').style.width = per + '%';
      }
    };
    xhr.onload = function() {
        if (xhr.status === 200) {
            alert('Update successful! The device will restart with default settings.');
        } else {
            alert('Update failed! Status: ' + xhr.status);
        }
    };
    xhr.send(data);
  };
</script>
<style>
  body { background: #121212; font-family: sans-serif; font-size: 14px; color: #e0e0e0; }
  form { background: #1e1e1e; max-width: 300px; margin: 75px auto; padding: 30px; border-radius: 8px; text-align: center; border: 1px solid #333;}
  #file-input, .btn { width: 100%; height: 44px; border-radius: 4px; margin: 10px auto; font-size: 15px; }
  .btn { background: #3498db; color: #fff; cursor: pointer; border: 0; padding: 0 15px; }
  .btn:hover { background-color: #2980b9; }
  #file-input { padding: 0; border: 1px solid #333; line-height: 44px; text-align: left; display: block; cursor: pointer; padding-left: 10px; }
  #prgbar { background-color: #333; border-radius: 10px; }
  #bar { background-color: #3498db; width: 0%; height: 10px; border-radius: 10px; }
</style>
)rawliteral";


// --- HTML, CSS & JS for the modern web interface ---
const char* INDEX_HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Server Status</title>
    <style>
        :root {
            --bg-color: #121212;
            --card-bg: #1e1e1e;
            --font-color: #e0e0e0;
            --color-green: #2ecc71;
            --color-red: #e74c3c;
            --color-blue: #3498db;
            --color-orange: #f39c12;
            --border-color: #333;
            --border-radius: 8px;
        }
        * { box-sizing: border-box; }
        body { background-color: var(--bg-color); color: var(--font-color); font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; margin: 0; padding: 20px; }
        .container { max-width: 900px; margin: auto; }
        .main-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; flex-wrap: wrap; gap: 10px; }
        .main-header h1 { margin: 0; }
        
        .status-box { display: flex; align-items: center; background: #333; padding: 5px 10px; border-radius: 5px; }
        .status-indicator { width: 12px; height: 12px; border-radius: 50%; margin-right: 8px; background-color: #7f8c8d; transition: background-color 0.3s; }
        .online .status-indicator { background-color: var(--color-green); }
        .offline .status-indicator { background-color: var(--color-red); }

        .card { background: var(--card-bg); padding: 20px; border-radius: var(--border-radius); border: 1px solid var(--border-color); margin-top: 20px; }
        .timeline { position: relative; padding-left: 20px; border-left: 2px solid var(--border-color); max-height: 400px; overflow-y: auto; }
        .timeline-event { margin-bottom: 15px; position: relative; padding: 10px; background: rgba(255, 255, 255, 0.05); border-radius: 6px; }
        .timeline-event::before { content: ''; position: absolute; left: -28px; top: 15px; width: 12px; height: 12px; border-radius: 50%; background: var(--bg-color); border: 2px solid var(--border-color); }
        .status-on { border-left: 4px solid var(--color-green); }
        .status-on::before { border-color: var(--color-green); }
        .status-off { border-left: 4px solid var(--color-red); }
        .status-off::before { border-color: var(--color-red); }
        .timeline-event time { display: block; font-size: 0.8em; color: #999; margin-bottom: 5px; }
        .timeline-event p { margin:0; }
        
        button, .button-link { background: var(--color-blue); color: white !important; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.2s; font-size: 1em; text-decoration: none; display: inline-block; text-align: center; }
        button:hover, .button-link:hover { background-color: #2980b9; }
        
        .modal { display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100%; height: 100%; overflow: auto; background-color: rgba(0,0,0,0.7); }
        .modal-content { background-color: var(--card-bg); margin: 5% auto; padding: 0; border: 1px solid var(--border-color); width: 90%; max-width: 700px; border-radius: var(--border-radius); }
        .modal-header { padding: 15px 20px; border-bottom: 1px solid var(--border-color); display: flex; justify-content: space-between; align-items: center; }
        .modal-header h2 { margin: 0; }
        .close { color: #aaa; font-size: 28px; font-weight: bold; cursor: pointer; }
        .modal-body { padding: 20px; max-height: 60vh; overflow-y: auto;}
        .modal-footer { padding: 15px 20px; border-top: 1px solid var(--border-color); text-align: right; }
        
        .tab-buttons { border-bottom: 1px solid var(--border-color); padding: 0 10px;}
        .tab-buttons button { background: none; border: none; padding: 10px 15px; cursor: pointer; color: #888; border-bottom: 3px solid transparent; font-size: 1em; }
        .tab-buttons button.active { color: var(--color-blue); border-bottom-color: var(--color-blue); }
        .tab-content, .server-tab-content { display: none; }
        .tab-content.active, .server-tab-content.active { display: block; }

        input, select { width: 100%; padding: 10px; margin: 5px 0; display: inline-block; border: 1px solid var(--border-color); border-radius: 4px; background-color: #333; color: var(--font-color); }
        form label.section-label { margin-top: 20px; display: block; font-weight: bold; font-size: 1.1em; color: var(--font-color); border-bottom: 1px solid var(--border-color); padding-bottom: 5px; margin-bottom: 15px; }
        form label { color: #ccc; }
        p.description { font-size: 0.8em; color: #888; margin-top: -2px; margin-bottom: 8px; }
        .form-group { margin-bottom: 15px; }
        hr { border-color: var(--border-color); margin: 20px 0; }
        .version-info { text-align: center; margin-top: 15px; color: #888; }
        .server-details-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px;}
    </style>
</head>
<body>
    <div class="container">
        <header class="main-header">
            <h1>Server Status Monitor</h1>
            <button id="settingsBtn">Settings</button>
        </header>

        <main>
            <div class="tab-buttons" id="server-tabs">
                <!-- Server tabs generated here -->
            </div>
            <div id="server-content">
                <!-- Server content generated here -->
            </div>
        </main>
    </div>

    <div id="settingsModal" class="modal">
      <div class="modal-content">
        <div class="modal-header">
            <h2>Settings</h2>
            <span class="close">&times;</span>
        </div>
        <div class="tab-buttons" id="settings-tabs">
            <button class="tab-link active" onclick="openSettingsTab(event, 'general')">General</button>
            <button class="tab-link" onclick="openSettingsTab(event, 'server_0')">Server 1</button>
            <button class="tab-link" onclick="openSettingsTab(event, 'server_1')">Server 2</button>
            <button class="tab-link" onclick="openSettingsTab(event, 'server_2')">Server 3</button>
        </div>
        <form id="settingsForm">
            <div id="general" class="modal-body tab-content active">
                <label class="section-label">General</label>
                <div class="form-group"><label for="ssid_input">WiFi SSID</label><input type="text" id="ssid_input" name="ssid"></div>
                <div class="form-group"><label for="password_input">WiFi Password</label><p class="description">Leave blank to keep the current password.</p><input type="password" id="password_input" name="password"></div>
                <div class="form-group"><label for="gmt_offset_input">UTC Offset (hours)</label><input type="number" id="gmt_offset_input" name="gmt_offset" min="-12" max="14"></div>
            </div>

            <!-- Server Settings Tabs will be generated by JS -->

            <div class="modal-footer">
                <button type="submit">Save & Restart</button>
            </div>
        </form>
        <div class="modal-body">
             <hr>
            <a href="/update" class="button-link" style="width: 100%; background-color: var(--color-orange);">Firmware Update</a>
            <p class="version-info" id="firmware_version">Current Version: ...</p>
        </div>
      </div>
    </div>

    <script>
        function openSettingsTab(evt, tabName) {
            let i, tabcontent, tablinks;
            tabcontent = document.getElementById('settingsForm').getElementsByClassName("tab-content");
            for (i = 0; i < tabcontent.length; i++) { tabcontent[i].style.display = "none"; }
            tablinks = document.getElementById('settings-tabs').getElementsByClassName("tab-link");
            for (i = 0; i < tablinks.length; i++) { tablinks[i].className = tablinks[i].className.replace(" active", ""); }
            document.getElementById(tabName).style.display = "block";
            evt.currentTarget.className += " active";
        }
        function openServerTab(evt, tabName) {
            let i, tabcontent, tablinks;
            tabcontent = document.getElementsByClassName("server-tab-content");
            for (i = 0; i < tabcontent.length; i++) { tabcontent[i].style.display = "none"; }
            tablinks = document.getElementById('server-tabs').getElementsByClassName("tab-link");
            for (i = 0; i < tablinks.length; i++) { tablinks[i].className = tablinks[i].className.replace(" active", ""); }
            document.getElementById(tabName).style.display = "block";
            evt.currentTarget.className += " active";
        }

        document.addEventListener('DOMContentLoaded', () => {
            let refreshIntervalId;
            const settingsForm = document.getElementById('settingsForm');
            for(let i = 0; i < 3; i++) {
                const serverTab = document.createElement('div');
                serverTab.id = `server_${i}`;
                serverTab.className = 'modal-body tab-content';
                serverTab.innerHTML = `
                    <div class="form-group"><label class="section-label">Server ${i + 1}</label><label for="weburl_${i}">URL to Monitor</label><input type="text" id="weburl_${i}" name="weburl_${i}"></div>
                    <hr>
                    <label class="section-label">Notifications</label>
                    <div class="form-group"><label for="discord_webhook_${i}">Discord</label><p class="description">Enter Webhook URL. Set to '0' to disable.</p><input type="text" id="discord_webhook_${i}" name="discord_webhook_${i}"></div>
                    <div class="form-group"><label for="ntfy_url_${i}">Ntfy</label><p class="description">Enter Ntfy topic URL. Set to '0' to disable.</p><input type="text" id="ntfy_url_${i}" name="ntfy_url_${i}"></div>
                    <div class="form-group"><label for="ntfy_priority_${i}">Ntfy Priority on Failure</label><select id="ntfy_priority_${i}" name="ntfy_priority_${i}"><option value="default">Default</option><option value="min">Minimal</option><option value="high">High</option><option value="max">Maximum (critical)</option></select></div>
                    <div class="form-group"><label for="telegram_bot_token_${i}">Telegram</label><p class="description">Enter Bot Token. Set to '0' to disable.</p><input type="text" id="telegram_bot_token_${i}" name="telegram_bot_token_${i}"></div>
                    <div class="form-group"><label for="telegram_chat_id_${i}">Telegram Chat ID</label><p class="description">Set to '0' to disable.</p><input type="text" id="telegram_chat_id_${i}" name="telegram_chat_id_${i}"></div>
                    <hr>
                    <label class="section-label">Custom Actions (HTTP GET)</label>
                    <div class="form-group"><label for="http_get_url_on_${i}">On 'Server Online'</label><p class="description">URL to call when server is back online. '0' = disable.</p><input type="text" id="http_get_url_on_${i}" name="http_get_url_on_${i}"></div>
                    <div class="form-group"><label for="http_get_url_off_${i}">On 'Server Offline'</label><p class="description">URL to call when server goes offline. '0' = disable.</p><input type="text" id="http_get_url_off_${i}" name="http_get_url_off_${i}"></div>`;
                settingsForm.insertBefore(serverTab, settingsForm.querySelector('.modal-footer'));
            }

            const modal = document.getElementById('settingsModal');
            const settingsBtn = document.getElementById('settingsBtn');
            const closeBtn = document.querySelector('.close');

            function startAutoRefresh() { clearInterval(refreshIntervalId); fetchData(); refreshIntervalId = setInterval(fetchData, 5000); }
            function stopAutoRefresh() { clearInterval(refreshIntervalId); }

            settingsBtn.onclick = () => { stopAutoRefresh(); modal.style.display = 'block'; }
            closeBtn.onclick = () => { modal.style.display = 'none'; startAutoRefresh(); }
            window.onclick = (event) => { if (event.target == modal) { modal.style.display = 'none'; startAutoRefresh(); } }

            async function fetchData() {
                try {
                    const response = await fetch('/api/status');
                    if (!response.ok) throw new Error('Network response was not ok');
                    const data = await response.json();
                    updateUI(data);
                } catch (error) { console.error('Error fetching status data:', error); }
            }

            function updateUI(data) {
                const serverTabs = document.getElementById('server-tabs');
                const serverContent = document.getElementById('server-content');
                
                // Only build tabs once
                if(serverTabs.childElementCount === 0) {
                    data.targets.forEach((target, i) => {
                        const tabButton = document.createElement('button');
                        tabButton.className = `tab-link ${i===0 ? 'active':''}`;
                        tabButton.onclick = (event) => openServerTab(event, `server_content_${i}`);
                        tabButton.innerHTML = `Server ${i+1}`;
                        serverTabs.appendChild(tabButton);

                        const contentDiv = document.createElement('div');
                        contentDiv.id = `server_content_${i}`;
                        contentDiv.className = `server-tab-content ${i===0 ? 'active':''}`;
                        serverContent.appendChild(contentDiv);
                    });
                }
                
                data.targets.forEach((target, i) => {
                    const isOnline = target.http_code >= 200 && target.http_code < 400;
                    const contentDiv = document.getElementById(`server_content_${i}`);
                    contentDiv.innerHTML = `
                        <div class="card">
                            <div class="main-header">
                                <h2 style="word-break:break-all;">${target.config.weburl}</h2>
                                <div class="status-box ${isOnline ? 'online' : 'offline'}">
                                    <span class="status-indicator"></span>
                                    <span>${isOnline ? 'Online' : `Offline (${target.http_code})`}</span>
                                </div>
                            </div>
                            <div class="server-details-grid">
                                <div class="card"><p><strong>Ping:</strong> ${target.ping.last} ms</p></div>
                                <div class="card"><p><strong>Min Ping:</strong> ${target.ping.min} ms</p></div>
                                <div class="card"><p><strong>Max Ping:</strong> ${target.ping.max} ms</p></div>
                            </div>
                        </div>
                        <div class="card">
                            <h2>Uptime Log</h2>
                            <div class="timeline" id="timeline_${i}"></div>
                        </div>
                    `;

                    // Render timeline for this server
                    const timeline = document.getElementById(`timeline_${i}`);
                    const logEntries = target.log.split('\n').filter(e => e.trim() !== '');
                    if (logEntries.length === 0) {
                        timeline.innerHTML = '<p>No log entries yet.</p>';
                    } else {
                        logEntries.forEach(entry => {
                            const parts = entry.split(';');
                            if (parts.length < 2) return;
                            const status = parts[0];
                            const time = parts[1];
                            const isEntryOnline = (status === 'on');
                            const text = isEntryOnline ? 'Server Online' : 'Server Offline';
                            const eventDiv = document.createElement('div');
                            eventDiv.className = `timeline-event ${isEntryOnline ? 'status-on' : 'status-off'}`;
                            eventDiv.innerHTML = `<time>${time}</time><p>${text}</p>`;
                            timeline.appendChild(eventDiv);
                        });
                    }

                    // Populate settings form for this server
                    document.getElementById(`weburl_${i}`).value = target.config.weburl;
                    document.getElementById(`discord_webhook_${i}`).value = target.config.discord_webhook;
                    document.getElementById(`ntfy_url_${i}`).value = target.config.ntfy_url;
                    document.getElementById(`ntfy_priority_${i}`).value = target.config.ntfy_priority;
                    document.getElementById(`telegram_bot_token_${i}`).value = target.config.telegram_bot_token;
                    document.getElementById(`telegram_chat_id_${i}`).value = target.config.telegram_chat_id;
                    document.getElementById(`http_get_url_on_${i}`).value = target.config.http_get_url_on;
                    document.getElementById(`http_get_url_off_${i}`).value = target.config.http_get_url_off;
                });
                
                document.getElementById('ssid_input').value = data.general_config.ssid;
                document.getElementById('gmt_offset_input').value = data.general_config.gmt_offset;
                document.getElementById('firmware_version').textContent = 'Current Version: ' + (data.firmware_version / 10).toFixed(1);
            }

            settingsForm.addEventListener('submit', (e) => {
                e.preventDefault();
                const formData = new FormData(e.target);
                const params = new URLSearchParams();
                for (const pair of formData) { params.append(pair[0], pair[1]); }
                fetch('/api/settings', { method: 'POST', body: params }).then(res => {
                    if(res.ok) {
                        alert('Settings saved! The device will now restart.');
                        modal.style.display = 'none';
                    } else { alert('Error saving settings.'); }
                });
            });
            
            startAutoRefresh();
        });
    </script>
</body>
</html>
)rawliteral";

// --- HELPER FUNCTIONS ---

String urlEncode(const char* msg) {
  String encodedMsg = "";
  char c;
  char hex_buf[4];
  while (*msg) {
    c = *msg;
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedMsg += c;
    } else {
      sprintf(hex_buf, "%%%02X", c);
      encodedMsg += hex_buf;
    }
    msg++;
  }
  return encodedMsg;
}

void resetToDefault() {
  Serial.println("Resetting to default values...");
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.put(CONFIG_VERSION_ADDRESS, CONFIG_VERSION);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("EEPROM cleared. Defaults will be loaded on next boot.");
}

void loadConfig() {
    EEPROM.begin(EEPROM_SIZE);
    
    // Load general settings
    EEPROM.get(0, ssid);
    EEPROM.get(32, password);
    EEPROM.get(64, gmt_offset);

    // Load settings for each target
    int address = 100; // Start address for targets
    for(int i = 0; i < NUM_TARGETS; i++) {
        EEPROM.get(address, targets[i]);
        address += sizeof(TargetConfig);
    }
    
    EEPROM.end();

    // Initialize with defaults if EEPROM is fresh
    if (strlen(ssid) == 0) {
        strcpy(ssid, default_ssid);
        strcpy(password, default_password);
        gmt_offset = default_gmt_offset;

        for(int i = 0; i < NUM_TARGETS; i++) {
          if (i == 0) {
            strcpy(targets[i].weburl, "http://localhost");
          } else {
            strcpy(targets[i].weburl, "0");
          }
          strcpy(targets[i].discord_webhook_url, "0");
          strcpy(targets[i].ntfy_url, "0");
          strcpy(targets[i].ntfy_priority, "default");
          strcpy(targets[i].telegram_bot_token, "0");
          strcpy(targets[i].telegram_chat_id, "0");
          strcpy(targets[i].http_get_url_on, "0");
          strcpy(targets[i].http_get_url_off, "0");
        }
    }
    gmtOffset_sec = gmt_offset * 3600;
}

void saveConfig() {
    EEPROM.begin(EEPROM_SIZE);

    // Save general settings
    EEPROM.put(0, ssid);
    EEPROM.put(32, password);
    EEPROM.put(64, gmt_offset);

    // Save settings for each target
    int address = 100;
    for(int i = 0; i < NUM_TARGETS; i++) {
        EEPROM.put(address, targets[i]);
        address += sizeof(TargetConfig);
    }

    EEPROM.put(CONFIG_VERSION_ADDRESS, CONFIG_VERSION);
    EEPROM.commit();
    EEPROM.end();
}

String getTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Time not available";
    }
    char buffer[30];
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

void updatePingStats(int index) {
    if (pingTime[index] < minpingTime[index] || minpingTime[index] == 0) {
        minpingTime[index] = pingTime[index];
    }
    if (pingTime[index] > maxpingTime[index]) {
        maxpingTime[index] = pingTime[index];
    }
}

// --- NOTIFICATION FUNCTIONS ---

void sendNotifications(int index, const char* msg) {
    TargetConfig target = targets[index];
    
    // Discord
    if (strcmp(target.discord_webhook_url, "0") != 0 && strlen(target.discord_webhook_url) > 10) {
      HTTPClient http; http.begin(target.discord_webhook_url); http.addHeader("Content-Type", "application/json");
      String payload = "{\"content\":\"" + String(msg) + "\"}"; http.POST(payload); http.end();
    }
    // Ntfy
    if (strcmp(target.ntfy_url, "0") != 0 && strlen(target.ntfy_url) > 10) {
      HTTPClient http; http.begin(target.ntfy_url); http.addHeader("Content-Type", "text/plain");
      http.addHeader("Priority", target.ntfy_priority); http.POST(msg); http.end();
    }
    // Telegram
    if (strlen(target.telegram_bot_token) > 10 && strcmp(target.telegram_chat_id, "0") != 0) {
      String url = "https://api.telegram.org/bot" + String(target.telegram_bot_token) + "/sendMessage?chat_id=" + String(target.telegram_chat_id) + "&text=" + urlEncode(msg);
      HTTPClient http; http.begin(url); http.GET(); http.end();
    }
}

void sendCustomHttpRequest(const char* url) {
    if (strcmp(url, "0") == 0 || strlen(url) < 10) return;
    HTTPClient http;
    http.begin(url);
    http.GET();
    http.end();
}


// --- SETUP ---
void setup() {
    Serial.begin(115200);
    Serial.println("\nBooting device...");

    EEPROM.begin(EEPROM_SIZE);
    int storedVersion = 0;
    EEPROM.get(CONFIG_VERSION_ADDRESS, storedVersion);
    EEPROM.end();

    if (storedVersion != CONFIG_VERSION) {
        Serial.printf("Config version mismatch! Stored: %d, Firmware: %d. Resetting to defaults.\n", storedVersion, CONFIG_VERSION);
        resetToDefault();
        delay(1000);
        ESP.restart();
    }

    pinMode(0, INPUT_PULLUP);
    delay(100);
    if (digitalRead(0) == LOW) {
        Serial.println("BOOT button pressed. Hold for 5 seconds for manual reset...");
        delay(5000);
        if (digitalRead(0) == LOW) {
            resetToDefault();
            Serial.println("Device will restart with default values...");
            delay(1000);
            ESP.restart();
        }
    }

    loadConfig();

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi: "); Serial.println(ssid);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500); Serial.print("."); retries++;
    }
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected!"); Serial.print("IP Address: "); Serial.println(WiFi.localIP());
      configTime(gmtOffset_sec, 0, ntpServer);
    } else {
      Serial.println("\nWiFi connection failed.");
    }
    
    // --- SERVER ROUTE DEFINITIONS ---

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", INDEX_HTML);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        DynamicJsonDocument json(4096);
        json["firmware_version"] = CONFIG_VERSION;
        
        JsonObject general_config = json.createNestedObject("general_config");
        general_config["ssid"] = ssid;
        general_config["gmt_offset"] = gmt_offset;

        JsonArray targets_json = json.createNestedArray("targets");
        for(int i=0; i < NUM_TARGETS; i++) {
          JsonObject target_obj = targets_json.createNestedObject();
          target_obj["http_code"] = httpCode[i];
          target_obj["log"] = logMessages[i];

          JsonObject ping = target_obj.createNestedObject("ping");
          ping["last"] = pingTime[i]; ping["min"] = minpingTime[i]; ping["max"] = maxpingTime[i];
          
          JsonObject config = target_obj.createNestedObject("config");
          config["weburl"] = targets[i].weburl;
          config["discord_webhook"] = targets[i].discord_webhook_url;
          config["ntfy_url"] = targets[i].ntfy_url;
          config["ntfy_priority"] = targets[i].ntfy_priority;
          config["telegram_bot_token"] = targets[i].telegram_bot_token;
          config["telegram_chat_id"] = targets[i].telegram_chat_id;
          config["http_get_url_on"] = targets[i].http_get_url_on;
          config["http_get_url_off"] = targets[i].http_get_url_off;
        }
        
        String response;
        serializeJson(json, response);
        request->send(200, "application/json", response);
    });

    server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request){
        // General settings
        if(request->hasParam("ssid", true)) strncpy(ssid, request->getParam("ssid", true)->value().c_str(), sizeof(ssid));
        if(request->hasParam("password", true) && request->getParam("password", true)->value().length() > 0) {
            strncpy(password, request->getParam("password", true)->value().c_str(), sizeof(password));
        }
        if(request->hasParam("gmt_offset", true)) gmt_offset = request->getParam("gmt_offset", true)->value().toInt();

        // Server-specific settings
        for(int i=0; i<NUM_TARGETS; i++) {
          String prefix;
          prefix = "weburl_" + String(i); if(request->hasParam(prefix, true)) strncpy(targets[i].weburl, request->getParam(prefix, true)->value().c_str(), sizeof(targets[i].weburl));
          prefix = "discord_webhook_" + String(i); if(request->hasParam(prefix, true)) strncpy(targets[i].discord_webhook_url, request->getParam(prefix, true)->value().c_str(), sizeof(targets[i].discord_webhook_url));
          prefix = "ntfy_url_" + String(i); if(request->hasParam(prefix, true)) strncpy(targets[i].ntfy_url, request->getParam(prefix, true)->value().c_str(), sizeof(targets[i].ntfy_url));
          prefix = "ntfy_priority_" + String(i); if(request->hasParam(prefix, true)) strncpy(targets[i].ntfy_priority, request->getParam(prefix, true)->value().c_str(), sizeof(targets[i].ntfy_priority));
          prefix = "telegram_bot_token_" + String(i); if(request->hasParam(prefix, true)) strncpy(targets[i].telegram_bot_token, request->getParam(prefix, true)->value().c_str(), sizeof(targets[i].telegram_bot_token));
          prefix = "telegram_chat_id_" + String(i); if(request->hasParam(prefix, true)) strncpy(targets[i].telegram_chat_id, request->getParam(prefix, true)->value().c_str(), sizeof(targets[i].telegram_chat_id));
          prefix = "http_get_url_on_" + String(i); if(request->hasParam(prefix, true)) strncpy(targets[i].http_get_url_on, request->getParam(prefix, true)->value().c_str(), sizeof(targets[i].http_get_url_on));
          prefix = "http_get_url_off_" + String(i); if(request->hasParam(prefix, true)) strncpy(targets[i].http_get_url_off, request->getParam(prefix, true)->value().c_str(), sizeof(targets[i].http_get_url_off));
        }
        
        saveConfig();
        request->send(200, "text/plain", "OK");
        delay(1000);
        ESP.restart();
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/html", UPDATE_HTML);
    });

    server.on("/updatefirmware", HTTP_POST, 
      [](AsyncWebServerRequest *request){ request->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK"); }, 
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if (!index) {
          Serial.printf("Update Start: %s\n", filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
        }
        if (len) {
          if (Update.write(data, len) != len) Update.printError(Serial);
        }
        if (final) {
          if (Update.end(true)) {
            Serial.printf("Update Success: %u\n", index + len);
            resetToDefault();
            Serial.println("Resetting to default settings and rebooting...");
            delay(1000);
            ESP.restart();
          } else {
            Update.printError(Serial);
          }
        }
    });

    server.begin();
}


// --- LOOP ---
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
      delay(5000);
      return;
    }
    
    for (int i=0; i<NUM_TARGETS; i++) {
      // Skip if URL is not set (is "0" or empty)
      if(strcmp(targets[i].weburl, "0") == 0 || strlen(targets[i].weburl) < 10) {
        httpCode[i] = 0; // Ensure inactive servers show as offline
        continue;
      }

      HTTPClient http;
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      http.begin(targets[i].weburl);
      
      unsigned long singleStartTime = millis();
      int currentHttpCode = http.GET();
      unsigned long singleEndTime = millis();
      http.end();

      pingTime[i] = singleEndTime - singleStartTime;
      httpCode[i] = currentHttpCode;
      updatePingStats(i);

      bool isOnline = (httpCode[i] >= 200 && httpCode[i] < 400);
      bool wasOnline = (lastHttpCode[i] >= 200 && lastHttpCode[i] < 400);

      if (isOnline != wasOnline || (isFirstCheck && httpCode[i] != 0)) {
          String statusMessage;
          String logEntry;
          String eventTime = getTime();

          if (isOnline) {
              statusMessage = "✅ Server " + String(i+1) + " is back online: " + String(targets[i].weburl);
              logEntry = "on;" + eventTime + "\n";
              if (!isFirstCheck && !wasOnline) { 
                sendNotifications(i, statusMessage.c_str());
                sendCustomHttpRequest(targets[i].http_get_url_on);
              }
          } else {
              statusMessage = "🚨 Server " + String(i+1) + " OUTAGE: " + String(targets[i].weburl) + " (Code: " + String(httpCode[i]) + ")";
              logEntry = "off;" + eventTime + "\n";
              if (!isFirstCheck || (isFirstCheck && !isOnline)) {
                sendNotifications(i, statusMessage.c_str());
                sendCustomHttpRequest(targets[i].http_get_url_off);
              }
          }
          
          logMessages[i] = logEntry + logMessages[i];
          if (logMessages[i].length() > 1000) { // Log buffer per server
              int lastNewline = logMessages[i].lastIndexOf('\n', 900);
              if (lastNewline != -1) {
                logMessages[i] = logMessages[i].substring(0, lastNewline + 1);
              }
          }
          lastHttpCode[i] = httpCode[i];
      }
      Serial.printf("[Server %d] URL: %s, Status: %d, Ping: %lu ms\n", i+1, targets[i].weburl, httpCode[i], pingTime[i]);
    }

    isFirstCheck = false;
    delay(10000); // Check all targets every 10 seconds
}

