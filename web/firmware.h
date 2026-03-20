#pragma once
#include "styles.h"
#include "navigation.h"

// ── Firmware page (/firmware): version info, web OTA upload, ArduinoOTA, partition ──
static const char PAGE_FIRMWARE[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Firmware</title>"
  "<style>" COMMON_CSS
  ".progress-wrap{background:var(--border-color);border-radius:4px;height:16px;overflow:hidden;margin:8px 0}"
  ".progress-bar{height:100%;width:0;background:#00bcd4;transition:width .25s;border-radius:4px}"
  ".progress-pct{font-size:.78em;color:var(--text-muted);text-align:right;font-family:monospace;margin-bottom:4px}"
  ".drop-zone{border:2px dashed var(--border-color);border-radius:6px;padding:22px;text-align:center;"
  "cursor:pointer;margin-bottom:12px;transition:border-color .2s}"
  ".drop-zone:hover,.drop-zone.over{border-color:#00bcd4}"
  "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <!-- Firmware Info -->
  <div class="card">
    <h3>Firmware Info</h3>
    <div class="metric"><span class="metric-label">Version</span><span class="metric-value" id="fw-version">…</span></div>
    <div class="metric"><span class="metric-label">Build</span><span class="metric-value" id="fw-build">…</span></div>
    <div class="metric"><span class="metric-label">Sketch size</span><span class="metric-value" id="fw-sketch-kb">…</span></div>
    <div class="metric"><span class="metric-label">OTA space</span><span class="metric-value" id="fw-ota-kb">…</span></div>
    <div class="metric"><span class="metric-label">Partition</span><span class="metric-value" id="fw-partition">…</span></div>
    <div class="metric" style="border-bottom:none">
      <span class="metric-label">MD5</span>
      <span class="metric-value" id="fw-md5" style="font-size:.72em;word-break:break-all;text-align:right;max-width:200px">…</span>
    </div>
  </div>

  <!-- Web OTA Upload -->
  <div class="card">
    <h3>Upload Firmware</h3>
    <p style="font-size:.85em;color:var(--text-muted);margin-bottom:12px">
      Flash a <code style="background:var(--bg-secondary);padding:1px 4px;border-radius:3px">.bin</code>
      compiled in Arduino IDE. The device reboots automatically after flashing.
    </p>
    <div id="drop-zone" class="drop-zone"
         ondragover="event.preventDefault();this.classList.add('over')"
         ondragleave="this.classList.remove('over')"
         ondrop="onDrop(event)"
         onclick="document.getElementById('fw-file').click()">
      <div style="font-size:2em;margin-bottom:6px">&#128190;</div>
      <div style="font-size:.88em;color:var(--text-muted)">Click or drag &amp; drop a <strong>.bin</strong> file here</div>
      <div id="fw-filename" style="margin-top:6px;font-size:.82em;font-family:monospace;color:#00bcd4"></div>
    </div>
    <input type="file" id="fw-file" accept=".bin" style="display:none" onchange="onFileSelect(event)">
    <div id="fw-progress-wrap" class="progress-wrap" style="display:none">
      <div id="fw-progress-bar" class="progress-bar"></div>
    </div>
    <div id="fw-progress-pct" class="progress-pct" style="display:none"></div>
    <div id="fw-status" style="font-size:.85em;margin-top:6px;min-height:1.4em"></div>
    <div style="margin-top:10px">
      <button id="fw-upload-btn" class="btn btn-success" onclick="startUpload()" disabled>Flash Firmware</button>
    </div>
  </div>

  <!-- ArduinoOTA -->
  <div class="card">
    <h3>ArduinoOTA</h3>
    <p style="font-size:.85em;color:var(--text-muted);margin-bottom:10px">
      Upload firmware from Arduino IDE or <code style="background:var(--bg-secondary);padding:1px 4px;border-radius:3px">arduino-cli</code> over the network.
    </p>
    <div class="metric"><span class="metric-label">Status</span><span class="metric-value" id="ota-status">…</span></div>
    <div class="metric"><span class="metric-label">Host</span><span class="metric-value" id="ota-host" style="font-family:monospace;font-size:.82em">…</span></div>
    <div class="metric"><span class="metric-label">Port</span><span class="metric-value">3232</span></div>
    <div class="metric"><span class="metric-label">Password</span><span class="metric-value" id="ota-pw">…</span></div>
    <div style="font-size:.78em;color:var(--text-muted);margin-top:10px;line-height:1.8;background:var(--bg-secondary);padding:8px 10px;border-radius:4px">
      <strong>Arduino IDE:</strong> Tools &#8594; Port &#8594; <span id="ota-ide-label" style="font-family:monospace;color:#00bcd4">…</span><br>
      <strong>arduino-cli:</strong> compile then upload with <code style="background:var(--border-color);padding:1px 4px;border-radius:3px">--port &lt;ip&gt; --fqbn ...</code>
    </div>
  </div>

  <!-- Partition Info -->
  <div class="card">
    <h3>Partition Info</h3>
    <div class="metric"><span class="metric-label">Running</span><span class="metric-value" id="part-running">…</span></div>
    <div class="metric"><span class="metric-label">Boot target</span><span class="metric-value" id="part-boot">…</span></div>
    <div class="metric"><span class="metric-label">app0 size</span><span class="metric-value" id="part-app0">…</span></div>
    <div class="metric" style="border-bottom:none"><span class="metric-label">app1 size</span><span class="metric-value" id="part-app1">…</span></div>
    <div style="display:flex;gap:8px;margin-top:14px;flex-wrap:wrap">
      <button id="btn-app0" class="btn btn-secondary" onclick="switchPartition('app0')">Boot app0</button>
      <button id="btn-app1" class="btn btn-secondary" onclick="switchPartition('app1')">Boot app1</button>
    </div>
    <p style="font-size:.8em;color:var(--text-muted);margin-top:8px">
      Switch partition to roll back to the previous firmware. The device will reboot.
    </p>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
var _file = null;

function onDrop(e) {
  e.preventDefault();
  document.getElementById('drop-zone').classList.remove('over');
  setFile(e.dataTransfer.files[0]);
}
function onFileSelect(e) { setFile(e.target.files[0]); }
function setFile(f) {
  if (!f) return;
  if (!f.name.endsWith('.bin')) { showAlert('Please select a .bin firmware file.'); return; }
  _file = f;
  document.getElementById('fw-filename').textContent = f.name + ' (' + (f.size/1024).toFixed(1) + ' KB)';
  document.getElementById('fw-upload-btn').disabled = false;
  document.getElementById('fw-status').textContent = '';
}
function startUpload() {
  if (!_file) { showAlert('No file selected.'); return; }
  showConfirm(
    'Flash \'' + _file.name + '\' (' + (_file.size/1024).toFixed(1) + ' KB)?\n\nThe device will reboot automatically after flashing.',
    function() {
      var fd = new FormData();
      fd.append('firmware', _file);
      var xhr = new XMLHttpRequest();
      document.getElementById('fw-upload-btn').disabled = true;
      xhr.upload.onprogress = function(e) {
        if (!e.lengthComputable) return;
        var pct = Math.round(e.loaded / e.total * 100);
        document.getElementById('fw-progress-wrap').style.display = 'block';
        document.getElementById('fw-progress-pct').style.display  = 'block';
        document.getElementById('fw-progress-bar').style.width    = pct + '%';
        document.getElementById('fw-progress-pct').textContent    = pct + '%';
        document.getElementById('fw-status').textContent          = 'Uploading\u2026';
        document.getElementById('fw-status').style.color          = 'var(--text-muted)';
      };
      xhr.onload = function() {
        var ok = false;
        var errMsg = 'unknown error';
        try {
          var r = JSON.parse(xhr.responseText);
          ok = r.ok;
          errMsg = r.error || errMsg;
        } catch(ex) { errMsg = xhr.responseText; }
        if (ok) {
          document.getElementById('fw-progress-bar').style.width = '100%';
          document.getElementById('fw-progress-pct').textContent = '100%';
          document.getElementById('fw-status').textContent       = 'Flashing complete \u2014 rebooting\u2026';
          document.getElementById('fw-status').style.color       = '#28a745';
          document.body.innerHTML =
            '<div style="display:flex;justify-content:center;align-items:center;height:100vh;'
            +'font-family:sans-serif;flex-direction:column;gap:12px">'
            +'<div style="font-size:1.5em;font-weight:bold">Rebooting\u2026</div>'
            +'<div style="color:#888">Page reloads in 12 s.</div></div>';
          setTimeout(function(){ location.reload(); }, 12000);
        } else {
          document.getElementById('fw-status').textContent = 'Error: ' + errMsg;
          document.getElementById('fw-status').style.color = '#dc3545';
          document.getElementById('fw-upload-btn').disabled = false;
        }
      };
      xhr.onerror = function() {
        document.getElementById('fw-status').textContent = 'Upload failed \u2014 check connection.';
        document.getElementById('fw-status').style.color = '#dc3545';
        document.getElementById('fw-upload-btn').disabled = false;
      };
      xhr.open('POST', '/api/ota-upload');
      xhr.send(fd);
    }
  );
}
function switchPartition(p) {
  showConfirm('Switch boot partition to ' + p + ' and reboot?\n\nUse this to roll back to a previous firmware.', function() {
    fetch('/api/partition-switch', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: 'partition=' + p
    }).then(function(r){ return r.json(); }).then(function(d){
      if (d.ok) {
        document.body.innerHTML =
          '<div style="display:flex;justify-content:center;align-items:center;height:100vh;'
          +'font-family:sans-serif;flex-direction:column;gap:12px">'
          +'<div style="font-size:1.5em;font-weight:bold">Rebooting to ' + p + '\u2026</div>'
          +'<div style="color:#888">Page reloads in 12 s.</div></div>';
        setTimeout(function(){ location.reload(); }, 12000);
      } else {
        showAlert('Error: ' + (d.error || 'Failed to switch partition'));
      }
    }).catch(function(e){ showAlert('Error: ' + e); });
  });
}
(function init(){
  fetch('/api/firmware-info')
    .then(function(r){ return r.json(); })
    .then(function(d){
      document.getElementById('fw-version').textContent   = d.version  || '\u2014';
      document.getElementById('fw-build').textContent     = d.build    || '\u2014';
      document.getElementById('fw-sketch-kb').textContent = d.sketch_kb + ' KB';
      document.getElementById('fw-ota-kb').textContent    = d.ota_kb   + ' KB free';
      document.getElementById('fw-partition').textContent = d.partition || '\u2014';
      document.getElementById('fw-md5').textContent       = d.md5      || '\u2014';

      var otaReady = d.ota_started;
      var otaEl = document.getElementById('ota-status');
      otaEl.textContent  = otaReady ? 'Ready' : 'Not started';
      otaEl.style.color  = otaReady ? '#28a745' : '#dc3545';

      var host = d.ota_host + ':3232';
      document.getElementById('ota-host').textContent      = host;
      document.getElementById('ota-ide-label').textContent = host;
      document.getElementById('ota-pw').textContent        = d.ota_password ? 'Set' : 'None';

      document.getElementById('part-running').textContent  = d.part_running || '\u2014';
      document.getElementById('part-boot').textContent     = d.part_boot    || '\u2014';
      document.getElementById('part-app0').textContent     = d.part_app0_kb ? d.part_app0_kb + ' KB' : '\u2014';
      document.getElementById('part-app1').textContent     = d.part_app1_kb ? d.part_app1_kb + ' KB' : '\u2014';

      // Mark the currently running partition button
      ['app0','app1'].forEach(function(p){
        var btn = document.getElementById('btn-' + p);
        if (d.part_running === p) {
          btn.className   = 'btn btn-success';
          btn.textContent = p + ' (Running)';
        }
      });
    })
    .catch(function(){});
})();
</script>
</body></html>
)html";
