#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "AsyncJson.h"
#include <stdarg.h>

// --- Configuration Version ---
const int CONFIG_VERSION = 12;
const int NUM_TARGETS = 3;
const int EEPROM_SIZE = 4095;
const int CONFIG_VERSION_ADDRESS = 4090;

// --- Data Structure for a single target ---
struct TargetConfig {
    char server_name[32];
    char weburl[128];
    char discord_webhook_url[128];
    char ntfy_url[64];
    char ntfy_priority[16];
    char telegram_bot_token[50];
    char telegram_chat_id_1[16];
    char telegram_chat_id_2[16];
    char telegram_chat_id_3[16];
    char http_get_url_on[128];
    char http_get_url_off[128];
    char online_message[128];
    char offline_message[128];
    uint16_t check_interval_seconds;
    uint8_t failure_threshold;
    uint8_t recovery_threshold;
};

// --- Default Configuration (Fallback values) ---
char default_ssid[32] = "hotspot123321";
char default_password[32] = "pw123456";
int default_gmt_offset = 1;

// --- Global variables for operation ---
char ssid[32];
char password[32];
int gmt_offset;
TargetConfig targets[NUM_TARGETS];

// --- Runtime state variables ---
long gmtOffset_sec;
const char* ntpServer = "pool.ntp.org";
int httpCode[NUM_TARGETS] = {0};
unsigned long pingTime[NUM_TARGETS] = {0};
unsigned long minpingTime[NUM_TARGETS] = {0};
unsigned long maxpingTime[NUM_TARGETS] = {0};
unsigned long last_check_time[NUM_TARGETS] = {0};
uint8_t failure_count[NUM_TARGETS] = {0};
uint8_t success_count[NUM_TARGETS] = {0};
bool confirmed_online_state[NUM_TARGETS] = {true};

// --- Buffers for logs to prevent memory fragmentation ---
const int TARGET_LOG_SIZE = 1024;
const int SERIAL_LOG_SIZE = 2048;
char targetLogMessages[NUM_TARGETS][TARGET_LOG_SIZE];
char serialLogBuffer[SERIAL_LOG_SIZE];
int serialLogBufferPos = 0;

// --- WiFi Reconnection Timer ---
unsigned long lastWifiReconnectAttempt = 0;
const long wifiReconnectInterval = 10000; // Try to reconnect every 10 seconds

AsyncWebServer server(80);

// --- HTML for the Firmware Update Page ---
const char* UPDATE_HTML = R"rawliteral(
<form method='POST' action='/updatefirmware' enctype='multipart/form-data' id='upload_form'><h2>Firmware Update</h2><p>Upload the new .bin file here. <strong>Warning:</strong> After the update, all settings will be reset to their default values.</p><input type='file' name='update' id='file' onchange='sub(this)' style='display:none'><label id='file-input' for='file'>Choose File...</label><input type='submit' class='btn' value='Start Update'><br><br><div id='prg'>Progress: 0%</div><br><div id='prgbar'><div id='bar'></div></div></form><script>function sub(obj){var fileName=obj.value.split('\\').pop();document.getElementById('file-input').innerHTML=fileName}
document.getElementById('upload_form').onsubmit=function(e){e.preventDefault();var form=document.getElementById('upload_form');var data=new FormData(form);var xhr=new XMLHttpRequest();xhr.open('POST','/updatefirmware',true);xhr.upload.onprogress=function(evt){if(evt.lengthComputable){var per=Math.round((evt.loaded/evt.total)*100);document.getElementById('prg').innerHTML='Progress: '+per+'%';document.getElementById('bar').style.width=per+'%'}};xhr.onload=function(){if(xhr.status===200){alert('Update successful! The device will restart with default settings.')}else{alert('Update failed! Status: '+xhr.status)}};xhr.send(data)};</script><style>body{background:#121212;font-family:sans-serif;font-size:14px;color:#e0e0e0}form{background:#1e1e1e;max-width:300px;margin:75px auto;padding:30px;border-radius:8px;text-align:center;border:1px solid #333}#file-input,.btn{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}.btn{background:#3498db;color:#fff;cursor:pointer;border:0;padding:0 15px}.btn:hover{background-color:#2980b9}#file-input{padding:0;border:1px solid #333;line-height:44px;text-align:left;display:block;cursor:pointer;padding-left:10px}#prgbar{background-color:#333;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px;border-radius:10px}</style>
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
        .header-buttons { display: flex; gap: 10px; }
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
        input, select, textarea { width: 100%; padding: 10px; margin: 5px 0; display: inline-block; border: 1px solid var(--border-color); border-radius: 4px; background-color: #333; color: var(--font-color); font-family: inherit;}
        textarea { resize: vertical; min-height: 80px;}
        form label.section-label { margin-top: 20px; display: block; font-weight: bold; font-size: 1.1em; color: var(--font-color); border-bottom: 1px solid var(--border-color); padding-bottom: 5px; margin-bottom: 15px; }
        form label { color: #ccc; }
        p.description { font-size: 0.8em; color: #888; margin-top: -2px; margin-bottom: 8px; }
        .form-group { margin-bottom: 15px; }
        .form-row { display: flex; gap: 10px; }
        .form-row .form-group { flex: 1; }
        hr { border-color: var(--border-color); margin: 20px 0; }
        .version-info { text-align: center; margin-top: 15px; color: #888; }
        .server-details-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px;}
    </style>
</head>
<body>
    <div class="container">
        <header class="main-header">
            <h1>Server Status Monitor</h1>
            <div class="header-buttons">
                <button id="logBtn" style="background-color: var(--color-orange);">Logs</button>
                <button id="settingsBtn">Settings</button>
            </div>
        </header>

        <main>
            <div class="tab-buttons" id="server-tabs"></div>
            <div id="server-content"></div>
        </main>
    </div>

    <div id="settingsModal" class="modal">
      <div class="modal-content">
        <div class="modal-header">
            <h2>Settings</h2>
            <span class="close" id="closeSettingsBtn">&times;</span>
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
            <div class="modal-footer">
                <button type="submit">Save & Restart</button>
            </div>
        </form>
        <div class="modal-body">
             <hr>
            <a href="/update" class="button-link" style="width: 100%; background-color: #e67e22;">Firmware Update</a>
            <p class="version-info" id="firmware_version">Current Version: ...</p>
        </div>
      </div>
    </div>

    <div id="logModal" class="modal">
        <div class="modal-content">
            <div class="modal-header">
                <h2>Serial Log</h2>
                <span class="close" id="closeLogBtn">&times;</span>
            </div>
            <div class="modal-body">
                <pre id="log-content" style="white-space: pre-wrap; word-wrap: break-word; background-color: #111; padding: 10px; border-radius: 5px; color: #e0e0e0; font-family: monospace; max-height: 60vh; overflow-y: auto;"></pre>
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
            let logRefreshIntervalId; 

            const settingsModal = document.getElementById('settingsModal');
            const settingsBtn = document.getElementById('settingsBtn');
            const closeSettingsBtn = document.getElementById('closeSettingsBtn');
            const settingsForm = document.getElementById('settingsForm');

            const logModal = document.getElementById('logModal');
            const logBtn = document.getElementById('logBtn');
            const closeLogBtn = document.getElementById('closeLogBtn');
            const logContent = document.getElementById('log-content');

            for(let i = 0; i < 3; i++) {
                const serverTab = document.createElement('div');
                serverTab.id = `server_${i}`;
                serverTab.className = 'modal-body tab-content';
                serverTab.innerHTML = `
                    <div class="form-group"><label class="section-label">Server ${i + 1} Settings</label><label for="server_name_${i}">Server Name</label><input type="text" id="server_name_${i}" name="server_name_${i}"></div>
                    <div class="form-group"><label for="weburl_${i}">URL to Monitor</label><input type="text" id="weburl_${i}" name="weburl_${i}"></div>
                    <div class="form-row">
                        <div class="form-group"><label for="check_interval_${i}">Check Interval (s)</label><input type="number" id="check_interval_${i}" name="check_interval_${i}" min="5"></div>
                        <div class="form-group"><label for="failure_threshold_${i}">Failures for Alert</label><input type="number" id="failure_threshold_${i}" name="failure_threshold_${i}" min="1"></div>
                        <div class="form-group"><label for="recovery_threshold_${i}">Successes for Recovery</label><input type="number" id="recovery_threshold_${i}" name="recovery_threshold_${i}" min="1"></div>
                    </div>
                    <hr>
                    <label class="section-label">Notifications</label>
                    <div class="form-group"><label for="online_message_${i}">Online Message</label><p class="description">Placeholders: {NAME}, {URL}</p><textarea id="online_message_${i}" name="online_message_${i}"></textarea></div>
                    <div class="form-group"><label for="offline_message_${i}">Offline Message</label><p class="description">Placeholders: {NAME}, {URL}, {CODE}</p><textarea id="offline_message_${i}" name="offline_message_${i}"></textarea></div>
                    <div class="form-group"><label for="discord_webhook_${i}">Discord</label><p class="description">Webhook URL. '0' = disable.</p><input type="text" id="discord_webhook_${i}" name="discord_webhook_${i}"></div>
                    <div class="form-group"><label for="ntfy_url_${i}">Ntfy</label><p class="description">Topic URL. '0' = disable.</p><input type="text" id="ntfy_url_${i}" name="ntfy_url_${i}"></div>
                    <div class="form-group"><label for="ntfy_priority_${i}">Ntfy Priority on Failure</label><select id="ntfy_priority_${i}" name="ntfy_priority_${i}"><option value="default">Default</option><option value="min">Minimal</option><option value="high">High</option><option value="max">Maximum</option></select></div>
                    <div class="form-group"><label for="telegram_bot_token_${i}">Telegram</label><p class="description">Bot Token. '0' = disable.</p><input type="text" id="telegram_bot_token_${i}" name="telegram_bot_token_${i}"></div>
                    <div class="form-row">
                        <div class="form-group"><label for="telegram_chat_id_1_${i}">Chat ID 1</label><input type="text" id="telegram_chat_id_1_${i}" name="telegram_chat_id_1_${i}"></div>
                        <div class="form-group"><label for="telegram_chat_id_2_${i}">Chat ID 2</label><input type="text" id="telegram_chat_id_2_${i}" name="telegram_chat_id_2_${i}"></div>
                        <div class="form-group"><label for="telegram_chat_id_3_${i}">Chat ID 3</label><input type="text" id="telegram_chat_id_3_${i}" name="telegram_chat_id_3_${i}"></div>
                    </div>
                    <hr>
                    <label class="section-label">Custom Actions (HTTP GET)</label>
                    <div class="form-group"><label for="http_get_url_on_${i}">On 'Server Online'</label><p class="description">'0' = disable.</p><input type="text" id="http_get_url_on_${i}" name="http_get_url_on_${i}"></div>
                    <div class="form-group"><label for="http_get_url_off_${i}">On 'Server Offline'</label><p class="description">'0' = disable.</p><input type="text" id="http_get_url_off_${i}" name="http_get_url_off_${i}"></div>`;
                settingsForm.insertBefore(serverTab, settingsForm.querySelector('.modal-footer'));
            }

            function startAutoRefresh() { clearInterval(refreshIntervalId); fetchData(); refreshIntervalId = setInterval(fetchData, 5000); }
            function stopAutoRefresh() { clearInterval(refreshIntervalId); }

            settingsBtn.onclick = () => { stopAutoRefresh(); settingsModal.style.display = 'block'; }
            closeSettingsBtn.onclick = () => { settingsModal.style.display = 'none'; startAutoRefresh(); }
            
            async function fetchLogs() {
                try {
                    const response = await fetch('/api/logs');
                    if (!response.ok) throw new Error('Log fetch failed');
                    const logText = await response.text();
                    logContent.textContent = logText;
                    logContent.scrollTop = logContent.scrollHeight;
                } catch (error) {
                    console.error('Error fetching logs:', error);
                    logContent.textContent = 'Error loading logs.';
                }
            }
            
            logBtn.onclick = () => {
                stopAutoRefresh();
                logModal.style.display = 'block';
                fetchLogs();
                logRefreshIntervalId = setInterval(fetchLogs, 2000);
            };

            const closeLogModal = () => {
                logModal.style.display = 'none';
                clearInterval(logRefreshIntervalId);
                startAutoRefresh();
            };
            closeLogBtn.onclick = closeLogModal;

            window.onclick = (event) => {
                if (event.target == settingsModal) {
                    settingsModal.style.display = 'none';
                    startAutoRefresh();
                }
                if (event.target == logModal) {
                    closeLogModal();
                }
            }

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
                
                if(serverTabs.childElementCount === 0) {
                    data.targets.forEach((target, i) => {
                        const tabButton = document.createElement('button');
                        tabButton.className = `tab-link ${i===0 ? 'active':''}`;
                        tabButton.onclick = (event) => openServerTab(event, `server_content_${i}`);
                        tabButton.id = `main_tab_btn_${i}`;
                        serverTabs.appendChild(tabButton);

                        const contentDiv = document.createElement('div');
                        contentDiv.id = `server_content_${i}`;
                        contentDiv.className = `server-tab-content ${i===0 ? 'active':''}`;
                        serverContent.appendChild(contentDiv);
                        
                        document.querySelector(`#settings-tabs button[onclick*="server_${i}"]`).textContent = `Server ${i+1}`;
                    });
                }
                
                data.targets.forEach((target, i) => {
                    const serverName = target.config.server_name || `Server ${i+1}`;
                    document.getElementById(`main_tab_btn_${i}`).textContent = serverName;
                    document.querySelector(`#settings-tabs button[onclick*="server_${i}"]`).textContent = serverName;

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
                        <div class="card" style="text-align: center; padding: 15px;">
                            <a href="https://github.com/PearXP/esp32-web-server-status" target="_blank" style="color: var(--color-blue); text-decoration: none; font-size: 1.1em;">
                                View Project on GitHub 🚀
                            </a>
                        </div>
                    `;

                    const timeline = document.getElementById(`timeline_${i}`);
                    const logEntries = target.log.split('\n').filter(e => e.trim() !== '');
                    if (logEntries.length === 0) {
                        timeline.innerHTML = '<p>No log entries yet.</p>';
                    } else {
                        timeline.innerHTML = '';
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

                    document.getElementById(`server_name_${i}`).value = target.config.server_name;
                    document.getElementById(`weburl_${i}`).value = target.config.weburl;
                    document.getElementById(`check_interval_${i}`).value = target.config.check_interval_seconds;
                    document.getElementById(`failure_threshold_${i}`).value = target.config.failure_threshold;
                    document.getElementById(`recovery_threshold_${i}`).value = target.config.recovery_threshold;
                    document.getElementById(`online_message_${i}`).value = target.config.online_message;
                    document.getElementById(`offline_message_${i}`).value = target.config.offline_message;
                    document.getElementById(`discord_webhook_${i}`).value = target.config.discord_webhook;
                    document.getElementById(`ntfy_url_${i}`).value = target.config.ntfy_url;
                    document.getElementById(`ntfy_priority_${i}`).value = target.config.ntfy_priority;
                    document.getElementById(`telegram_bot_token_${i}`).value = target.config.telegram_bot_token;
                    document.getElementById(`telegram_chat_id_1_${i}`).value = target.config.telegram_chat_id_1;
                    document.getElementById(`telegram_chat_id_2_${i}`).value = target.config.telegram_chat_id_2;
                    document.getElementById(`telegram_chat_id_3_${i}`).value = target.config.telegram_chat_id_3;
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
                        settingsModal.style.display = 'none';
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

void getFormattedTime(char* buffer, size_t bufferSize) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        strncpy(buffer, "Time not set", bufferSize -1);
        buffer[bufferSize -1] = '\0';
        return;
    }
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

void web_log_printf(const char *format, ...) {
    char timeBuf[30];
    getFormattedTime(timeBuf, sizeof(timeBuf));

    char logBuf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(logBuf, sizeof(logBuf), format, args);
    va_end(args);

    Serial.printf("[%s] %s\n", timeBuf, logBuf);

    int len = snprintf(NULL, 0, "[%s] %s\n", timeBuf, logBuf);
    if (serialLogBufferPos + len >= SERIAL_LOG_SIZE) {
        serialLogBufferPos = 0;
    }
    snprintf(serialLogBuffer + serialLogBufferPos, SERIAL_LOG_SIZE - serialLogBufferPos, "[%s] %s\n", timeBuf, logBuf);
    serialLogBufferPos += len;
}

void prependToLog(char* logBuffer, const char* newEntry, size_t bufferSize) {
    size_t entryLen = strlen(newEntry);
    if (entryLen >= bufferSize) return;

    size_t currentLen = strlen(logBuffer);
    
    if (currentLen + entryLen >= bufferSize) {
        currentLen = bufferSize - entryLen - 1;
    }

    memmove(logBuffer + entryLen, logBuffer, currentLen);
    memcpy(logBuffer, newEntry, entryLen);
    logBuffer[currentLen + entryLen] = '\0';
}

void safeStrcpy(char* dest, const char* src, size_t size) {
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

void urlEncode(char* dst, const char* src, size_t dstSize) {
    char c, hex_buf[4];
    size_t written = 0;
    while (*src && written + 4 < dstSize) {
        c = *src++;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[written++] = c;
        } else {
            sprintf(hex_buf, "%%%02X", c);
            dst[written++] = hex_buf[0];
            dst[written++] = hex_buf[1];
            dst[written++] = hex_buf[2];
        }
    }
    dst[written] = '\0';
}

void resetToDefault() {
    Serial.println("Resetting to default values...");
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
    EEPROM.put(CONFIG_VERSION_ADDRESS, CONFIG_VERSION);
    EEPROM.commit();
    EEPROM.end();
    Serial.println("EEPROM cleared. Defaults will be loaded on next boot.");
}

void loadConfig() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, ssid);
    EEPROM.get(32, password);
    EEPROM.get(64, gmt_offset);
    int address = 100;
    for (int i = 0; i < NUM_TARGETS; i++) {
        EEPROM.get(address, targets[i]);
        address += sizeof(TargetConfig);
    }
    EEPROM.end();

    if (strlen(ssid) == 0) {
        strcpy(ssid, default_ssid);
        strcpy(password, default_password);
        gmt_offset = default_gmt_offset;
        for (int i = 0; i < NUM_TARGETS; i++) {
            char name[10];
            snprintf(name, sizeof(name), "Server %d", i + 1);
            safeStrcpy(targets[i].server_name, name, sizeof(targets[i].server_name));
            strcpy(targets[i].weburl, (i == 0) ? "http://localhost" : "0");
            strcpy(targets[i].discord_webhook_url, "0");
            strcpy(targets[i].ntfy_url, "0");
            strcpy(targets[i].ntfy_priority, "default");
            strcpy(targets[i].telegram_bot_token, "0");
            strcpy(targets[i].telegram_chat_id_1, "0");
            strcpy(targets[i].telegram_chat_id_2, "0");
            strcpy(targets[i].telegram_chat_id_3, "0");
            strcpy(targets[i].http_get_url_on, "0");
            strcpy(targets[i].http_get_url_off, "0");
            safeStrcpy(targets[i].online_message, "✅ {NAME} is back online: {URL}", sizeof(targets[i].online_message));
            safeStrcpy(targets[i].offline_message, "🚨 {NAME} OUTAGE: {URL} (Code: {CODE})", sizeof(targets[i].offline_message));
            targets[i].check_interval_seconds = 20;
            targets[i].failure_threshold = 3;
            targets[i].recovery_threshold = 2;
        }
    }
    gmtOffset_sec = gmt_offset * 3600;
}

void saveConfig() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(0, ssid);
    EEPROM.put(32, password);
    EEPROM.put(64, gmt_offset);
    int address = 100;
    for (int i = 0; i < NUM_TARGETS; i++) {
        EEPROM.put(address, targets[i]);
        address += sizeof(TargetConfig);
    }
    EEPROM.put(CONFIG_VERSION_ADDRESS, CONFIG_VERSION);
    EEPROM.commit();
    EEPROM.end();
}

void updatePingStats(int index) {
    if (pingTime[index] < minpingTime[index] || minpingTime[index] == 0) minpingTime[index] = pingTime[index];
    if (pingTime[index] > maxpingTime[index]) maxpingTime[index] = pingTime[index];
}

void sendNotifications(int index, const char* msg) {
    TargetConfig target = targets[index];
    HTTPClient http;

    if (strcmp(target.discord_webhook_url, "0") != 0 && strlen(target.discord_webhook_url) > 10) {
        http.begin(target.discord_webhook_url);
        http.addHeader("Content-Type", "application/json");
        char payload[256];
        snprintf(payload, sizeof(payload), "{\"content\":\"%s\"}", msg);
        http.POST(payload);
        http.end();
    }
    if (strcmp(target.ntfy_url, "0") != 0 && strlen(target.ntfy_url) > 10) {
        http.begin(target.ntfy_url);
        http.addHeader("Content-Type", "text/plain");
        http.addHeader("Priority", target.ntfy_priority);
        http.POST(msg);
        http.end();
    }
    if (strlen(target.telegram_bot_token) > 10) {
        char encodedMsg[512];
        urlEncode(encodedMsg, msg, sizeof(encodedMsg));
        
        char url[512];
        const char* chat_ids[] = {target.telegram_chat_id_1, target.telegram_chat_id_2, target.telegram_chat_id_3};
        for (int j = 0; j < 3; j++) {
            if (strcmp(chat_ids[j], "0") != 0 && strlen(chat_ids[j]) > 1) {
                snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=%s", target.telegram_bot_token, chat_ids[j], encodedMsg);
                http.begin(url);
                http.GET();
                http.end();
            }
        }
    }
}

void sendCustomHttpRequest(const char* url) {
    if (strcmp(url, "0") == 0 || strlen(url) < 10) return;
    HTTPClient http;
    http.begin(url);
    http.GET();
    http.end();
}

void manageWifiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiReconnectAttempt > wifiReconnectInterval) {
      lastWifiReconnectAttempt = millis();
      web_log_printf("WiFi connection lost. Attempting to reconnect...");
      WiFi.begin(ssid, password);
    }
  }
}

void setup() {
    Serial.begin(115200);
    web_log_printf("Booting device...");

    EEPROM.begin(EEPROM_SIZE);
    int storedVersion = 0;
    EEPROM.get(CONFIG_VERSION_ADDRESS, storedVersion);
    EEPROM.end();
    if (storedVersion != CONFIG_VERSION) {
        web_log_printf("Config version mismatch! Stored: %d, Firmware: %d. Resetting.", storedVersion, CONFIG_VERSION);
        resetToDefault();
        delay(1000);
        ESP.restart();
    }
    
    pinMode(0, INPUT_PULLUP);
    delay(100);
    if (digitalRead(0) == LOW) {
        web_log_printf("BOOT button pressed. Hold for 5 seconds for manual reset...");
        delay(5000);
        if (digitalRead(0) == LOW) {
            resetToDefault();
            web_log_printf("Device will restart with default values...");
            delay(1000);
            ESP.restart();
        }
    }

    loadConfig();
    
    WiFi.begin(ssid, password);
    web_log_printf("Connecting to WiFi: %s", ssid);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        web_log_printf("WiFi connected!");
        web_log_printf("IP Address: %s", WiFi.localIP().toString().c_str());
        configTime(gmtOffset_sec, 0, ntpServer);
    } else {
        web_log_printf("WiFi connection failed. Will keep trying in the background.");
    }
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", INDEX_HTML);
    });

    server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", serialLogBuffer);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncJsonResponse * response = new AsyncJsonResponse();
        JsonObject root = response->getRoot();

        root["firmware_version"] = CONFIG_VERSION;
        JsonObject general_config = root.createNestedObject("general_config");
        general_config["ssid"] = ssid;
        general_config["gmt_offset"] = gmt_offset;

        JsonArray targets_json = root.createNestedArray("targets");
        for (int i = 0; i < NUM_TARGETS; i++) {
            JsonObject target_obj = targets_json.createNestedObject();
            target_obj["http_code"] = httpCode[i];
            target_obj["log"] = targetLogMessages[i];
            JsonObject ping = target_obj.createNestedObject("ping");
            ping["last"] = pingTime[i];
            ping["min"] = minpingTime[i];
            ping["max"] = maxpingTime[i];
            
            JsonObject config = target_obj.createNestedObject("config");
            config["server_name"] = targets[i].server_name;
            config["weburl"] = targets[i].weburl;
            config["discord_webhook"] = targets[i].discord_webhook_url;
            config["ntfy_url"] = targets[i].ntfy_url;
            config["ntfy_priority"] = targets[i].ntfy_priority;
            config["telegram_bot_token"] = targets[i].telegram_bot_token;
            config["telegram_chat_id_1"] = targets[i].telegram_chat_id_1;
            config["telegram_chat_id_2"] = targets[i].telegram_chat_id_2;
            config["telegram_chat_id_3"] = targets[i].telegram_chat_id_3;
            config["http_get_url_on"] = targets[i].http_get_url_on;
            config["http_get_url_off"] = targets[i].http_get_url_off;
            config["online_message"] = targets[i].online_message;
            config["offline_message"] = targets[i].offline_message;
            config["check_interval_seconds"] = targets[i].check_interval_seconds;
            config["failure_threshold"] = targets[i].failure_threshold;
            config["recovery_threshold"] = targets[i].recovery_threshold;
        }
        
        response->setLength();
        request->send(response);
    });
    
    server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request) {
        for (int i = 0; i < request->params(); i++) {
            const AsyncWebParameter* p = request->getParam(i);
            if (!p->isPost()) continue;
            
            const char* paramName = p->name().c_str();
            const char* paramValue = p->value().c_str();

            if (strcmp(paramName, "ssid") == 0) safeStrcpy(ssid, paramValue, sizeof(ssid));
            else if (strcmp(paramName, "password") == 0 && strlen(paramValue) > 0) safeStrcpy(password, paramValue, sizeof(password));
            else if (strcmp(paramName, "gmt_offset") == 0) gmt_offset = atoi(paramValue);
            else {
                const char* lastUnderscore = strrchr(paramName, '_');
                if (lastUnderscore == NULL) continue;
                
                int serverIndex = atoi(lastUnderscore + 1);
                if (serverIndex >= 0 && serverIndex < NUM_TARGETS) {
                    char settingNameBuf[64];
                    strncpy(settingNameBuf, paramName, lastUnderscore - paramName);
                    settingNameBuf[lastUnderscore - paramName] = '\0';
                    
                    if (strstr(settingNameBuf, "telegram_chat_id")) strcpy(settingNameBuf, "telegram_chat_id");

                    if (strcmp(settingNameBuf, "server_name") == 0) safeStrcpy(targets[serverIndex].server_name, paramValue, sizeof(targets[serverIndex].server_name));
                    else if (strcmp(settingNameBuf, "weburl") == 0) safeStrcpy(targets[serverIndex].weburl, paramValue, sizeof(targets[serverIndex].weburl));
                    else if (strcmp(settingNameBuf, "check_interval") == 0) targets[serverIndex].check_interval_seconds = atoi(paramValue);
                    else if (strcmp(settingNameBuf, "failure_threshold") == 0) targets[serverIndex].failure_threshold = atoi(paramValue);
                    else if (strcmp(settingNameBuf, "recovery_threshold") == 0) targets[serverIndex].recovery_threshold = atoi(paramValue);
                    else if (strcmp(settingNameBuf, "online_message") == 0) safeStrcpy(targets[serverIndex].online_message, paramValue, sizeof(targets[serverIndex].online_message));
                    else if (strcmp(settingNameBuf, "offline_message") == 0) safeStrcpy(targets[serverIndex].offline_message, paramValue, sizeof(targets[serverIndex].offline_message));
                    else if (strcmp(settingNameBuf, "discord_webhook") == 0) safeStrcpy(targets[serverIndex].discord_webhook_url, paramValue, sizeof(targets[serverIndex].discord_webhook_url));
                    else if (strcmp(settingNameBuf, "ntfy_url") == 0) safeStrcpy(targets[serverIndex].ntfy_url, paramValue, sizeof(targets[serverIndex].ntfy_url));
                    else if (strcmp(settingNameBuf, "ntfy_priority") == 0) safeStrcpy(targets[serverIndex].ntfy_priority, paramValue, sizeof(targets[serverIndex].ntfy_priority));
                    else if (strcmp(settingNameBuf, "telegram_bot_token") == 0) safeStrcpy(targets[serverIndex].telegram_bot_token, paramValue, sizeof(targets[serverIndex].telegram_bot_token));
                    else if (strcmp(settingNameBuf, "telegram_chat_id") == 0) {
                        char idNum = paramName[lastUnderscore - paramName - 1];
                        if (idNum == '1') safeStrcpy(targets[serverIndex].telegram_chat_id_1, paramValue, sizeof(targets[serverIndex].telegram_chat_id_1));
                        else if (idNum == '2') safeStrcpy(targets[serverIndex].telegram_chat_id_2, paramValue, sizeof(targets[serverIndex].telegram_chat_id_2));
                        else if (idNum == '3') safeStrcpy(targets[serverIndex].telegram_chat_id_3, paramValue, sizeof(targets[serverIndex].telegram_chat_id_3));
                    }
                    else if (strcmp(settingNameBuf, "http_get_url_on") == 0) safeStrcpy(targets[serverIndex].http_get_url_on, paramValue, sizeof(targets[serverIndex].http_get_url_on));
                    else if (strcmp(settingNameBuf, "http_get_url_off") == 0) safeStrcpy(targets[serverIndex].http_get_url_off, paramValue, sizeof(targets[serverIndex].http_get_url_off));
                }
            }
        }
        saveConfig();
        request->send(200, "text/plain", "OK");
        delay(1000);
        ESP.restart();
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", UPDATE_HTML);
    });
    
    server.on("/updatefirmware", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        }, 
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                web_log_printf("Update Start: %s", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
            }
            if (len) {
                if (Update.write(data, len) != len) Update.printError(Serial);
            }
            if (final) {
                if (Update.end(true)) {
                    web_log_printf("Update Success: %u bytes", index + len);
                    resetToDefault();
                    delay(1000);
                    ESP.restart();
                } else {
                    Update.printError(Serial);
                }
            }
        });

    server.begin();
}


void loop() {
    manageWifiConnection();

    if (WiFi.status() == WL_CONNECTED) {
        for (int i = 0; i < NUM_TARGETS; i++) {
            if (strcmp(targets[i].weburl, "0") == 0 || strlen(targets[i].weburl) < 10) {
                httpCode[i] = 0;
                continue;
            }

            if (millis() - last_check_time[i] < (targets[i].check_interval_seconds * 1000)) {
                continue;
            }
            last_check_time[i] = millis();

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

            if (isOnline) {
                failure_count[i] = 0;
                success_count[i]++;
                if (confirmed_online_state[i] == false && success_count[i] >= targets[i].recovery_threshold) {
                    confirmed_online_state[i] = true;

                    String message = targets[i].online_message;
                    message.replace("{NAME}", targets[i].server_name);
                    message.replace("{URL}", targets[i].weburl);

                    char timeBuf[30], logEntry[64];
                    getFormattedTime(timeBuf, sizeof(timeBuf));
                    snprintf(logEntry, sizeof(logEntry), "on;%s\n", timeBuf);

                    sendNotifications(i, message.c_str());
                    sendCustomHttpRequest(targets[i].http_get_url_on);
                    prependToLog(targetLogMessages[i], logEntry, TARGET_LOG_SIZE);
                }
            } else {
                success_count[i] = 0;
                failure_count[i]++;
                if (confirmed_online_state[i] == true && failure_count[i] >= targets[i].failure_threshold) {
                    confirmed_online_state[i] = false;
                    
                    String message = targets[i].offline_message;
                    message.replace("{NAME}", targets[i].server_name);
                    message.replace("{URL}", targets[i].weburl);
                    message.replace("{CODE}", String(httpCode[i]));

                    char timeBuf[30], logEntry[64];
                    getFormattedTime(timeBuf, sizeof(timeBuf));
                    snprintf(logEntry, sizeof(logEntry), "off;%s\n", timeBuf);

                    sendNotifications(i, message.c_str());
                    sendCustomHttpRequest(targets[i].http_get_url_off);
                    prependToLog(targetLogMessages[i], logEntry, TARGET_LOG_SIZE);
                }
            }
            web_log_printf("[Server %d] URL: %s, Status: %d, Ping: %lu ms, Fails: %d, Successes: %d",
                i + 1, targets[i].weburl, httpCode[i], pingTime[i], failure_count[i], success_count[i]);
        }
    }
    delay(100);
}
