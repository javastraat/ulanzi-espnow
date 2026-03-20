#pragma once
// WiFi configuration page — multi-slot networks + SoftAP settings.
// Served dynamically so current status and slot data are embedded at request time.

#include "web/styles.h"
#include "web/navigation.h"

inline String getWifiPageHTML() {
  String html;
  html.reserve(32000);

  html = "<!DOCTYPE html><html lang='en'><head>"
         "<meta charset='UTF-8'>"
         "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
         "<title>Ulanzi WiFi</title>"
         "<style>" COMMON_CSS
         // Page-specific styles
         ".status-box{padding:8px 12px;border-radius:4px;margin-bottom:10px;font-weight:bold;font-size:.9em}"
         ".status-connected{background:#d4edda;color:#155724}"
         ".status-ap{background:#fff3cd;color:#856404}"
         ".status-disconnected{background:#f8d7da;color:#721c24}"
         ".info-box{background:var(--info-bg);padding:10px;border-radius:6px;margin-bottom:12px;font-size:.85em}"
         ".scan-results{margin-top:12px}"
         ".action-col{display:flex;flex-direction:column;gap:8px;margin-top:10px}"
         "</style>"
         THEME_INIT_SCRIPT
         "</head><body>";

  html += NAV_BAR;
  html += NAV_LIVE_MODAL;

  html += "<div class='container'>";
  html += "<h1 style='margin-bottom:6px'>WiFi Configuration</h1>";
  html += "<p style='margin-bottom:16px;font-size:.9em;color:var(--text-muted)'>Configure up to 6 networks. On boot the clock tries them in order; starts an Access Point if none connect.</p>";
  html += "<div class='grid'>";

  // ── Card 1: Current status ──────────────────────────────────────────────────
  html += "<div class='card'><h3>Current Status</h3>";
  if (WiFi.isConnected()) {
    int rssi = WiFi.RSSI();
    const char* q = rssi >= -50 ? "Excellent" : rssi >= -60 ? "Good" : rssi >= -70 ? "Fair" : "Weak";
    html += "<div class='status-box status-connected'>&#10003; Connected to WiFi</div>";
    html += "<div class='metric'><span class='metric-label'>SSID</span><span class='metric-value'>" + WiFi.SSID() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>IP Address</span><span class='metric-value'>" + WiFi.localIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Gateway</span><span class='metric-value'>" + WiFi.gatewayIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Signal</span><span class='metric-value'>" + String(rssi) + " dBm (" + q + ")</span></div>";
    html += "<div class='metric'><span class='metric-label'>Channel</span><span class='metric-value'>" + String(WiFi.channel()) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>MAC</span><span class='metric-value'>" + WiFi.macAddress() + "</span></div>";
  } else if (softAPActive) {
    html += "<div class='status-box status-ap'>&#9654; Access Point active</div>";
    html += "<div class='metric'><span class='metric-label'>AP SSID</span><span class='metric-value'>" + WiFi.softAPSSID() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>AP IP</span><span class='metric-value'>" + WiFi.softAPIP().toString() + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Clients</span><span class='metric-value'>" + String(WiFi.softAPgetStationNum()) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>Channel</span><span class='metric-value'>" + String(WiFi.channel()) + "</span></div>";
    html += "<div class='metric'><span class='metric-label'>MAC</span><span class='metric-value'>" + WiFi.softAPmacAddress() + "</span></div>";
  } else {
    html += "<div class='status-box status-disconnected'>&#10007; Not connected</div>";
    html += "<div class='metric'><span class='metric-label'>MAC</span><span class='metric-value'>" + WiFi.macAddress() + "</span></div>";
  }
  html += "</div>"; // card

  // ── Card 2: Configure network slots ────────────────────────────────────────
  html += "<div class='card'><h3>Configure Networks</h3>";
  html += "<div class='info-box'>Select a slot, fill in credentials and save. The clock tries all non-empty slots in order on every boot.</div>";
  html += "<form onsubmit='return false'>";
  html += "<input type='hidden' id='currentSlot' value='0'>";
  html += "<div class='metric'><span class='metric-label'>Slot</span>";
  html += "<select id='slotSelector' style='width:140px;padding:4px 6px;margin-left:8px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px'>";
  for (int i = 0; i < WIFI_SLOT_COUNT; i++) {
    String opt = "Slot " + String(i + 1) + ": ";
    opt += (wifiSlotLabel[i].length() > 0 ? wifiSlotLabel[i] : "-");
    if (wifiSlotSsid[i].length() > 0) opt += " (" + wifiSlotSsid[i] + ")";
    html += "<option value='" + String(i) + "'>" + opt + "</option>";
  }
  html += "</select></div>";
  html += "<div style='margin-top:12px'></div>";
  html += "<div class='metric'><span class='metric-label'>Label</span>"
          "<input type='text' id='labelInput' maxlength='20' placeholder='e.g. Home' value='" + wifiSlotLabel[0] + "' style='width:140px;padding:4px 6px;margin-left:8px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px'></div>";
  html += "<div class='metric'><span class='metric-label'>SSID</span>"
          "<input type='text' id='ssidInput' placeholder='Network name' value='" + wifiSlotSsid[0] + "' autocomplete='username' style='width:140px;padding:4px 6px;margin-left:8px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px'></div>";
  html += "<div class='metric' style='position:relative'><span class='metric-label'>Password</span>"
          "<input type='password' id='passwordInput' placeholder='Password' value='" + wifiSlotPass[0] + "' autocomplete='current-password' style='width:120px;padding:4px 24px 4px 6px;margin-left:8px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px'>"
          "<span style='position:absolute;right:8px;top:50%;transform:translateY(-50%);cursor:pointer' onclick='togglePw(\"passwordInput\",this)'>"
          "<svg width=16 height=16 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg>"
          "</span></div>";
  html += "<div class='action-col'>"
          "<button type='button' onclick='saveSlot()' class='btn btn-success'>Save This Network</button>"
          "<button type='button' onclick='resetSlot()' class='btn btn-danger'>Reset Slot to Default</button>"
          "</div>";
  html += "</form></div>"; // card

  // ── Card 3: Scan ────────────────────────────────────────────────────────────
  html += "<div class='card'><h3>Available Networks</h3>";
  html += "<button class='btn btn-warning' onclick='scanNetworks()'>Scan for Networks</button>";
  html += "<div id='scan-results' class='scan-results' style='display:none'><div id='networks'>Scanning...</div></div>";
  html += "</div>"; // card

  // ── Card 4: Access Point settings ──────────────────────────────────────────
  html += "<div class='card'><h3>Access Point Settings</h3>";
  html += "<p style='font-size:.85em;color:var(--text-muted);margin-bottom:10px'>Used as fallback when no saved network connects. Connect to this AP and browse to 192.168.4.1 to configure WiFi.</p>";
  html += "<form onsubmit='return false'>";
  html += "<div class='metric'><span class='metric-label'>AP SSID</span>"
          "<input type='text' id='apSsid' value='" + wifiApSsid + "' maxlength='32' style='width:140px;padding:4px 6px;margin-left:8px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px' autocomplete='username'></div>";
  html += "<div class='metric' style='position:relative'><span class='metric-label'>AP Password</span>"
          "<input type='password' id='apPassword' value='" + wifiApPassword + "' minlength='8' maxlength='63' style='width:120px;padding:4px 24px 4px 6px;margin-left:8px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px' autocomplete='current-password'>"
          "<span style='position:absolute;right:8px;top:50%;transform:translateY(-50%);cursor:pointer' onclick='togglePw(\"apPassword\",this)'>"
          "<svg width=16 height=16 viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7-11-7-11-7z'/><circle cx='12' cy='12' r='3'/></svg>"
          "</span></div>";
  html += "<div class='metric'><span class='metric-label'>Channel</span>"
          "<input type='number' id='apChannel' value='" + String(wifiApChannel) + "' min='1' max='13' style='width:140px;padding:4px 6px;margin-left:8px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px'></div>";
  html += "<div class='metric'><span class='metric-label'>Retries/slot</span>"
          "<input type='number' id='apRetries' value='" + String(wifiMaxRetries) + "' min='1' max='40' style='width:140px;padding:4px 6px;margin-left:8px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px'></div>";
  html += "<p style='font-size:.8em;color:var(--text-muted);margin-top:6px'>Retries × 500 ms = per-slot timeout (10 → 5 s).</p>";
  html += "<div class='action-col' style='margin-top:12px'>"
          "<button class='btn btn-success' type='button' onclick='saveAP()'>Save AP &amp; Reboot</button>"
          "<button class='btn btn-secondary' type='button' onclick='resetAP()'>Reset to Default</button>"
          "</div>";
  html += "</form></div>"; // card

  html += "</div>"; // grid

  // ── Save All button ─────────────────────────────────────────────────────────
  html += "<div style='margin-top:20px'>";
  html += "<button class='btn btn-success' onclick='saveAll()'>Save All &amp; Reboot</button>";
  html += "</div>";
  html += "<div class='info-box' style='margin-top:16px'><strong>Note:</strong> Slot changes are saved immediately. Use <b>Save All &amp; Reboot</b> to also save AP settings and reboot.</div>";

  // ── JavaScript ──────────────────────────────────────────────────────────────
  html += "<script>" COMMON_JS NAV_LIVE_JS;
  html += R"JS(
var currentSlot = 0;

function togglePw(id, btn) {
  var inp = document.getElementById(id);
  inp.type = inp.type === 'password' ? 'text' : 'password';
}

function loadSlot(slot) {
  fetch('/api/wifi-slot?slot=' + slot)
    .then(function(r){ return r.json(); })
    .then(function(d){
      document.getElementById('labelInput').value    = d.label    || '';
      document.getElementById('ssidInput').value     = d.ssid     || '';
      document.getElementById('passwordInput').value = d.password || '';
      document.getElementById('currentSlot').value   = slot;
      currentSlot = slot;
    })
    .catch(function(){ });
}

document.getElementById('slotSelector').addEventListener('change', function(){
  loadSlot(parseInt(this.value));
});

function saveSlot() {
  var slot  = document.getElementById('currentSlot').value;
  var label = document.getElementById('labelInput').value;
  var ssid  = document.getElementById('ssidInput').value;
  var pass  = document.getElementById('passwordInput').value;
  fetch('/api/save-wifi-slot', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'slot=' + slot + '&label=' + encodeURIComponent(label) +
          '&ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass)
  }).then(function(r){ return r.text(); })
    .then(function(msg){
      showAlert(msg);
      refreshSlotSelector();
    })
    .catch(function(e){ showAlert('Error: ' + e); });
}

function resetSlot() {
  showConfirm('Reset slot ' + (currentSlot + 1) + ' to its default?', function(){
    fetch('/api/reset-wifi-slot', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: 'slot=' + currentSlot
    }).then(function(r){ return r.json(); })
      .then(function(d){
        document.getElementById('labelInput').value    = d.label    || '';
        document.getElementById('ssidInput').value     = d.ssid     || '';
        document.getElementById('passwordInput').value = d.password || '';
        refreshSlotSelector();
        showAlert('Slot reset.');
      })
      .catch(function(e){ showAlert('Error: ' + e); });
  });
}

function refreshSlotSelector() {
  fetch('/api/wifi-slots')
    .then(function(r){ return r.json(); })
    .then(function(d){
      var sel = document.getElementById('slotSelector');
      for (var i = 0; i < 6; i++) {
        var s = d.slots[i];
        var t = 'Slot ' + (i+1) + ': ' + (s.label || '-');
        if (s.ssid) t += ' (' + s.ssid + ')';
        sel.options[i].text = t;
      }
    });
}

function scanNetworks() {
  document.getElementById('scan-results').style.display = 'block';
  document.getElementById('networks').innerHTML = 'Scanning\u2026';
  fetch('/api/wifiscan')
    .then(function(r){ return r.json(); })
    .then(function(data){
      var h = '';
      if (!data.networks || data.networks.length === 0) {
        h = '<div style="padding:8px;color:var(--text-muted)">No networks found.</div>';
      } else {
        data.networks.forEach(function(n){
          var q = n.rssi >= -50 ? 'Excellent' : n.rssi >= -60 ? 'Good' : n.rssi >= -70 ? 'Fair' : 'Weak';
          var safe = n.ssid.replace(/'/g, '');
          h += '<div style="cursor:pointer;margin-bottom:6px;padding:8px;border:1px solid var(--border-color);border-radius:6px" '
             + 'onclick="pickNetwork(\'' + safe + '\',\'' + n.encryption + '\')">'
             + '<div><strong>' + n.ssid + '</strong></div>'
             + '<div style="font-size:.8em;color:var(--text-muted);margin-top:2px">'
             + n.rssi + ' dBm (' + q + ') &bull; ' + n.encryption + '</div></div>';
        });
      }
      document.getElementById('networks').innerHTML = h;
    })
    .catch(function(){ document.getElementById('networks').innerHTML = '<div style="padding:8px">Scan failed.</div>'; });
}

function pickNetwork(ssid, enc) {
  if (enc === 'Open') {
    saveNetworkToSlot(ssid, '');
    return;
  }
  showModal(function(box, close){
    box.innerHTML = '<h4>Password for <b>' + ssid + '</b></h4>';
    var inp = document.createElement('input');
    inp.type = 'password'; inp.placeholder = 'WiFi password'; inp.style.width = '100%';
    box.appendChild(inp);
    var btns = document.createElement('div'); btns.className = 'modal-buttons';
    var ok = document.createElement('button'); ok.textContent = 'Next';
    ok.className = 'btn btn-success';
    ok.onclick = function(){ var pw = inp.value; close(); saveNetworkToSlot(ssid, pw); };
    var cancel = document.createElement('button'); cancel.textContent = 'Cancel';
    cancel.className = 'btn btn-secondary'; cancel.onclick = close;
    btns.appendChild(ok); btns.appendChild(cancel); box.appendChild(btns);
    setTimeout(function(){ inp.focus(); }, 80);
  });
}

function saveNetworkToSlot(ssid, password) {
  showModal(function(box, close){
    box.innerHTML = '<h4>Save <b>' + ssid + '</b> to slot:</h4>';
    var sel = document.createElement('select'); sel.style.width = '100%'; sel.style.marginBottom = '12px';
    var src = document.getElementById('slotSelector');
    for (var i = 0; i < 6; i++) sel.options.add(new Option(src.options[i].text, i));
    box.appendChild(sel);
    var btns = document.createElement('div'); btns.className = 'modal-buttons';
    var ok = document.createElement('button'); ok.textContent = 'Save';
    ok.className = 'btn btn-success';
    ok.onclick = function(){
      var slot = sel.value;
      close();
      fetch('/api/save-wifi-slot', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'slot=' + slot + '&label=' + encodeURIComponent(ssid) +
              '&ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)
      }).then(function(r){ return r.text(); })
        .then(function(msg){ showAlert(msg); loadSlot(parseInt(slot)); refreshSlotSelector(); });
    };
    var cancel = document.createElement('button'); cancel.textContent = 'Cancel';
    cancel.className = 'btn btn-secondary'; cancel.onclick = close;
    btns.appendChild(ok); btns.appendChild(cancel); box.appendChild(btns);
  });
}

function saveAP() {
  var ssid    = document.getElementById('apSsid').value;
  var pass    = document.getElementById('apPassword').value;
  var channel = document.getElementById('apChannel').value;
  var retries = document.getElementById('apRetries').value;
  if (!ssid || ssid.length < 3)   { showAlert('AP SSID must be at least 3 characters.'); return; }
  if (!pass || pass.length < 8)   { showAlert('AP password must be at least 8 characters.'); return; }
  if (channel < 1 || channel > 13){ showAlert('Channel must be 1\u201313.'); return; }
  showConfirm('Save AP settings and reboot?', function(){
    fetch('/api/save-wifi-ap', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass) +
            '&channel=' + channel + '&retries=' + retries
    }).then(function(r){ return r.text(); })
      .then(function(msg){
        showAlert(msg);
        fetch('/api/reboot', {method:'POST'});
        document.body.innerHTML = '<div style="display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:1.2em">Rebooting\u2026 page reloads in 12 s.</div>';
        setTimeout(function(){ location.reload(); }, 12000);
      });
  });
}

function resetAP() {
  showConfirm('Reset AP settings to defaults and reboot?', function(){
    fetch('/api/reset-wifi-ap', {method:'POST'})
      .then(function(r){ return r.text(); })
      .then(function(msg){
        showAlert(msg);
        fetch('/api/reboot', {method:'POST'});
        document.body.innerHTML = '<div style="display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:1.2em">Rebooting\u2026 page reloads in 12 s.</div>';
        setTimeout(function(){ location.reload(); }, 12000);
      });
  });
}

function saveAll() {
  var ssid    = document.getElementById('apSsid').value;
  var pass    = document.getElementById('apPassword').value;
  var channel = document.getElementById('apChannel').value;
  var retries = document.getElementById('apRetries').value;
  if (!ssid || ssid.length < 3)   { showAlert('AP SSID must be at least 3 characters.'); return; }
  if (!pass || pass.length < 8)   { showAlert('AP password must be at least 8 characters.'); return; }
  showConfirm('Save current slot + AP settings, then reboot?', function(){
    var slot  = document.getElementById('currentSlot').value;
    var label = document.getElementById('labelInput').value;
    var slotSsid = document.getElementById('ssidInput').value;
    var slotPass = document.getElementById('passwordInput').value;
    var post = function(url, body){
      return fetch(url, {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:body});
    };
    post('/api/save-wifi-slot',
         'slot=' + slot + '&label=' + encodeURIComponent(label) +
         '&ssid=' + encodeURIComponent(slotSsid) + '&password=' + encodeURIComponent(slotPass))
    .then(function(){
      return post('/api/save-wifi-ap',
        'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass) +
        '&channel=' + channel + '&retries=' + retries);
    })
    .then(function(){
      showAlert('All saved.');
      fetch('/api/reboot', {method:'POST'});
      document.body.innerHTML = '<div style="display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:1.2em">Rebooting\u2026 page reloads in 12 s.</div>';
      setTimeout(function(){ location.reload(); }, 12000);
    })
    .catch(function(e){ showAlert('Error: ' + e); });
  });
}
)JS";
  html += "</script>";
  html += "</div></body></html>";  // close .container
  return html;
}
