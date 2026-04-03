#pragma once
const char INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Monitor</title>
<style>
:root{--bg:#090910;--c1:#0f1018;--c2:#161824;--c3:#1d2030;--bd:#252840;--tx:#eaeaf6;--mu:#5858a0;--gr:#22c55e;--re:#ef4444;--bl:#4f8ef7;--or:#f59e0b;--pu:#a855f7;--rr:12px;--sh:0 4px 24px rgba(0,0,0,.4)}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--tx);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;min-height:100vh;font-size:14px}
.wrap{max-width:900px;margin:0 auto;padding:14px 16px}

/* ── Header ── */
.hdr{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px;gap:8px}
.hdr-brand{display:flex;align-items:center;gap:10px}
.hdr-logo{width:32px;height:32px;background:linear-gradient(135deg,var(--bl),var(--pu));border-radius:8px;display:flex;align-items:center;justify-content:center;flex-shrink:0}
.hdr-title{font-size:1.1rem;font-weight:800;letter-spacing:-.4px}
.hdr-sub{font-size:11px;color:var(--mu);margin-top:1px}
.hdr-btns{display:flex;gap:8px;flex-shrink:0}

/* ── Buttons ── */
.btn{background:var(--c2);border:1px solid var(--bd);color:var(--tx);padding:7px 14px;border-radius:9px;cursor:pointer;font-size:13px;font-weight:500;transition:all .15s;white-space:nowrap;display:inline-flex;align-items:center;gap:6px}
.btn:hover{background:var(--c3);border-color:#363860}
.btn-p{background:linear-gradient(135deg,#3b7ef0,#6b4ef8);border-color:transparent;color:#fff}
.btn-p:hover{background:linear-gradient(135deg,#2563eb,#5b3de8);border-color:transparent}
.btn-sm{padding:5px 11px;font-size:12px;border-radius:7px}
.btn-ghost{background:transparent;border-color:transparent;color:var(--mu)}
.btn-ghost:hover{background:var(--c2);color:var(--tx)}

/* ── Sysbar ── */
.sysbar{display:flex;gap:0;background:var(--c1);border:1px solid var(--bd);border-radius:var(--rr);margin-bottom:16px;overflow:hidden}
.syschip{display:flex;align-items:center;gap:7px;padding:10px 16px;font-size:12px;color:var(--mu);border-right:1px solid var(--bd);flex:1;min-width:0}
.syschip:last-child{border-right:none}
.syschip b{color:var(--tx);font-weight:600;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.syschip svg{flex-shrink:0;opacity:.6}
.wled{width:7px;height:7px;border-radius:50%;background:var(--mu);flex-shrink:0;transition:background .5s,box-shadow .5s}
.wled.on{background:var(--gr);box-shadow:0 0 6px var(--gr)}

/* ── Server Tabs — compact scroll row ── */
.tabs-wrap{background:var(--c1);border:1px solid var(--bd);border-radius:var(--rr);margin-bottom:16px;padding:8px 10px;overflow-x:auto}
.tabs{display:flex;gap:5px;flex-wrap:nowrap;min-width:max-content}
.tab{padding:6px 13px;border-radius:8px;border:1px solid var(--bd);background:var(--c2);color:var(--mu);cursor:pointer;font-size:12px;font-weight:600;transition:all .15s;position:relative;display:flex;align-items:center;gap:6px;white-space:nowrap}
.tab.on{background:linear-gradient(135deg,#1a2a50,#1a1a3a);border-color:#4060c0;color:var(--tx)}
.tab-dot{width:7px;height:7px;border-radius:50%;background:var(--mu);flex-shrink:0;transition:background .4s,box-shadow .4s}
.tab-dot.gr{background:var(--gr);box-shadow:0 0 6px var(--gr)}
.tab-dot.re{background:var(--re);box-shadow:0 0 6px var(--re)}

/* ── Cards ── */
.card{background:var(--c1);border:1px solid var(--bd);border-radius:var(--rr);padding:20px;margin-bottom:14px;box-shadow:var(--sh)}
.card-sm{background:var(--c2);border:1px solid var(--bd);border-radius:10px;padding:16px}

/* ── Status row ── */
.status-row{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:20px}
.status-left{display:flex;align-items:center;gap:14px}
.status-ind{width:16px;height:16px;border-radius:50%;flex-shrink:0;transition:background .4s,box-shadow .4s}
.status-ind.on{background:var(--gr);box-shadow:0 0 14px var(--gr)}
.status-ind.off{background:var(--re);box-shadow:0 0 14px var(--re)}
.status-ind.dis{background:var(--mu)}
.status-name{font-size:1.15rem;font-weight:800}
.status-url{font-size:12px;color:var(--mu);margin-top:2px;word-break:break-all}
.status-badge{display:inline-flex;align-items:center;gap:5px;padding:3px 10px;border-radius:20px;font-size:11px;font-weight:700;text-transform:uppercase;letter-spacing:.5px}
.badge-on{background:rgba(34,197,94,.15);color:var(--gr);border:1px solid rgba(34,197,94,.3)}
.badge-off{background:rgba(239,68,68,.15);color:var(--re);border:1px solid rgba(239,68,68,.3)}

/* ── Stats grid ── */
.stats-grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px;margin-bottom:14px}
.stat-card{background:var(--c2);border:1px solid var(--bd);border-radius:10px;padding:16px;text-align:center;position:relative;overflow:hidden}
.stat-card::before{content:'';position:absolute;inset:0;background:linear-gradient(135deg,rgba(79,142,247,.03),transparent)}
.stat-val{font-size:1.5rem;font-weight:800;letter-spacing:-.5px;line-height:1}
.stat-lbl{font-size:10px;color:var(--mu);margin-top:5px;text-transform:uppercase;letter-spacing:.6px;font-weight:600}
.stat-sub{font-size:11px;color:var(--mu);margin-top:3px}

/* ── Uptime ring ── */
.ring-wrap{display:flex;justify-content:center;align-items:center;padding:4px 0}

/* ── Ping gauge ── */
.gauge-wrap{margin-top:10px}
.gauge-label{display:flex;justify-content:space-between;font-size:11px;color:var(--mu);margin-bottom:4px}
.gauge-track{background:var(--c3);border-radius:4px;height:5px;overflow:hidden}
.gauge-fill{height:5px;border-radius:4px;transition:width .6s ease,background .6s ease}

/* ── Sparkline card ── */
.spark-card{background:var(--c2);border:1px solid var(--bd);border-radius:10px;padding:16px;margin-bottom:14px}
.spark-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px}
.spark-title{font-size:11px;font-weight:700;color:var(--mu);text-transform:uppercase;letter-spacing:.6px}
.spark-stats{display:flex;gap:16px}
.spark-stat{font-size:11px;color:var(--mu)}
.spark-stat b{color:var(--tx);font-weight:600}
.spark-svg{display:block;width:100%}

/* ── Disabled notice ── */
.disabled-card{text-align:center;padding:60px 20px;color:var(--mu)}
.disabled-icon{font-size:2.5rem;margin-bottom:12px;opacity:.4}
.disabled-card h3{font-size:15px;font-weight:700;color:var(--tx);margin-bottom:6px}
.disabled-card p{font-size:13px}

/* ── Modal ── */
.modal{display:none;position:fixed;inset:0;background:rgba(0,0,0,.7);z-index:100;align-items:center;justify-content:center;padding:16px;backdrop-filter:blur(4px)}
.modal.show{display:flex}
.mbox{background:var(--c1);border:1px solid var(--bd);border-radius:16px;width:100%;max-width:600px;max-height:92vh;display:flex;flex-direction:column;overflow:hidden;box-shadow:0 20px 80px rgba(0,0,0,.6)}
.mhd{padding:18px 22px;border-bottom:1px solid var(--bd);display:flex;align-items:center;justify-content:space-between;flex-shrink:0}
.mhd h2{font-size:1rem;font-weight:800}
.mcl{background:none;border:none;color:var(--mu);cursor:pointer;font-size:1.5rem;line-height:1;padding:0;width:28px;height:28px;display:flex;align-items:center;justify-content:center;border-radius:6px}
.mcl:hover{background:var(--c2);color:var(--tx)}
.mbody{overflow-y:auto;flex:1;padding:20px 22px}
.mfoot{padding:16px 22px;border-top:1px solid var(--bd);display:flex;justify-content:flex-end;gap:8px;flex-shrink:0}

/* ── Settings tabs ── */
.stabs-wrap{background:var(--c3);border:1px solid var(--bd);border-radius:10px;padding:6px;margin-bottom:18px;overflow-x:auto}
.stabs{display:flex;gap:4px;flex-wrap:nowrap;min-width:max-content}
.stab{padding:5px 12px;border-radius:7px;border:none;background:transparent;color:var(--mu);cursor:pointer;font-size:12px;font-weight:600;white-space:nowrap;transition:all .15s}
.stab.on{background:var(--bl);color:#fff}
.spane{display:none}
.spane.on{display:block}

/* ── Section header ── */
.sec{font-size:11px;font-weight:700;color:var(--mu);text-transform:uppercase;letter-spacing:.6px;margin:0 0 14px;padding-bottom:10px;border-bottom:1px solid var(--bd);display:flex;align-items:center;gap:8px}
.sec-mt{margin-top:20px}

/* ── Form ── */
.fg{margin-bottom:14px}
.fg label{display:block;font-size:12px;color:var(--mu);margin-bottom:6px;font-weight:600}
.fg input,.fg textarea,.fg select{width:100%;background:var(--c2);border:1px solid var(--bd);color:var(--tx);padding:9px 12px;border-radius:8px;font-size:13px;outline:none;transition:border-color .15s}
.fg input:focus,.fg textarea:focus,.fg select:focus{border-color:var(--bl);background:var(--c3)}
.fg textarea{min-height:64px;resize:vertical;font-family:inherit}
.fg select{cursor:pointer}
.fg .hint{font-size:11px;color:var(--mu);margin-top:4px}
.frow2{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.frow3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}
.input-wrap{position:relative}
.input-wrap input{padding-right:40px}
.input-btn{position:absolute;right:8px;top:50%;transform:translateY(-50%);background:none;border:none;color:var(--mu);cursor:pointer;padding:4px;border-radius:4px;display:flex;align-items:center}
.input-btn:hover{color:var(--tx);background:var(--bd)}

/* ── Notif block ── */
.notif-block{background:var(--c3);border:1px solid var(--bd);border-radius:10px;padding:14px;margin-bottom:12px}
.notif-hd{display:flex;align-items:center;gap:8px;margin-bottom:14px;font-size:12px;font-weight:700;color:var(--tx)}

/* ── WiFi scan list ── */
.scan-list{background:var(--c3);border:1px solid var(--bd);border-radius:8px;margin-top:6px;max-height:160px;overflow-y:auto;display:none}
.scan-item{padding:8px 12px;cursor:pointer;font-size:13px;display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--bd)}
.scan-item:last-child{border-bottom:none}
.scan-item:hover{background:var(--bd)}
.scan-rssi{font-size:11px;color:var(--mu)}

/* ── Ping bar strip ── */
.strip-wrap{display:flex;gap:2px;height:20px;align-items:flex-end;margin-top:6px}
.strip-bar{flex:1;border-radius:2px;min-height:4px;transition:height .3s}

/* ── Events ── */
.ev-card{background:var(--c2);border:1px solid var(--bd);border-radius:10px;padding:16px;margin-bottom:14px}
.ev-title{font-size:11px;font-weight:700;color:var(--mu);text-transform:uppercase;letter-spacing:.6px;margin-bottom:12px}
.ev-row{display:flex;align-items:center;gap:10px;padding:7px 0;border-bottom:1px solid rgba(37,40,64,.8)}
.ev-row:last-child{border-bottom:none}
.ev-dot{width:8px;height:8px;border-radius:50%;flex-shrink:0}
.ev-dot-on{background:var(--gr)}
.ev-dot-off{background:var(--re)}
.ev-text{font-family:monospace;font-size:12px;color:var(--tx);flex:1}
.ev-time{font-size:11px;color:var(--mu);white-space:nowrap}
.ev-empty{color:var(--mu);font-size:13px;padding:8px 0}

/* ── Port scanner ── */
.ps-disclaimer{background:rgba(245,158,11,.08);border:1px solid rgba(245,158,11,.3);border-radius:10px;padding:14px;margin-bottom:16px;font-size:13px;line-height:1.6;color:var(--tx)}
.ps-disclaimer b{color:var(--or)}
.ps-check{display:flex;align-items:flex-start;gap:10px;margin-top:10px;cursor:pointer;font-size:13px}
.ps-check input{margin-top:2px;cursor:pointer;accent-color:var(--or)}
.ps-results{background:var(--c3);border:1px solid var(--bd);border-radius:8px;padding:14px;margin-top:14px;font-family:monospace;font-size:12px;line-height:1.8;color:var(--tx);white-space:pre-wrap;min-height:60px;display:none}
.ps-progress{font-size:12px;color:var(--mu);margin-top:8px;display:none}
.footer{text-align:center;padding:24px 16px 20px;border-top:1px solid var(--bd);margin-top:8px}
.gh-btn{display:inline-flex;align-items:center;gap:10px;padding:10px 20px;background:var(--c2);border:1px solid var(--bd);border-radius:10px;color:var(--tx);text-decoration:none;font-size:13px;font-weight:600;transition:all .15s}
.gh-btn:hover{background:var(--c3);border-color:#363860;transform:translateY(-1px);box-shadow:0 4px 20px rgba(0,0,0,.3)}
.gh-btn svg{flex-shrink:0}
.footer-note{font-size:11px;color:var(--mu);margin-top:10px}

/* ── Misc ── */
.loading{text-align:center;padding:60px;color:var(--mu)}
@media(max-width:520px){
  .stats-grid{grid-template-columns:1fr 1fr}
  .frow3{grid-template-columns:1fr}
  .frow2{grid-template-columns:1fr}
  .hdr-btns .btn span{display:none}
  .syschip{padding:8px 10px}
}
</style>
</head>
<body>
<div class="wrap">

<!-- Header -->
<div class="hdr">
  <div class="hdr-brand">
    <div class="hdr-logo">
      <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.5" stroke-linecap="round">
        <circle cx="12" cy="12" r="3"/><path d="M12 2v3M12 19v3M4.22 4.22l2.12 2.12M17.66 17.66l2.12 2.12M2 12h3M19 12h3M4.22 19.78l2.12-2.12M17.66 6.34l2.12-2.12"/>
      </svg>
    </div>
    <div>
      <div class="hdr-title">ESP32 Monitor</div>
      <div class="hdr-sub">Uptime &amp; Status Dashboard</div>
    </div>
  </div>
  <div class="hdr-btns">
    <button class="btn btn-ghost" id="logsBtn">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14,2 14,8 20,8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg>
      <span>Logs</span>
    </button>
    <button class="btn btn-ghost" id="toolsBtn">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14M4.93 4.93a10 10 0 0 0 0 14.14"/></svg>
      <span>Tools</span>
    </button>
    <button class="btn btn-p" id="settingsBtn">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
      <span>Settings</span>
    </button>
  </div>
</div>

<!-- System bar -->
<div class="sysbar">
  <div class="syschip">
    <span class="wled" id="wled"></span>
    <span>WiFi</span><b id="s-rssi">--</b>
  </div>
  <div class="syschip">
    <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="7" width="20" height="14" rx="2"/><path d="M16 7V5a2 2 0 0 0-4 0v2"/></svg>
    <b id="s-heap">--</b>
  </div>
  <div class="syschip">
    <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12,6 12,12 16,14"/></svg>
    <b id="s-up">--</b>
  </div>
  <div class="syschip">
    <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="9" y="2" width="6" height="6"/><rect x="9" y="16" width="6" height="6"/><rect x="2" y="9" width="6" height="6"/><rect x="16" y="9" width="6" height="6"/></svg>
    <b id="s-ip">--</b>
  </div>
</div>

<!-- Server tabs — scrollable row -->
<div class="tabs-wrap">
  <div class="tabs" id="maintabs"></div>
</div>

<!-- Main content -->
<div id="maincontent"><div class="loading">Connecting to ESP32...</div></div>

<!-- Footer -->
<div class="footer">
  <a class="gh-btn" href="https://github.com/PearXP/ESP32-Uptime-Monitor" target="_blank" rel="noopener">
    <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
      <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0 0 24 12c0-6.63-5.37-12-12-12z"/>
    </svg>
    View on GitHub
  </a>
  <div class="footer-note">ESP32 Uptime Monitor &bull; Open Source</div>
</div>

</div><!-- /wrap -->

<!-- ─── Modals ────────────────────────────────────────── -->

<!-- Settings -->
<div class="modal" id="settingsModal">
  <div class="mbox">
    <div class="mhd">
      <h2>Settings</h2>
      <button class="mcl" id="clsSettings">&times;</button>
    </div>
    <div class="mbody">
      <div class="stabs-wrap"><div class="stabs" id="stabs"></div></div>
      <div id="spanes"></div>
    </div>
    <div class="mfoot">
      <button class="btn" id="cancelSettings">Cancel</button>
      <button class="btn btn-p" id="saveSettings">
        <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="20,6 9,17 4,12"/></svg>
        Save &amp; Restart
      </button>
    </div>
  </div>
</div>

<!-- Logs -->
<div class="modal" id="logsModal">
  <div class="mbox" style="max-width:720px">
    <div class="mhd">
      <h2>System Logs</h2>
      <button class="mcl" id="clsLogs">&times;</button>
    </div>
    <div class="mbody" style="padding:0">
      <pre id="logcontent" style="font-size:12px;color:#8090c0;white-space:pre-wrap;font-family:'JetBrains Mono',monospace;line-height:1.75;min-height:250px;padding:20px;margin:0">Loading...</pre>
    </div>
  </div>
</div>

<!-- Tools -->
<div class="modal" id="toolsModal">
  <div class="mbox">
    <div class="mhd">
      <h2>System Tools</h2>
      <button class="mcl" id="clsTools">&times;</button>
    </div>
    <div class="mbody">
      <!-- Tool tabs -->
      <div class="stabs-wrap" style="margin-bottom:18px">
        <div class="stabs" id="toolTabs">
          <button class="stab on" id="ttab-sys" onclick="switchToolTab('sys')">System</button>
          <button class="stab" id="ttab-ps" onclick="switchToolTab('ps')">Port Scanner</button>
        </div>
      </div>

      <!-- System tab -->
      <div id="toolPane-sys">
        <div class="sec">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="3" width="20" height="14" rx="2"/><line x1="8" y1="21" x2="16" y2="21"/><line x1="12" y1="17" x2="12" y2="21"/></svg>
          System Information
        </div>
        <div id="sysinfo" style="margin-bottom:22px"></div>
        <div class="sec sec-mt">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="16,16 12,12 8,16"/><line x1="12" y1="12" x2="12" y2="21"/><path d="M20.39 18.39A5 5 0 0 0 18 9h-1.26A8 8 0 1 0 3 16.3"/></svg>
          Firmware Update (OTA)
        </div>
        <div class="fg">
          <label>Select .bin firmware file</label>
          <input type="file" id="otaFile" accept=".bin">
        </div>
        <div id="otaprog" style="display:none;margin-bottom:14px">
          <div style="background:var(--c3);border-radius:6px;height:8px;overflow:hidden">
            <div id="otabar" style="background:linear-gradient(90deg,var(--bl),var(--pu));height:8px;width:0;transition:width .2s;border-radius:6px"></div>
          </div>
          <div id="otapct" style="font-size:12px;color:var(--mu);margin-top:5px">0%</div>
        </div>
        <button class="btn btn-p" id="otaBtn">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="16,16 12,12 8,16"/><line x1="12" y1="12" x2="12" y2="21"/><path d="M20.39 18.39A5 5 0 0 0 18 9h-1.26A8 8 0 1 0 3 16.3"/></svg>
          Upload Firmware
        </button>
      </div>

      <!-- Port Scanner tab -->
      <div id="toolPane-ps" style="display:none">
        <div class="sec">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>
          Port Scanner
        </div>
        <div class="ps-disclaimer">
          <b>⚠ Warning:</b> Use this tool responsibly. This port scanner is intended for scanning your own servers only. Scanning servers without authorization may be illegal in your country.<br>
          <label class="ps-check">
            <input type="checkbox" id="psAgree">
            <span>I confirm that I own or have explicit permission to scan the target host.</span>
          </label>
        </div>
        <div class="fg">
          <label>Host to scan</label>
          <input type="text" id="psHost" placeholder="192.168.1.1 or https://example.com" style="opacity:.5;pointer-events:none" disabled>
        </div>
        <button class="btn btn-p" id="psScanBtn" disabled style="opacity:.4">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
          Start Scan
        </button>
        <div class="ps-progress" id="psProgress">Scanning ports, please wait...</div>
        <div class="ps-results" id="psResults"></div>
      </div>

    </div>
  </div>
</div>

<script>
var refreshTimer = null;
var logTimer     = null;
var activeTab    = 0;
var lastData     = null;
var tabsBuilt    = false;
var NUM_TARGETS  = 10;

function ge(id) { return document.getElementById(id); }
function st(id, v) { var e = ge(id); if (e) e.textContent = v; }
function showM(id) { var e = ge(id); if (e) e.classList.add('show'); }
function hideM(id) { var e = ge(id); if (e) e.classList.remove('show'); }

function fmtUp(ms) {
  var s = Math.floor(ms / 1000);
  var d = Math.floor(s / 86400); s %= 86400;
  var h = Math.floor(s / 3600);  s %= 3600;
  var m = Math.floor(s / 60);    s %= 60;
  if (d > 0) return d + 'd ' + h + 'h ' + m + 'm';
  if (h > 0) return h + 'h ' + m + 'm ' + s + 's';
  return m + 'm ' + s + 's';
}

function fmtPct(a, b) { return b > 0 ? (a / b * 100).toFixed(1) + '%' : '0%'; }
function fmtPctNum(a, b) { return b > 0 ? a / b * 100 : 0; }

// Backdrop close
document.querySelectorAll('.modal').forEach(function(m) {
  m.addEventListener('click', function(e) { if (e.target === m) hideM(m.id); });
});

// ── Button wiring ──────────────────────────────────────────────────────────
ge('settingsBtn').onclick   = function() { stopRefresh(); openSettings(); };
ge('clsSettings').onclick   = function() { hideM('settingsModal'); startRefresh(); };
ge('cancelSettings').onclick = function() { hideM('settingsModal'); startRefresh(); };

ge('logsBtn').onclick = function() {
  stopRefresh();
  showM('logsModal');
  fetchLogs();
  logTimer = setInterval(fetchLogs, 3000);
};
ge('clsLogs').onclick = function() {
  hideM('logsModal');
  clearInterval(logTimer);
  startRefresh();
};

ge('toolsBtn').onclick = function() { showM('toolsModal'); switchToolTab('sys'); fetchSysDetail(); };
ge('clsTools').onclick = function() { hideM('toolsModal'); };

function switchToolTab(tab) {
  ['sys','ps'].forEach(function(t) {
    ge('toolPane-' + t).style.display = t === tab ? 'block' : 'none';
    ge('ttab-' + t).classList.toggle('on', t === tab);
  });
  if (tab === 'sys') fetchSysDetail();
}

// ── Refresh ────────────────────────────────────────────────────────────────
function startRefresh() {
  if (refreshTimer) return;
  fetchData();
  refreshTimer = setInterval(fetchData, 5000);
}

function stopRefresh() {
  clearInterval(refreshTimer);
  refreshTimer = null;
}

// ── System bar ────────────────────────────────────────────────────────────
async function fetchSys() {
  try {
    var d = await (await fetch('/api/system')).json();
    st('s-ip',   d.ip);
    st('s-rssi', d.rssi + ' dBm');
    st('s-heap', Math.round(d.free_heap / 1024) + ' KB');
    st('s-up',   fmtUp(d.uptime_ms));
    var led = ge('wled');
    if (led) {
      if (d.rssi > -80) { led.classList.add('on'); }
      else              { led.classList.remove('on'); }
    }
  } catch(e) {}
}

async function fetchSysDetail() {
  try {
    var d = await (await fetch('/api/system')).json();
    var rows = [
      ['IP Address',    d.ip],
      ['WiFi RSSI',     d.rssi + ' dBm'],
      ['Free Heap',     Math.round(d.free_heap / 1024) + ' KB'],
      ['Min Free Heap', Math.round(d.min_free_heap / 1024) + ' KB'],
      ['CPU Frequency', d.cpu_mhz + ' MHz'],
      ['Chip Model',    d.chip_model],
      ['Flash Size',    Math.round(d.flash_size / 1048576) + ' MB'],
      ['Uptime',        fmtUp(d.uptime_ms)],
      ['Firmware',      'v' + (d.firmware_ver / 10).toFixed(1)]
    ];
    ge('sysinfo').innerHTML = rows.map(function(r) {
      return '<div style="display:flex;justify-content:space-between;align-items:center;padding:8px 0;border-bottom:1px solid var(--bd);font-size:13px">' +
        '<span style="color:var(--mu)">' + r[0] + '</span>' +
        '<span style="font-family:monospace;font-weight:600">' + r[1] + '</span></div>';
    }).join('');
  } catch(e) {}
}

// ── Logs ──────────────────────────────────────────────────────────────────
async function fetchLogs() {
  try {
    var t = await (await fetch('/api/logs')).text();
    var el = ge('logcontent');
    if (el) { el.textContent = t || '(no logs yet)'; el.scrollTop = el.scrollHeight; }
  } catch(e) {}
}

// ── Main data ─────────────────────────────────────────────────────────────
async function fetchData() {
  try {
    var data = await (await fetch('/api/status')).json();
    lastData = data;
    renderTabs(data);
    renderTarget(data, activeTab);
    fetchSys();
  } catch(e) {}
}

// ── Tabs ──────────────────────────────────────────────────────────────────
function renderTabs(data) {
  var bar = ge('maintabs');
  if (!bar) return;
  if (!tabsBuilt) {
    tabsBuilt = true;
    bar.innerHTML = '';
    data.targets.forEach(function(t, i) {
      var btn = document.createElement('button');
      btn.className = 'tab' + (i === 0 ? ' on' : '');
      btn.id = 'tab-' + i;
      var dot = document.createElement('span');
      dot.className = 'tab-dot';
      dot.id = 'tdot-' + i;
      var label = document.createTextNode(t.name || ('S' + (i + 1)));
      btn.appendChild(dot);
      btn.appendChild(label);
      btn.onclick = (function(idx, b) {
        return function() {
          activeTab = idx;
          bar.querySelectorAll('.tab').forEach(function(x) { x.classList.remove('on'); });
          b.classList.add('on');
          if (lastData) renderTarget(lastData, idx);
        };
      })(i, btn);
      bar.appendChild(btn);
    });
  }
  data.targets.forEach(function(t, i) {
    var dot = ge('tdot-' + i);
    if (!dot) return;
    dot.className = 'tab-dot';
    if (t.enabled) dot.classList.add(t.online ? 'gr' : 're');
  });
}

// ── SVG: Uptime ring ──────────────────────────────────────────────────────
function uptimeRing(pct) {
  var r = 28, cx = 34, cy = 34, sw = 6;
  var circ = 2 * Math.PI * r;
  var offset = circ * (1 - pct / 100);
  var col = pct >= 99 ? 'var(--gr)' : pct >= 95 ? 'var(--or)' : 'var(--re)';
  return '<svg class="ring-wrap" width="68" height="68" viewBox="0 0 68 68">' +
    '<circle cx="' + cx + '" cy="' + cy + '" r="' + r + '" fill="none" stroke="var(--c3)" stroke-width="' + sw + '"/>' +
    '<circle cx="' + cx + '" cy="' + cy + '" r="' + r + '" fill="none" stroke="' + col + '" stroke-width="' + sw + '"' +
    ' stroke-dasharray="' + circ.toFixed(2) + '" stroke-dashoffset="' + offset.toFixed(2) + '"' +
    ' stroke-linecap="round" transform="rotate(-90 ' + cx + ' ' + cy + ')"' +
    ' style="transition:stroke-dashoffset .8s ease"/>' +
    '<text x="' + cx + '" y="' + (cy - 3) + '" text-anchor="middle" dominant-baseline="middle"' +
    ' fill="var(--tx)" font-size="11" font-weight="800" font-family="inherit">' + pct.toFixed(1) + '%</text>' +
    '<text x="' + cx + '" y="' + (cy + 10) + '" text-anchor="middle" dominant-baseline="middle"' +
    ' fill="var(--mu)" font-size="8" font-family="inherit">UPTIME</text>' +
    '</svg>';
}

// ── SVG: Sparkline ────────────────────────────────────────────────────────
function sparkline(history) {
  var pts = history.filter(function(v) { return v > 0; });
  var W = 300, H = 48;
  if (pts.length < 2) {
    return '<svg class="spark-svg" viewBox="0 0 ' + W + ' ' + H + '" style="height:' + H + 'px">' +
      '<text x="' + W/2 + '" y="' + H/2 + '" text-anchor="middle" fill="var(--mu)" font-size="11">Gathering data...</text></svg>';
  }
  var mx = Math.max.apply(null, pts);
  var mn = Math.min.apply(null, pts);
  var range = mx - mn || 1;
  var pad = 4;
  var step = (W - pad * 2) / (pts.length - 1);
  var points = pts.map(function(v, i) {
    var x = pad + i * step;
    var y = pad + (1 - (v - mn) / range) * (H - pad * 2);
    return x.toFixed(1) + ',' + y.toFixed(1);
  }).join(' ');
  var areaPoints = 'M ' + pad + ',' + H + ' L ' +
    pts.map(function(v, i) {
      return (pad + i * step).toFixed(1) + ',' + (pad + (1 - (v - mn) / range) * (H - pad * 2)).toFixed(1);
    }).join(' L ') + ' L ' + (pad + (pts.length - 1) * step).toFixed(1) + ',' + H + ' Z';
  return '<svg class="spark-svg" viewBox="0 0 ' + W + ' ' + H + '" style="height:' + H + 'px" preserveAspectRatio="none">' +
    '<defs><linearGradient id="sg" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="var(--bl)" stop-opacity=".35"/><stop offset="100%" stop-color="var(--bl)" stop-opacity="0"/></linearGradient></defs>' +
    '<path d="' + areaPoints + '" fill="url(#sg)"/>' +
    '<polyline points="' + points + '" fill="none" stroke="var(--bl)" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>' +
    '</svg>';
}

// ── SVG: Ping gauge bar ───────────────────────────────────────────────────
function pingGauge(ms) {
  var max = 1000;
  var pct = Math.min(100, ms / max * 100);
  var col = ms < 200 ? 'var(--gr)' : ms < 500 ? 'var(--or)' : 'var(--re)';
  return '<div class="gauge-wrap">' +
    '<div class="gauge-track"><div class="gauge-fill" style="width:' + pct.toFixed(1) + '%;background:' + col + '"></div></div>' +
    '</div>';
}

// ── Render target ─────────────────────────────────────────────────────────
function renderTarget(data, idx) {
  var t = data.targets[idx];
  var c = ge('maincontent');
  if (!t || !c) return;

  if (!t.enabled) {
    c.innerHTML = '<div class="card disabled-card">' +
      '<div class="disabled-icon">&#9711;</div>' +
      '<h3>Server Disabled</h3>' +
      '<p>Set a URL in Settings to enable monitoring for this server.</p>' +
      '</div>';
    return;
  }

  var pctNum = fmtPctNum(t.online_checks, t.total_checks);
  var hist   = t.ping_history || [];

  c.innerHTML =
    '<div class="card">' +
      '<div class="status-row">' +
        '<div class="status-left">' +
          '<div class="status-ind ' + (t.online ? 'on' : 'off') + '"></div>' +
          '<div>' +
            '<div class="status-name">' + (t.name || 'Server') + '</div>' +
            '<div class="status-url">' + (t.url || '') + '</div>' +
          '</div>' +
        '</div>' +
        '<div style="display:flex;gap:8px;align-items:center">' +
          '<span class="status-badge ' + (t.online ? 'badge-on' : 'badge-off') + '">' +
            '<span style="width:6px;height:6px;border-radius:50%;background:currentColor"></span>' +
            (t.online ? 'Online' : 'Offline') +
          '</span>' +
          '<button class="btn btn-sm" onclick="checkNow(' + idx + ')">Check Now</button>' +
        '</div>' +
      '</div>' +
      '<div class="stats-grid">' +
        '<div class="stat-card">' +
          '<div class="stat-val" style="color:' + (t.ping_ms < 200 ? 'var(--gr)' : t.ping_ms < 500 ? 'var(--or)' : 'var(--re)') + '">' + (t.ping_ms > 0 ? t.ping_ms + '<small style="font-size:.6em;font-weight:500">ms</small>' : '--') + '</div>' +
          '<div class="stat-lbl">Response</div>' +
          (t.ping_ms > 0 ? pingGauge(t.ping_ms) : '') +
        '</div>' +
        '<div class="stat-card">' +
          uptimeRing(pctNum) +
        '</div>' +
        '<div class="stat-card">' +
          '<div class="stat-val">' + (t.total_checks || 0).toLocaleString() + '</div>' +
          '<div class="stat-lbl">Total Checks</div>' +
        '</div>' +
      '</div>' +
    '</div>' +
    '<div class="spark-card">' +
      '<div class="spark-header">' +
        '<div class="spark-title">Response Time Trend</div>' +
        '<div class="spark-stats">' +
          '<div class="spark-stat">Min <b>' + (t.ping_min > 0 ? t.ping_min + ' ms' : '--') + '</b></div>' +
          '<div class="spark-stat">Avg <b>' + (t.ping_avg > 0 ? t.ping_avg + ' ms' : '--') + '</b></div>' +
          '<div class="spark-stat">Max <b>' + (t.ping_max > 0 ? t.ping_max + ' ms' : '--') + '</b></div>' +
        '</div>' +
      '</div>' +
      sparkline(hist) +
    '</div>' +

    // Events card
    '<div class="ev-card">' +
      '<div class="ev-title">Recent Events</div>' +
      renderEvents(t.log) +
    '</div>';
}

function renderEvents(log) {
  if (!log || !log.trim()) {
    return '<div class="ev-empty">No events recorded yet.</div>';
  }
  return log.trim().split('\n').slice(0, 12).map(function(line) {
    var on = line.indexOf('ONLINE') !== -1 && line.indexOf('OFFLINE') === -1;
    var time = line.match(/\[([^\]]+)\]/);
    var text = line.replace(/\[[^\]]+\]\s*/, '');
    return '<div class="ev-row">' +
      '<span class="ev-dot ' + (on ? 'ev-dot-on' : 'ev-dot-off') + '"></span>' +
      '<span class="ev-text">' + text + '</span>' +
      (time ? '<span class="ev-time">' + time[1] + '</span>' : '') +
      '</div>';
  }).join('');
}

async function checkNow(idx) {
  try {
    await fetch('/api/check_now', { method: 'POST', body: new URLSearchParams({ target: idx }) });
    setTimeout(fetchData, 1500);
  } catch(e) {}
}

// ── Settings helpers ───────────────────────────────────────────────────────
function fg(lbl, type, name, hint) {
  return '<div class="fg"><label>' + lbl + '</label>' +
    '<input type="' + type + '" name="' + name + '" id="sf_' + name + '">' +
    (hint ? '<div class="hint">' + hint + '</div>' : '') + '</div>';
}
function fgta(lbl, name, hint) {
  return '<div class="fg"><label>' + lbl + '</label>' +
    '<textarea name="' + name + '" id="sf_' + name + '"></textarea>' +
    (hint ? '<div class="hint">' + hint + '</div>' : '') + '</div>';
}
function fgsel(lbl, name, opts) {
  return '<div class="fg"><label>' + lbl + '</label><select name="' + name + '" id="sf_' + name + '">' +
    opts.map(function(o) { return '<option value="' + o + '">' + o[0].toUpperCase() + o.slice(1) + '</option>'; }).join('') +
    '</select></div>';
}
function fgpass(lbl, name, hint) {
  return '<div class="fg"><label>' + lbl + '</label><div class="input-wrap">' +
    '<input type="password" name="' + name + '" id="sf_' + name + '">' +
    '<button type="button" class="input-btn" onclick="togglePw(\'sf_' + name + '\')" title="Show/hide">' +
    '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>' +
    '</button></div>' +
    (hint ? '<div class="hint">' + hint + '</div>' : '') + '</div>';
}

function togglePw(id) {
  var e = ge(id);
  if (e) e.type = (e.type === 'password') ? 'text' : 'password';
}

function buildWifiPane() {
  return '<div class="sec">' +
    '<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><circle cx="12" cy="20" r="1" fill="currentColor"/></svg>' +
    'WiFi Connection</div>' +
    '<div class="fg"><label>Network SSID</label>' +
    '<div class="input-wrap">' +
    '<input type="text" name="cfg_ssid" id="sf_cfg_ssid">' +
    '<button type="button" class="input-btn" id="scanBtn" title="Scan networks">' +
    '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>' +
    '</button></div>' +
    '<div id="scanList" class="scan-list"></div></div>' +
    fgpass('Password', 'cfg_pass', 'Leave blank to keep current password') +
    fg('GMT Offset (hours)', 'number', 'cfg_gmt', 'e.g. 1 for UTC+1, -5 for UTC-5, 0 for UTC');
}

function buildServerPane(i) {
  return '<div class="sec">' +
    '<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="2" width="20" height="8" rx="2"/><rect x="2" y="14" width="20" height="8" rx="2"/><line x1="6" y1="6" x2="6.01" y2="6"/><line x1="6" y1="18" x2="6.01" y2="18"/></svg>' +
    'Server ' + (i + 1) + ' \u2014 Basic Setup</div>' +
    '<div class="frow2">' +
      fg('Display Name', 'text', 'name_' + i, '') +
      fg('Check Interval (s)', 'number', 'interval_' + i, 'Minimum: 5s') +
    '</div>' +
    fg('URL to Monitor', 'text', 'url_' + i, 'https://example.com  \u2014  or  0  to disable this server') +
    '<div class="frow2">' +
      fg('Fail Threshold', 'number', 'fail_' + i, 'Consecutive failures before alert') +
      fg('Recover Threshold', 'number', 'rec_' + i, 'Consecutive successes to recover') +
    '</div>' +

    '<div class="sec sec-mt">' +
    '<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M22 16.92v3a2 2 0 0 1-2.18 2 19.79 19.79 0 0 1-8.63-3.07A19.5 19.5 0 0 1 4.69 15a19.79 19.79 0 0 1-3.07-8.67A2 2 0 0 1 3.6 4.18h3a2 2 0 0 1 2 1.72c.127.96.361 1.903.7 2.81a2 2 0 0 1-.45 2.11L7.91 11.9a16 16 0 0 0 6 6l1.27-1.27a2 2 0 0 1 2.11-.45c.907.339 1.85.573 2.81.7A2 2 0 0 1 22 18.92z"/></svg>' +
    'Alert Messages</div>' +
    fgta('Online Message', 'msg_on_' + i, 'Placeholders: {NAME} {URL}') +
    fgta('Offline Message', 'msg_off_' + i, 'Placeholders: {NAME} {URL} {CODE}') +

    '<div class="sec sec-mt">' +
    '<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 8A6 6 0 0 0 6 8c0 7-3 9-3 9h18s-3-2-3-9"/><path d="M13.73 21a2 2 0 0 1-3.46 0"/></svg>' +
    'Notifications</div>' +

    '<div class="notif-block">' +
      '<div class="notif-hd"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/></svg>Discord</div>' +
      fg('Webhook URL', 'text', 'discord_' + i, '0 = disable') +
    '</div>' +
    '<div class="notif-block">' +
      '<div class="notif-hd"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 8A6 6 0 0 0 6 8c0 7-3 9-3 9h18s-3-2-3-9"/><path d="M13.73 21a2 2 0 0 1-3.46 0"/></svg>Ntfy</div>' +
      '<div class="frow2">' +
        fg('Topic URL', 'text', 'ntfy_url_' + i, '0 = disable') +
        fgsel('Priority', 'ntfy_pri_' + i, ['default','min','low','high','max']) +
      '</div>' +
    '</div>' +
    '<div class="notif-block">' +
      '<div class="notif-hd"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="22" y1="2" x2="11" y2="13"/><polygon points="22,2 15,22 11,13 2,9"/></svg>Telegram</div>' +
      fg('Bot Token', 'text', 'tg_token_' + i, '0 = disable') +
      '<div class="frow3">' +
        fg('Chat ID 1', 'text', 'tg_id1_' + i, '') +
        fg('Chat ID 2', 'text', 'tg_id2_' + i, '') +
        fg('Chat ID 3', 'text', 'tg_id3_' + i, '') +
      '</div>' +
    '</div>' +

    '<div class="sec sec-mt">' +
    '<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"/><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"/></svg>' +
    'HTTP Actions</div>' +
    '<div class="frow2">' +
      fg('GET on Online', 'text', 'http_on_' + i, '0 = disable') +
      fg('GET on Offline', 'text', 'http_off_' + i, '0 = disable') +
    '</div>';
}

function openSettings() {
  var stabs  = ge('stabs');
  var spanes = ge('spanes');
  stabs.innerHTML  = '';
  spanes.innerHTML = '';
  var panes = [{ id: 'sp-wifi', label: 'WiFi', html: buildWifiPane() }];
  for (var s = 0; s < NUM_TARGETS; s++) {
    panes.push({ id: 'sp-s' + s, label: 'S' + (s + 1), html: buildServerPane(s) });
  }
  panes.forEach(function(p, i) {
    var btn = document.createElement('button');
    btn.className = 'stab' + (i === 0 ? ' on' : '');
    btn.textContent = p.label;
    btn.title = p.label === 'WiFi' ? 'WiFi Settings' : 'Server ' + p.label.slice(1);
    var pid = p.id;
    btn.onclick = function() {
      stabs.querySelectorAll('.stab').forEach(function(b) { b.classList.remove('on'); });
      btn.classList.add('on');
      spanes.querySelectorAll('.spane').forEach(function(pn) { pn.classList.remove('on'); });
      ge(pid).classList.add('on');
    };
    stabs.appendChild(btn);
    var div = document.createElement('div');
    div.className = 'spane' + (i === 0 ? ' on' : '');
    div.id = p.id;
    div.innerHTML = p.html;
    spanes.appendChild(div);
  });

  // WiFi scan button
  var scanBtn = ge('scanBtn');
  if (scanBtn) {
    scanBtn.onclick = function() {
      var list = ge('scanList');
      if (!list) return;
      list.style.display = 'block';
      list.innerHTML = '<div class="scan-item" style="color:var(--mu);cursor:default">Scanning...</div>';
      fetch('/api/wifi_scan').then(function(r) { return r.json(); }).then(function(nets) {
        if (!nets.length) {
          list.innerHTML = '<div class="scan-item" style="color:var(--mu)">No networks found</div>';
          return;
        }
        list.innerHTML = nets.map(function(n) {
          var bars = n.rssi > -60 ? '\u2582\u2584\u2586\u2588' : n.rssi > -75 ? '\u2582\u2584\u2586' : n.rssi > -85 ? '\u2582\u2584' : '\u2582';
          return '<div class="scan-item" onclick="selectNet(\'' + n.ssid.replace(/'/g, "\\'") + '\')">' +
            '<span>' + n.ssid + '</span>' +
            '<span class="scan-rssi">' + bars + ' ' + n.rssi + 'dBm</span></div>';
        }).join('');
      }).catch(function() {
        list.innerHTML = '<div class="scan-item" style="color:var(--re)">Scan failed</div>';
      });
    };
  }

  loadSettings();
  showM('settingsModal');
}

function selectNet(ssid) {
  var inp = ge('sf_cfg_ssid');
  if (inp) inp.value = ssid;
  var list = ge('scanList');
  if (list) list.style.display = 'none';
}

async function loadSettings() {
  try {
    var d = await (await fetch('/api/settings')).json();
    function sv(name, val) {
      var e = ge('sf_' + name);
      if (!e) return;
      if (e.tagName === 'SELECT') e.value = val || '';
      else e.value = (val != null) ? val : '';
    }
    sv('cfg_ssid', d.ssid);
    sv('cfg_gmt',  d.gmt_offset);
    for (var i = 0; i < NUM_TARGETS; i++) {
      var t = (d.targets && d.targets[i]) ? d.targets[i] : {};
      sv('name_'     + i, t.name);
      sv('url_'      + i, t.url);
      sv('interval_' + i, t.interval);
      sv('fail_'     + i, t.fail_thr);
      sv('rec_'      + i, t.rec_thr);
      sv('msg_on_'   + i, t.msg_on);
      sv('msg_off_'  + i, t.msg_off);
      sv('discord_'  + i, t.discord);
      sv('ntfy_url_' + i, t.ntfy_url);
      sv('ntfy_pri_' + i, t.ntfy_pri);
      sv('tg_token_' + i, t.tg_token);
      sv('tg_id1_'   + i, t.tg_id1);
      sv('tg_id2_'   + i, t.tg_id2);
      sv('tg_id3_'   + i, t.tg_id3);
      sv('http_on_'  + i, t.http_on);
      sv('http_off_' + i, t.http_off);
    }
  } catch(e) {}
}

ge('saveSettings').onclick = async function() {
  var form = {};
  document.querySelectorAll('#spanes input,#spanes textarea,#spanes select').forEach(function(e) {
    if (e.name) form[e.name] = e.value;
  });
  try {
    var r = await fetch('/api/settings', { method: 'POST', body: new URLSearchParams(form) });
    if (r.ok) {
      hideM('settingsModal');
      alert('Settings saved. Device is restarting...');
    } else {
      alert('Error saving settings (HTTP ' + r.status + ')');
    }
  } catch(e) { hideM('settingsModal'); }
};

// ── Port Scanner ──────────────────────────────────────────────────────────
ge('psAgree').onchange = function() {
  var agreed = this.checked;
  ge('psHost').disabled = !agreed;
  ge('psHost').style.opacity = agreed ? '1' : '.5';
  ge('psHost').style.pointerEvents = agreed ? '' : 'none';
  ge('psScanBtn').disabled = !agreed;
  ge('psScanBtn').style.opacity = agreed ? '1' : '.4';
};

ge('psScanBtn').onclick = async function() {
  var host = ge('psHost').value.trim();
  if (!host) { alert('Please enter a host to scan.'); return; }
  var res = ge('psResults');
  var prog = ge('psProgress');
  res.style.display = 'none';
  prog.style.display = 'block';
  prog.textContent = 'Scanning ' + host + ', please wait... (this may take ~30 seconds)';
  ge('psScanBtn').disabled = true;
  try {
    var d = await (await fetch('/api/portscan?host=' + encodeURIComponent(host))).json();
    prog.style.display = 'none';
    res.style.display = 'block';
    var open = d.open_ports ? d.open_ports.split(',').filter(Boolean) : [];
    var report = '=== Port Scan Report ===\n';
    report += 'Host:    ' + d.host + '\n';
    report += 'Scanned: ' + d.total_scanned + ' common ports\n';
    report += 'Open:    ' + d.open_count + ' port(s)\n';
    report += '─────────────────────────\n';
    if (open.length === 0) {
      report += 'No open ports found.\n';
    } else {
      open.forEach(function(p) {
        var svc = portService(parseInt(p));
        report += 'PORT ' + p + '/tcp  OPEN' + (svc ? '  (' + svc + ')' : '') + '\n';
      });
    }
    report += '─────────────────────────\n';
    report += 'Scan complete.';
    res.textContent = report;
  } catch(e) {
    prog.style.display = 'none';
    res.style.display = 'block';
    res.textContent = 'Scan failed: ' + e.message;
  }
  ge('psScanBtn').disabled = false;
};

function portService(p) {
  var s = {21:'FTP',22:'SSH',23:'Telnet',25:'SMTP',53:'DNS',80:'HTTP',110:'POP3',
    143:'IMAP',443:'HTTPS',445:'SMB',465:'SMTPS',587:'SMTP',993:'IMAPS',995:'POP3S',
    1433:'MSSQL',1521:'Oracle',1883:'MQTT',3000:'Dev/Node',3001:'Dev',3306:'MySQL',
    3389:'RDP',5000:'Flask/Dev',5432:'PostgreSQL',5900:'VNC',6379:'Redis',
    8080:'HTTP-Alt',8081:'HTTP-Alt',8123:'Home Assistant',8443:'HTTPS-Alt',
    9000:'Portainer',9200:'Elasticsearch',27017:'MongoDB',32400:'Plex',51413:'Torrent'};
  return s[p] || '';
}

// ── OTA ───────────────────────────────────────────────────────────────────
ge('otaBtn').onclick = function() {
  var file = ge('otaFile').files[0];
  if (!file) { alert('Please select a .bin firmware file.'); return; }
  var fd = new FormData();
  fd.append('update', file);
  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/updatefirmware', true);
  ge('otaprog').style.display = 'block';
  xhr.upload.onprogress = function(ev) {
    if (ev.lengthComputable) {
      var pct = Math.round(ev.loaded / ev.total * 100);
      ge('otabar').style.width = pct + '%';
      ge('otapct').textContent = pct + '%';
    }
  };
  xhr.onload = function() {
    alert(xhr.status === 200 ? 'Update successful! Device is restarting...' : 'Update failed: HTTP ' + xhr.status);
  };
  xhr.send(fd);
};

// ── Start ─────────────────────────────────────────────────────────────────
startRefresh();
</script>
</body>
</html>
)rawliteral";
