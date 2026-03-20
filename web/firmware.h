#pragma once
// ── Firmware page (/firmware) — mirrors system_firmware.h from the MMDVM hotspot ──
// Dynamic page: all data (OTA settings, partition versions, remote size check)
// is fetched server-side at page render time — no JS version check needed.

#include "styles.h"
#include "navigation.h"
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

inline String getFirmwarePageHTML() {

  // ── Per-partition firmware versions (saved to NVS "ota" on each boot) ──────
  Preferences ota; ota.begin("ota", true);
  String ver0 = ota.getString("ver_app0", "Unknown");
  String ver1 = ota.getString("ver_app1", "Unknown");
  ota.end();

  // ── Partition info ────────────────────────────────────────────────────────
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* boot    = esp_ota_get_boot_partition();
  const esp_partition_t* app0    = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
  const esp_partition_t* app1    = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);

  String runLabel  = running ? String(running->label) : "unknown";
  String bootLabel = boot    ? String(boot->label)    : "unknown";

  // Override NVS version for the currently running partition with live value
  if (runLabel == "app0") ver0 = FIRMWARE_VERSION;
  else if (runLabel == "app1") ver1 = FIRMWARE_VERSION;

  bool   isBeta     = (String(FIRMWARE_VERSION).indexOf("_BETA") >= 0);
  size_t sketchSize = ESP.getSketchSize();

  // ── Build HTML ────────────────────────────────────────────────────────────
  String html;
  html.reserve(28000);

  html = "<!DOCTYPE html><html lang='en'><head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>Ulanzi Firmware</title>"
         "<style>" COMMON_CSS
         // page-specific
         ".progress-wrap{background:var(--border-color);border-radius:4px;height:16px;overflow:hidden;margin:8px 0}"
         ".progress-fill{height:100%;width:0;background:#00bcd4;transition:width .3s;border-radius:4px}"
         ".progress-text{font-size:.78em;color:var(--text-muted);text-align:right;font-family:monospace;margin-top:2px}"
         ".fw-badge{display:inline-block;font-size:.82em;font-weight:bold;padding:3px 12px;border-radius:12px;color:#fff;margin:6px 0}"
         ".fw-badge-success{background:#28a745}"
         ".fw-badge-warning{background:#ffc107;color:#212529}"
         ".fw-badge-danger{background:#dc3545}"
         "</style>"
         THEME_INIT_SCRIPT
         "</head><body>";

  html += NAV_BAR;
  html += NAV_LIVE_MODAL;

  html += "<div class='container'>";
  html += "<h1 style='margin-bottom:6px'>Firmware Update</h1>";
  html += "<p style='font-size:.9em;color:var(--text-muted);margin-bottom:16px'>Upload new firmware to update the device over the air (OTA).</p>";
  html += "<div class='grid'>";

  // ── Card 1: ESP32 Firmware ──────────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>ESP32 Firmware</h3>";
  html += "<div class='metric'><span class='metric-label'>Chip Model</span><span class='metric-value'>" + String(ESP.getChipModel()) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Partition Size</span><span class='metric-value'>" + String(running ? running->size : 0) + " B</span></div>";
  html += "<div class='metric'><span class='metric-label'>Version</span><span class='metric-value'>" + String(FIRMWARE_VERSION) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Build Date</span><span class='metric-value'>" + String(__DATE__) + " " + String(__TIME__) + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Firmware Size</span><span class='metric-value'>" + String(sketchSize) + " B</span></div>";

  // Remote firmware info — filled in by JS after page load (async, fast page)
  html += "<div class='metric'><span class='metric-label'>" + String(isBeta ? "Latest Beta" : "Latest Stable") + "</span>"
          "<span class='metric-value' id='latest-ver'>Checking\u2026</span></div>";
  html += "<div class='metric'><span class='metric-label'>Remote Size</span>"
          "<span class='metric-value' id='remote-size'>Checking\u2026</span></div>";
  html += "<div id='fw-badge-wrap' style='margin:6px 0 8px'></div>";

  // Details section: JS will open it if update available
  html += "<details id='fw-details' style='margin-top:8px'>";
  html += "<summary style='cursor:pointer;color:#007bff;font-size:.9em'>Update Options</summary>";
  html += "<div style='margin-top:10px'>";
  html += "<p style='font-size:.85em;color:var(--text-muted);margin-bottom:10px'>Over-the-Air (OTA) firmware update options:</p>";
  html += "<div style='margin-bottom:10px'>";
  html += "<label class='metric-label' style='display:block;margin-bottom:5px'>Update Version:</label>";
  html += "<select id='version-select' style='width:100%;padding:6px 8px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;font-size:.88em'>";
  if (isBeta) {
    html += "<option value='stable'>Stable Release</option>";
    html += "<option value='beta' selected>Beta Release</option>";
  } else {
    html += "<option value='stable' selected>Stable Release</option>";
    html += "<option value='beta'>Beta Release</option>";
  }
  html += "</select></div>";
  html += "<div style='display:flex;flex-direction:column;gap:8px'>";
  html += "<button class='btn btn-success' onclick='startOnlineUpdate()'>Online Update</button>";
  html += "<button class='btn btn-primary' onclick='document.getElementById(\"fw-file\").click()'>Upload File</button>";
  html += "</div>";
  html += "<input type='file' id='fw-file' accept='.bin' style='display:none'>";
  html += "<div class='progress-wrap' id='dl-progress-wrap' style='display:none'><div id='dl-progress-fill' class='progress-fill'></div></div>";
  html += "<div id='dl-progress-text' class='progress-text' style='display:none'></div>";
  html += "<div id='update-status' style='margin-top:8px;font-size:.85em;min-height:1.4em'></div>";
  html += "</div></details>";
  html += "</div>"; // card

  // ── Card 2: Partition Management ───────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>Partition Management</h3>";
  html += "<p style='font-size:.85em;color:var(--text-muted);margin-bottom:10px'>Switch between firmware versions:</p>";
  html += "<div class='metric'><span class='metric-label'>Running Partition</span><span class='metric-value'>" + runLabel + "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Boot Partition</span><span class='metric-value'>" + bootLabel + "</span></div>";

  if (app0) {
    String lbl = (runLabel == "app0") ? "app0 (Current)" : "app0";
    html += "<div class='metric'><span class='metric-label'>" + lbl + "</span><span class='metric-value'>" + ver0 + "</span></div>";
  }
  if (app1) {
    String lbl = (runLabel == "app1") ? "app1 (Current)" : "app1";
    html += "<div class='metric'><span class='metric-label'>" + lbl + "</span><span class='metric-value'>" + ver1 + "</span></div>";
  }

  html += "<div style='display:flex;flex-direction:column;gap:8px;margin-top:14px'>";
  if (app0) {
    String cls = (runLabel == "app0") ? "btn btn-success" : "btn btn-primary";
    String txt = (runLabel == "app0") ? "app0 (Running)" : "Boot app0";
    html += "<button class='" + cls + "' onclick='switchPartition(\"app0\")'>" + txt + "</button>";
  }
  if (app1) {
    String cls = (runLabel == "app1") ? "btn btn-success" : "btn btn-primary";
    String txt = (runLabel == "app1") ? "app1 (Running)" : "Boot app1";
    html += "<button class='" + cls + "' onclick='switchPartition(\"app1\")'>" + txt + "</button>";
  }
  html += "</div>";
  html += "<p style='font-size:.8em;color:var(--text-muted);margin-top:8px'>Switch partitions to roll back to a previous firmware version.</p>";
  html += "</div>"; // card

  // ── Card 3: ArduinoOTA Settings ────────────────────────────────────────
  html += "<div class='card'>";
  html += "<h3>ArduinoOTA Settings</h3>";

  html += "<div class='metric'>";
  html += "<span class='metric-label'>OTA Enabled</span>";
  html += "<label class='switch'><input type='checkbox' id='ota-enabled'" + String(otaEnabled ? " checked" : "") + "><span class='slider'></span></label>";
  html += "</div>";

  // Hidden username to prevent password manager confusion
  html += "<input type='text' id='ota-u' autocomplete='username' style='position:absolute;left:-9999px;width:1px;height:1px;opacity:0' tabindex='-1' aria-hidden='true'>";

  html += "<div class='metric' style='position:relative'>";
  html += "<span class='metric-label'>OTA Password</span>";
  html += "<input type='password' id='ota-password' value='" + String(otaPassword) + "' placeholder='Leave empty for no password'"
          " autocomplete='current-password' style='width:130px;padding:4px 28px 4px 6px;"
          "background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px'>";
  html += "<span style='position:absolute;right:8px;top:50%;transform:translateY(-50%);cursor:pointer'"
          " onclick='var i=document.getElementById(\"ota-password\");i.type=i.type===\"password\"?\"text\":\"password\"'>"
          "<svg width=16 height=16 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'>"
          "<path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg></span>";
  html += "</div>";

  html += "<div class='metric'>";
  html += "<span class='metric-label'>OTA Port</span>";
  html += "<input type='number' id='ota-port' value='" + String(otaPort) + "' min='1' max='65535'"
          " style='width:130px;padding:4px 6px;background:var(--bg-secondary);color:var(--text-color);"
          "border:1px solid var(--border-color);border-radius:4px'>";
  html += "</div>";

  html += "<p style='font-size:.82em;color:var(--text-muted);margin-top:8px'>"
          "ArduinoOTA allows firmware uploads from Arduino IDE over WiFi. Changes require a reboot.</p>";

  html += "<div style='display:flex;flex-direction:column;gap:8px;margin-top:12px'>";
  html += "<button class='btn btn-success' id='ota-save-btn' onclick='saveOtaSettings()'>Save</button>";
  html += "<button class='btn btn-danger'  id='ota-reset-btn' onclick='resetOtaSettings()'>Reset to Default</button>";
  html += "</div>";
  html += "<div id='ota-settings-status' style='margin-top:8px;font-size:.85em;display:none'></div>";
  html += "</div>"; // card

  html += "</div>"; // grid

  // Warning banner
  html += "<div style='background:var(--info-bg);border-left:4px solid #ffc107;padding:10px 14px;"
          "border-radius:4px;margin-top:16px;font-size:.85em;color:var(--text-color)'>"
          "<strong>Warning:</strong> Firmware updates will cause the system to restart. "
          "Make sure you have saved any important configuration changes before proceeding.</div>";

  html += "</div>"; // container

  // ── JavaScript ────────────────────────────────────────────────────────────
  // Embed local sketch size for JS comparison
  html += "<script>var _sketchSize=" + String(sketchSize) + ";</script>";
  html += "<script>" COMMON_JS NAV_LIVE_JS;
  html += R"JS(

// ── Remote firmware info (async, size-based check like MMDVM) ─────────────
(function checkRemote(){
  fetch('/api/remote-fw-info')
    .then(function(r){ return r.json(); })
    .then(function(d){
      document.getElementById('latest-ver').textContent  = d.latest_version || 'N/A';
      document.getElementById('remote-size').textContent = d.remote_size > 0 ? d.remote_size + ' B' : 'N/A';
      var badge = document.getElementById('fw-badge-wrap');
      if (d.remote_size > 0) {
        if (d.remote_size === _sketchSize) {
          badge.innerHTML = '<span class="fw-badge fw-badge-success">Firmware up to date</span>';
        } else {
          badge.innerHTML = '<span class="fw-badge fw-badge-danger">Update available</span>';
          document.getElementById('fw-details').setAttribute('open','');
        }
      } else {
        badge.innerHTML = '<span class="fw-badge fw-badge-warning">Remote URL not available</span>';
        document.getElementById('fw-details').setAttribute('open','');
      }
    })
    .catch(function(){
      document.getElementById('latest-ver').textContent  = 'N/A';
      document.getElementById('remote-size').textContent = 'N/A';
      document.getElementById('fw-badge-wrap').innerHTML =
        '<span class="fw-badge fw-badge-warning">Remote URL not available</span>';
      document.getElementById('fw-details').setAttribute('open','');
    });
})();

// ── Online update ─────────────────────────────────────────────────────────
function startOnlineUpdate() {
  var sel  = document.getElementById('version-select');
  var ver  = sel.value;
  var verTxt = ver === 'beta' ? 'BETA' : 'Stable';
  showConfirm('Download ' + verTxt + ' firmware update from GitHub?\nThis will check for the latest version.',
    function() {
      var statusDiv = document.getElementById('update-status');
      statusDiv.style.color = 'var(--text-muted)';
      statusDiv.textContent = 'Downloading ' + verTxt + ' firmware from GitHub\u2026';
      document.getElementById('dl-progress-wrap').style.display = 'block';
      document.getElementById('dl-progress-text').style.display = 'block';

      // Fake progress: increments every second up to 90%
      var elapsed = 0;
      var fakeTimer = setInterval(function(){
        elapsed++;
        var pct = Math.min(90, elapsed * 5);
        document.getElementById('dl-progress-fill').style.width = pct + '%';
        document.getElementById('dl-progress-text').textContent = 'Downloading\u2026 ' + pct + '%';
      }, 1000);

      fetch('/api/ota-download', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'version=' + encodeURIComponent(ver)
      })
      .then(function(r){ return r.text(); })
      .then(function(data){
        clearInterval(fakeTimer);
        if (data.indexOf('SUCCESS') === 0) {
          document.getElementById('dl-progress-fill').style.width = '100%';
          document.getElementById('dl-progress-text').textContent = '100% — download complete';
          // Show inline OK / Cancel — device screen shows CLICK OK
          statusDiv.innerHTML =
            '<div style="margin-bottom:8px">Firmware staged (' + data.replace('SUCCESS ','') + '). Reboot now?</div>' +
            '<div style="display:flex;gap:8px">' +
            '<button class="btn btn-success" onclick="otaConfirm()">Reboot Now</button>' +
            '<button class="btn btn-danger"  onclick="otaCancel()">Cancel</button>' +
            '</div>';
          statusDiv.style.color = 'var(--text-color)';
        } else {
          statusDiv.textContent = data;
          statusDiv.style.color = '#dc3545';
          document.getElementById('dl-progress-fill').style.background = '#dc3545';
        }
      })
      .catch(function(e){
        clearInterval(fakeTimer);
        document.getElementById('update-status').textContent = 'Network error: ' + e;
        document.getElementById('update-status').style.color = '#dc3545';
      });
    }
  );
}

// ── File upload ───────────────────────────────────────────────────────────
document.getElementById('fw-file').onchange = function(e) {
  var file = e.target.files[0];
  if (!file) return;
  if (!file.name.endsWith('.bin')) { showAlert('Please select a valid .bin firmware file'); e.target.value = ''; return; }
  showConfirm(
    'Upload and flash firmware: ' + file.name + ' (' + (file.size/1024).toFixed(1) + ' KB)?\n\nThe device will reboot after flashing.',
    function(){
      var fd = new FormData(); fd.append('firmware', file);
      var statusDiv = document.getElementById('update-status');
      document.getElementById('dl-progress-wrap').style.display = 'block';
      document.getElementById('dl-progress-text').style.display = 'block';
      statusDiv.textContent = 'Uploading\u2026'; statusDiv.style.color = 'var(--text-muted)';
      var xhr = new XMLHttpRequest();
      xhr.upload.onprogress = function(e){
        if (!e.lengthComputable) return;
        var pct = Math.round(e.loaded/e.total*100);
        document.getElementById('dl-progress-fill').style.width = pct + '%';
        document.getElementById('dl-progress-text').textContent = pct + '%';
      };
      xhr.onload = function(){
        var ok = false; var errMsg = 'unknown error';
        var awaitingConfirm = false;
        try {
          var r=JSON.parse(xhr.responseText);
          ok=r.ok;
          errMsg=r.error||errMsg;
          awaitingConfirm = !!r.awaiting_confirm;
        } catch(ex){ errMsg=xhr.responseText; }
        if (ok) {
          document.getElementById('dl-progress-fill').style.width = '100%';
          document.getElementById('dl-progress-text').textContent = '100%';
          if (awaitingConfirm) {
            // Menu-armed update: firmware is staged and the device shows CLICK OK.
            statusDiv.innerHTML =
              '<div style="margin-bottom:8px">Firmware staged. Confirm reboot on device (CLICK OK) or here:</div>' +
              '<div style="display:flex;gap:8px">' +
              '<button class="btn btn-success" onclick="otaConfirm()">Reboot Now</button>' +
              '<button class="btn btn-danger"  onclick="otaCancel()">Cancel</button>' +
              '</div>';
            statusDiv.style.color = 'var(--text-color)';
          } else {
            statusDiv.textContent = 'Upload complete. Rebooting\u2026'; statusDiv.style.color = '#28a745';
            document.body.innerHTML =
              '<div style="display:flex;justify-content:center;align-items:center;height:100vh;'
              +'font-family:sans-serif;flex-direction:column;gap:12px">'
              +'<div style="font-size:1.5em;font-weight:bold">Rebooting\u2026</div>'
              +'<div style="color:#888">Page reloads in 12 s.</div></div>';
            setTimeout(function(){ location.reload(); }, 12000);
          }
        } else {
          statusDiv.textContent = 'Error: ' + errMsg; statusDiv.style.color = '#dc3545';
        }
      };
      xhr.onerror = function(){ statusDiv.textContent = 'Upload error.'; statusDiv.style.color = '#dc3545'; };
      xhr.open('POST', '/api/ota-upload');
      xhr.send(fd);
    }
  );
};

// ── OTA confirm / cancel ──────────────────────────────────────────────────
function otaConfirm() {
  fetch('/api/ota-confirm', {method:'POST'}).catch(function(){});
  document.body.innerHTML =
    '<div style="display:flex;justify-content:center;align-items:center;height:100vh;'
    +'font-family:sans-serif;flex-direction:column;gap:12px">'
    +'<div style="font-size:1.5em;font-weight:bold">Rebooting\u2026</div>'
    +'<div style="color:#888">Page reloads in 12 s.</div></div>';
  setTimeout(function(){ location.reload(); }, 12000);
}

function otaCancel() {
  var statusDiv = document.getElementById('update-status');
  statusDiv.textContent = 'Cancelling\u2026';
  statusDiv.style.color = 'var(--text-muted)';
  fetch('/api/ota-cancel', {method:'POST'})
    .then(function(){
      statusDiv.textContent = 'Update cancelled. Device returned to normal.';
      statusDiv.style.color = '#dc3545';
      document.getElementById('dl-progress-fill').style.background = '#dc3545';
    });
}

// ── Partition switch ──────────────────────────────────────────────────────
function switchPartition(p) {
  showConfirm(
    'Switch boot partition to ' + p + '?\n\nThe system will reboot and start from ' + p + '.\n\nContinue?',
    function(){
      fetch('/api/partition-switch', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'partition=' + p
      })
      .then(function(r){ return r.json(); })
      .then(function(d){
        if (d.ok) {
          document.body.innerHTML =
            '<div style="display:flex;justify-content:center;align-items:center;height:100vh;'
            +'font-family:sans-serif;flex-direction:column;gap:12px">'
            +'<div style="font-size:1.5em;font-weight:bold">Rebooting to ' + p + '\u2026</div>'
            +'<div style="color:#888">Page reloads in 12 s.</div></div>';
          setTimeout(function(){ location.reload(); }, 12000);
        } else { showAlert('Error: ' + (d.error || 'Failed')); }
      })
      .catch(function(e){ showAlert('Error: ' + e); });
    }
  );
}

// ── ArduinoOTA settings ───────────────────────────────────────────────────
function saveOtaSettings() {
  var enabled  = document.getElementById('ota-enabled').checked ? '1' : '0';
  var password = document.getElementById('ota-password').value;
  var port     = document.getElementById('ota-port').value;
  if (port < 1 || port > 65535) { showAlert('Port must be 1\u201365535'); return; }
  showConfirm('Save ArduinoOTA settings and reboot?', function(){
    var sd = document.getElementById('ota-settings-status');
    sd.style.display = 'block'; sd.textContent = 'Saving\u2026'; sd.style.color = 'var(--text-muted)';
    document.getElementById('ota-save-btn').disabled  = true;
    document.getElementById('ota-reset-btn').disabled = true;
    fetch('/api/save-ota-settings', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: 'enabled=' + enabled + '&password=' + encodeURIComponent(password) + '&port=' + port
    })
    .then(function(r){ return r.text(); })
    .then(function(msg){
      sd.textContent = msg; sd.style.color = '#28a745';
      document.body.innerHTML =
        '<div style="display:flex;justify-content:center;align-items:center;height:100vh;'
        +'font-family:sans-serif;flex-direction:column;gap:12px">'
        +'<div style="font-size:1.5em;font-weight:bold">Rebooting\u2026</div>'
        +'<div style="color:#888">Page reloads in 12 s.</div></div>';
      setTimeout(function(){ location.reload(); }, 12000);
    })
    .catch(function(e){
      sd.textContent = 'Error: ' + e; sd.style.color = '#dc3545';
      document.getElementById('ota-save-btn').disabled  = false;
      document.getElementById('ota-reset-btn').disabled = false;
    });
  });
}

function resetOtaSettings() {
  showConfirm('Reset ArduinoOTA settings to defaults and reboot?', function(){
    fetch('/api/reset-ota-settings', {method: 'POST'})
    .then(function(r){ return r.text(); })
    .then(function(msg){
      showAlert(msg + '\n\nThe device will now reboot.');
      setTimeout(function(){
        document.body.innerHTML =
          '<div style="display:flex;justify-content:center;align-items:center;height:100vh;'
          +'font-family:sans-serif;flex-direction:column;gap:12px">'
          +'<div style="font-size:1.5em;font-weight:bold">Rebooting\u2026</div>'
          +'<div style="color:#888">Page reloads in 12 s.</div></div>';
        setTimeout(function(){ location.reload(); }, 12000);
      }, 1500);
    });
  });
}
)JS";

  html += "</script>";
  html += "</body></html>";
  return html;
}
