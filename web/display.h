#pragma once
#include "styles.h"
#include "navigation.h"

// ── Display page (/display): brightness, indicators, rotation, screensaver, icons, colors ──
static const char PAGE_DISPLAY[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Display</title>"
  "<style>" COMMON_CSS
  ".bp{background:#444;color:#fff;border:none;padding:5px 0;border-radius:4px;"
  "cursor:pointer;font-size:.82em;font-weight:bold;flex:1;min-width:52px;transition:background .15s}"
  ".bp:hover{background:#555}"
  "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <!-- Brightness -->
  <div class="card">
    <h3>Brightness</h3>
    <div class="bright-top">
      <label class="switch">
        <input type="checkbox" id="tog-auto" onchange="onAutoToggle()">
        <span class="slider"></span>
      </label>
      <span class="bright-lbl" id="tog-lbl">Auto</span>
    </div>
    <div class="bright-bot">
      <input type="range" id="sld-bright" min="0" max="255" value="50" disabled
             oninput="document.getElementById('bright-num').textContent=fmtBright(this.value);updatePresetButtons(this.value)"
             onchange="onSliderChange()">
      <span class="bright-num" id="bright-num">50</span>
    </div>
    <div id="bright-presets" style="display:flex;gap:6px;margin-top:8px;flex-wrap:wrap">
      <button id="bp-0"   onclick="applyPreset(0)"   class="bp">Off</button>
      <button id="bp-10"  onclick="applyPreset(10)"  class="bp">Night</button>
      <button id="bp-50"  onclick="applyPreset(50)"  class="bp">Dim</button>
      <button id="bp-120" onclick="applyPreset(120)" class="bp">Medium</button>
      <button id="bp-255" onclick="applyPreset(255)" class="bp">Bright</button>
    </div>
  </div>

  <!-- Indicators -->
  <div class="card">
    <h3>Indicators</h3>
    <div class="metric" style="margin-bottom:10px">
      <div>
        <span class="metric-label">Show status dots</span>
        <div style="font-size:.75em;color:var(--text-muted);margin-top:2px">
          3 pixels on the right edge: WiFi · POCSAG · MQTT
        </div>
      </div>
      <label class="switch" style="margin-left:12px;flex-shrink:0">
        <input type="checkbox" id="tog-ind" onchange="setIndicators(this.checked)">
        <span class="slider"></span>
      </label>
    </div>
    <div style="font-size:.78em;color:var(--text-muted);line-height:1.6">
      <div><span style="color:#00b400">&#9679;</span> WiFi connected &nbsp;/&nbsp; <span style="color:#c80000">&#9679; blink</span> disconnected</div>
      <div><span style="color:#ff6400">&#9679;</span> POCSAG received — 10 s flash</div>
      <div><span style="color:#00c8c8">&#9679;</span> MQTT connected &nbsp;/&nbsp; <span style="color:#c80000">&#9679;</span> disconnected</div>
    </div>
  </div>

  <!-- Rotate Screens -->
  <div class="card">
    <h3>Rotate Screens</h3>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
        <span class="metric-label">Rotate Screens</span>
        <label class="switch">
          <input type="checkbox" id="tog-rot" onchange="onRotateChange()">
          <span class="slider"></span>
        </label>
      </div>
      <div class="bright-bot">
        <span class="metric-label" style="white-space:nowrap;flex-shrink:0">Rotate Interval</span>
        <input type="range" id="sld-rot" min="1" max="60" value="5"
               oninput="document.getElementById('rot-num').textContent=this.value+'s'"
               onchange="onRotateChange()">
        <span class="bright-num" id="rot-num">5s</span>
      </div>
    </div>
    <div class="metric" style="border-bottom:none;padding-top:8px">
      <span class="metric-label" style="font-size:.8em;color:var(--text-muted)">clock &#8594; temp &#8594; humidity &#8594; battery &#8594; clock</span>
    </div>
  </div>

  <!-- Clock Face -->
  <div class="card">
    <h3>Clock Face</h3>
    <div style="display:flex;flex-direction:column;gap:6px" id="cf-list">
      <button id="cf-0" class="bp" onclick="setFace(0)" style="text-align:left;padding:7px 10px">
        <strong>0 — Calendar</strong>
        <span style="color:var(--text-muted);font-size:.8em;margin-left:6px">Rolodex + HH:MM + day dots</span>
      </button>
      <button id="cf-1" class="bp" onclick="setFace(1)" style="text-align:left;padding:7px 10px">
        <strong>1 — Classic</strong>
        <span style="color:var(--text-muted);font-size:.8em;margin-left:6px">HH:MM:SS centred</span>
      </button>
      <button id="cf-2" class="bp" onclick="setFace(2)" style="text-align:left;padding:7px 10px">
        <strong>2 — Weekday</strong>
        <span style="color:var(--text-muted);font-size:.8em;margin-left:6px">HH:MM:SS + day dots</span>
      </button>
      <button id="cf-3" class="bp" onclick="setFace(3)" style="text-align:left;padding:7px 10px">
        <strong>3 — Big</strong>
        <span style="color:var(--text-muted);font-size:.8em;margin-left:6px">HH:MM large digits</span>
      </button>
      <button id="cf-4" class="bp" onclick="setFace(4)" style="text-align:left;padding:7px 10px">
        <strong>4 — Big Animated</strong>
        <span style="color:var(--text-muted);font-size:.8em;margin-left:6px">HH:MM large digits with rainbow or /bigtime.gif</span>
      </button>
      <button id="cf-5" class="bp" onclick="setFace(5)" style="text-align:left;padding:7px 10px">
        <strong>5 — Binary</strong>
        <span style="color:var(--text-muted);font-size:.8em;margin-left:6px">H/M/S as bit rows</span>
      </button>
    </div>
  </div>

  <!-- Show in Rotation -->
  <div class="card">
    <h3>Show in Rotation</h3>
    <div style="font-size:.78em;color:var(--text-muted);margin-bottom:10px">
      Choose which screens are included when Rotate Screens is enabled.
    </div>
    <div style="display:flex;flex-direction:column;gap:8px">
      <div class="metric"><span class="metric-label">Clock</span>
        <label class="switch"><input type="checkbox" id="scr-0" onchange="saveScreens()"><span class="slider"></span></label>
      </div>
      <div class="metric"><span class="metric-label">Temperature</span>
        <label class="switch"><input type="checkbox" id="scr-1" onchange="saveScreens()"><span class="slider"></span></label>
      </div>
      <div class="metric"><span class="metric-label">Humidity</span>
        <label class="switch"><input type="checkbox" id="scr-2" onchange="saveScreens()"><span class="slider"></span></label>
      </div>
      <div class="metric" style="border-bottom:none"><span class="metric-label">Battery</span>
        <label class="switch"><input type="checkbox" id="scr-3" onchange="saveScreens()"><span class="slider"></span></label>
      </div>
    </div>
  </div>

  <!-- Screensaver -->
  <div class="card">
    <h3>Screensaver</h3>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
        <span class="metric-label">Enable</span>
        <label class="switch">
          <input type="checkbox" id="tog-ss" onchange="onSsChange()">
          <span class="slider"></span>
        </label>
      </div>
      <div class="bright-bot">
        <span class="metric-label" style="white-space:nowrap;flex-shrink:0">Timeout</span>
        <input type="number" id="ss-timeout" min="5" max="3600" value="60"
               style="width:64px;background:var(--bg-secondary);color:var(--text-color);
                      border:1px solid var(--border-color);border-radius:4px;
                      padding:3px 6px;font-size:.88em"
               onchange="onSsChange()">
        <span style="font-size:.82em;color:var(--text-muted)">sec</span>
      </div>
    </div>
    <div class="metric" style="border-bottom:1px solid var(--border-color);padding:8px 0">
      <span class="metric-label">GIF</span>
      <select id="ss-file" onchange="onSsChange()"
              style="flex:1;margin-left:8px;padding:4px 6px;background:var(--bg-secondary);
                     color:var(--text-color);border:1px solid var(--border-color);
                     border-radius:4px;font-size:.88em">
        <option value="">(none)</option>
      </select>
    </div>
    <div style="padding:8px 0;border-bottom:1px solid var(--border-color)">
      <div style="font-size:.82em;color:var(--text-muted);margin-bottom:6px">Brightness</div>
      <div style="display:flex;gap:6px;margin-bottom:6px">
        <button id="ss-bp-a"  onclick="setSsBright(-2)" class="bp">Auto</button>
        <button id="ss-bp-0"  onclick="setSsBright(0)"  class="bp">Off</button>
        <button id="ss-bp-10" onclick="setSsBright(10)" class="bp">Night</button>
      </div>
      <div style="display:flex;gap:6px">
        <button id="ss-bp-50"  onclick="setSsBright(50)"  class="bp">Dim</button>
        <button id="ss-bp-120" onclick="setSsBright(120)" class="bp">Medium</button>
        <button id="ss-bp-255" onclick="setSsBright(255)" class="bp">Bright</button>
      </div>
    </div>
    <div style="font-size:.75em;color:var(--text-muted);padding:6px 0 8px">
      GIF must be exactly 32&#xd7;8 px. Place in /screensaver/ (visible in Files page).
    </div>
    <div id="ss-no-files" style="display:none;font-size:.82em;padding:4px 0 8px">
      No GIFs found in /screensaver/ &mdash; <a href="/files" style="color:#00bcd4">go to Files</a> to upload.
    </div>
    <div style="display:flex;align-items:center;justify-content:flex-end">
      <button id="btn-ss-test" onclick="testSs()"
              style="background:#444;color:#fff;border:none;padding:6px 18px;
                     border-radius:4px;cursor:pointer;font-weight:bold;font-size:.88em">
        Test
      </button>
    </div>
  </div>

  <!-- Text Colors -->
  <div class="card">
    <h3>Text Colors</h3>
    <div class="metric">
      <span class="metric-label" style="flex-shrink:0;width:72px">Clock</span>
      <input type="color" id="col-clock" value="#FFFFFF" onchange="saveColors()">
    </div>
    <div class="metric" style="border-bottom:none">
      <span class="metric-label" style="flex-shrink:0;width:72px">POCSAG</span>
      <input type="color" id="col-poc" value="#FFA000" onchange="saveColors()">
    </div>
  </div>

  <!-- Thresholds & Colors -->
  <div class="card">
    <h3>Thresholds &amp; Colors</h3>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="font-size:.82em;font-weight:bold;margin-bottom:6px">Temperature (&deg;C)</div>
      <div style="display:flex;align-items:center;gap:6px;margin-bottom:6px;flex-wrap:wrap">
        <span class="metric-label" style="flex-shrink:0;width:72px">Thresholds</span>
        <input type="number" id="t-thr-lo" min="-40" max="60" step="0.5"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
        <span style="color:var(--text-muted)">&#8211;</span>
        <input type="number" id="t-thr-hi" min="-40" max="60" step="0.5"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
      </div>
      <div style="margin-top:4px">
        <div style="font-size:.75em;color:var(--text-muted);margin-bottom:3px">Colors</div>
        <div style="display:flex;align-items:center;gap:8px">
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Low<input type="color" id="t-col-lo" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Mid<input type="color" id="t-col-mid" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">High<input type="color" id="t-col-hi" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
        </div>
      </div>
    </div>
    <div style="padding:6px 0;border-bottom:1px solid var(--border-color)">
      <div style="font-size:.82em;font-weight:bold;margin-bottom:6px">Humidity (%)</div>
      <div style="display:flex;align-items:center;gap:6px;margin-bottom:6px;flex-wrap:wrap">
        <span class="metric-label" style="flex-shrink:0;width:72px">Thresholds</span>
        <input type="number" id="h-thr-lo" min="0" max="100" step="1"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
        <span style="color:var(--text-muted)">&#8211;</span>
        <input type="number" id="h-thr-hi" min="0" max="100" step="1"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
      </div>
      <div style="margin-top:4px">
        <div style="font-size:.75em;color:var(--text-muted);margin-bottom:3px">Colors</div>
        <div style="display:flex;align-items:center;gap:8px">
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Low<input type="color" id="h-col-lo" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Mid<input type="color" id="h-col-mid" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">High<input type="color" id="h-col-hi" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
        </div>
      </div>
    </div>
    <div style="padding:6px 0">
      <div style="font-size:.82em;font-weight:bold;margin-bottom:6px">Battery (%)</div>
      <div style="display:flex;align-items:center;gap:6px;margin-bottom:6px;flex-wrap:wrap">
        <span class="metric-label" style="flex-shrink:0;width:72px">Thresholds</span>
        <input type="number" id="b-thr-lo" min="0" max="100" step="1"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
        <span style="color:var(--text-muted)">&#8211;</span>
        <input type="number" id="b-thr-hi" min="0" max="100" step="1"
               style="width:52px;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:3px 6px;font-size:.88em"
               onchange="saveThresholds()">
      </div>
      <div style="margin-top:4px">
        <div style="font-size:.75em;color:var(--text-muted);margin-bottom:3px">Colors</div>
        <div style="display:flex;align-items:center;gap:8px">
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Low<input type="color" id="b-col-lo" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">Mid<input type="color" id="b-col-mid" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
          <label style="display:flex;align-items:center;gap:3px;font-size:.75em;color:var(--text-muted)">High<input type="color" id="b-col-hi" onchange="saveThresholds()" style="width:30px;height:22px;padding:1px;border:none;cursor:pointer;border-radius:2px"></label>
        </div>
      </div>
    </div>
  </div>

  <!-- Transitions -->
  <div class="card">
    <h3>Transitions</h3>
    <div style="font-size:.78em;color:var(--text-muted);margin-bottom:10px">
      Animated effect played between screens during auto-rotation.
    </div>
    <div style="margin-bottom:10px">
      <div style="font-size:.82em;font-weight:bold;margin-bottom:6px">Effect</div>
      <div style="display:flex;flex-wrap:wrap;gap:6px" id="trans-btns"></div>
    </div>
    <div>
      <div style="font-size:.82em;font-weight:bold;margin-bottom:6px">
        Speed: <span id="trans-speed-lbl">400</span> ms
      </div>
      <input type="range" id="trans-speed" min="100" max="1200" step="50" value="400"
        style="width:100%"
        oninput="document.getElementById('trans-speed-lbl').textContent=this.value"
        onchange="saveTransition()">
    </div>
  </div>

  <!-- Custom Apps — rotation manager -->
  <div class="card">
    <h3>Custom Apps</h3>
    <div style="font-size:.78em;color:var(--text-muted);margin-bottom:10px;line-height:1.5">
      Apps run as a group after built-in screens: clock → temp → hum → bat → apps → …<br>
      Send via MQTT: <code style="font-size:.85em">{nodeId}/custom_app/{name}/set</code>
    </div>
    <div id="ca-list" style="margin-bottom:10px"></div>
    <div style="display:flex;gap:6px;margin-bottom:12px">
      <button class="bp" style="flex:0 0 auto;padding:5px 12px" onclick="loadCustomApps()">Refresh</button>
    </div>
    <div>
      <div style="font-size:.82em;font-weight:bold;margin-bottom:6px">Send / Update App (JSON)</div>
      <textarea id="ca-json" rows="4"
        style="width:100%;box-sizing:border-box;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:6px;font-size:.78em;font-family:monospace;resize:vertical"
        placeholder='{"name":"hello","text":"Hello World","color":"#00FF00","duration":10,"repeat":true}'></textarea>
      <div style="display:flex;gap:6px;margin-top:6px">
        <button class="bp" style="flex:1" onclick="sendCustomApp()">Send</button>
      </div>
    </div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
var PRESETS=[0,10,50,120,255];
function fmtBright(v){return parseInt(v)===0?'Off':v;}
function updatePresetButtons(val){
  val=parseInt(val);
  PRESETS.forEach(function(v){
    var b=document.getElementById('bp-'+v);
    if(!b)return;
    var active=(val===v);
    b.style.background=active?'#00bcd4':'#444';
    b.style.color=active?'#000':'#fff';
  });
}
function applyPreset(val){
  var tog=document.getElementById('tog-auto');
  if(tog.checked){
    tog.checked=false;
    document.getElementById('sld-bright').disabled=false;
    document.getElementById('tog-lbl').textContent='Manual';
    stopAutoPoller();
  }
  var sld=document.getElementById('sld-bright');
  sld.value=val;
  document.getElementById('bright-num').textContent=fmtBright(val);
  updatePresetButtons(val);
  postBright(false,val);
}
function postBright(isAuto,level){
  fetch('/api/brightness',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'auto='+(isAuto?1:0)+'&level='+level
  }).catch(function(){});
}
var _autoPoller=null;
function startAutoPoller(){
  if(_autoPoller)return;
  _autoPoller=setInterval(function(){
    fetch('/api/status').then(function(r){return r.json();}).then(function(d){
      if(!document.getElementById('tog-auto').checked){stopAutoPoller();return;}
      document.getElementById('sld-bright').value=d.brightness;
      document.getElementById('bright-num').textContent=fmtBright(d.brightness);
      updatePresetButtons(d.brightness);
    }).catch(function(){});
  },800);
}
function stopAutoPoller(){
  if(_autoPoller){clearInterval(_autoPoller);_autoPoller=null;}
}
function onAutoToggle(){
  var isAuto=document.getElementById('tog-auto').checked;
  document.getElementById('sld-bright').disabled=isAuto;
  document.getElementById('tog-lbl').textContent=isAuto?'Auto':'Manual';
  postBright(isAuto,document.getElementById('sld-bright').value);
  if(isAuto){startAutoPoller();}else{stopAutoPoller();}
}
function onSliderChange(){
  var v=document.getElementById('sld-bright').value;
  document.getElementById('bright-num').textContent=fmtBright(v);
  updatePresetButtons(v);
  postBright(false,v);
}
function onRotateChange(){
  fetch('/api/rotate',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'enabled='+(document.getElementById('tog-rot').checked?1:0)
        +'&interval='+document.getElementById('sld-rot').value
  }).catch(function(){});
}
function onSsChange(){
  fetch('/api/screensaver',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'enabled='+(document.getElementById('tog-ss').checked?1:0)
        +'&timeout='+document.getElementById('ss-timeout').value
        +'&file='+encodeURIComponent(document.getElementById('ss-file').value)
  }).catch(function(){});
}
var _ssBright=-1;
var SS_BP_IDS={'-2':'ss-bp-a','0':'ss-bp-0','10':'ss-bp-10','50':'ss-bp-50','120':'ss-bp-120','255':'ss-bp-255'};
function updateSsBrightButtons(val){
  _ssBright=val;
  Object.keys(SS_BP_IDS).forEach(function(k){
    var b=document.getElementById(SS_BP_IDS[k]);
    if(!b)return;
    var active=(parseInt(k)===val);
    b.style.background=active?'#00bcd4':'#444';
    b.style.color=active?'#000':'#fff';
  });
}
function setSsBright(val){
  updateSsBrightButtons(val);
  fetch('/api/screensaver',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'brightness='+val
  }).catch(function(){});
}
function testSs(){
  var btn=document.getElementById('btn-ss-test');
  var isTesting=(btn.textContent.trim()==='Test');
  fetch('/api/screensaver/test',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'action='+(isTesting?'test':'stop')
  }).then(function(){
    btn.textContent=isTesting?'Stop':'Test';
    btn.style.background=isTesting?'#dc3545':'#444';
  }).catch(function(){});
}
function saveColors(){
  fetch('/api/colors',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'clock='+encodeURIComponent(document.getElementById('col-clock').value)
        +'&poc='+encodeURIComponent(document.getElementById('col-poc').value)
  }).catch(function(){});
}
function saveThresholds(){
  fetch('/api/colors',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'tmp_thr_lo='+document.getElementById('t-thr-lo').value
        +'&tmp_thr_hi='+document.getElementById('t-thr-hi').value
        +'&t_lo='+encodeURIComponent(document.getElementById('t-col-lo').value)
        +'&t_mid='+encodeURIComponent(document.getElementById('t-col-mid').value)
        +'&t_hi='+encodeURIComponent(document.getElementById('t-col-hi').value)
        +'&hum_thr_lo='+document.getElementById('h-thr-lo').value
        +'&hum_thr_hi='+document.getElementById('h-thr-hi').value
        +'&h_lo='+encodeURIComponent(document.getElementById('h-col-lo').value)
        +'&h_mid='+encodeURIComponent(document.getElementById('h-col-mid').value)
        +'&h_hi='+encodeURIComponent(document.getElementById('h-col-hi').value)
        +'&bat_thr_lo='+document.getElementById('b-thr-lo').value
        +'&bat_thr_hi='+document.getElementById('b-thr-hi').value
        +'&b_lo='+encodeURIComponent(document.getElementById('b-col-lo').value)
        +'&b_mid='+encodeURIComponent(document.getElementById('b-col-mid').value)
        +'&b_hi='+encodeURIComponent(document.getElementById('b-col-hi').value)
  }).catch(function(){});
}
function setIndicators(val){
  fetch('/api/indicators',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'enabled='+(val?1:0)
  }).catch(function(){});
}
function saveScreens(){
  var mask=0;
  for(var i=0;i<4;i++){
    var cb=document.getElementById('scr-'+i);
    if(cb&&cb.checked)mask|=(1<<i);
  }
  fetch('/api/screens',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'screens='+mask
  }).catch(function(){});
}
var TRANS_EFFECTS=['None','Fade','Wipe','Slide','Blink','Reload','Curtain','Ripple','Crossfade','Random'];
var _curTrans=1;
function buildTransButtons(cur){
  _curTrans=cur;
  var c=document.getElementById('trans-btns');
  if(!c)return;
  var html='';
  TRANS_EFFECTS.forEach(function(name,i){
    var active=(i===cur);
    html+='<button class="bp" id="tb-'+i+'" onclick="setTrans('+i+')" style="padding:5px 10px;background:'+(active?'#00bcd4':'#444')+';color:'+(active?'#000':'#fff')+'">'+name+'</button>';
  });
  c.innerHTML=html;
}
function setTrans(idx){
  _curTrans=idx;
  buildTransButtons(idx);
  saveTransition();
}
function saveTransition(){
  var spd=document.getElementById('trans-speed').value;
  fetch('/api/transition',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'effect='+_curTrans+'&speed='+spd
  }).catch(function(){});
}
function loadTransition(){
  fetch('/api/transition').then(function(r){return r.json();}).then(function(d){
    buildTransButtons(d.effect||0);
    var sl=document.getElementById('trans-speed');
    if(sl){sl.value=d.speed||400;document.getElementById('trans-speed-lbl').textContent=sl.value;}
  }).catch(function(){buildTransButtons(1);});
}
function loadCustomApps(){
  fetch('/api/custom_apps').then(function(r){return r.json();}).then(function(apps){
    var el=document.getElementById('ca-list');
    if(!apps||apps.length===0){el.innerHTML='<div style="font-size:.78em;color:var(--text-muted)">No apps defined.</div>';return;}
    var html='<table style="width:100%;border-collapse:collapse;font-size:.78em">';
    html+='<tr>';
    html+='<th style="text-align:left;padding:3px 4px;color:var(--text-muted)">Active</th>';
    html+='<th style="text-align:left;padding:3px 4px;color:var(--text-muted)">Name</th>';
    html+='<th style="text-align:left;padding:3px 4px;color:var(--text-muted)">Text</th>';
    html+='<th style="text-align:left;padding:3px 4px;color:var(--text-muted)">Dur</th>';
    html+='<th style="padding:3px 4px"></th></tr>';
    apps.forEach(function(a){
      var swatch='<span style="display:inline-block;width:10px;height:10px;border-radius:2px;background:'+a.color+';border:1px solid #666;vertical-align:middle;margin-right:3px"></span>';
      var rem=a.lifetime_rem>0?'<span style="color:#f90;margin-left:3px">('+a.lifetime_rem+'s)</span>':'';
      var show=(a.show!==false);
      // Toggle switch
      var tid='tog-ca-'+a.name.replace(/[^a-z0-9]/gi,'_');
      var tog='<label style="display:inline-flex;align-items:center;cursor:pointer;gap:4px">'
        +'<input type="checkbox" id="'+tid+'" '+(show?'checked':'')+' onchange="toggleCustomApp(\''+escHtml(a.name)+'\')" style="display:none">'
        +'<span style="display:inline-block;width:34px;height:18px;border-radius:9px;background:'+(show?'#00bcd4':'#555')+';position:relative;transition:background .2s" id="'+tid+'-sw">'
        +'<span style="display:inline-block;width:14px;height:14px;border-radius:50%;background:#fff;position:absolute;top:2px;left:'+(show?'18px':'2px')+';transition:left .2s"></span>'
        +'</span>'
        +'</label>';
      html+='<tr style="border-top:1px solid var(--border-color);'+(show?'':'opacity:.5')+'" id="row-ca-'+a.name.replace(/[^a-z0-9]/gi,'_')+'">';
      html+='<td style="padding:4px">'+tog+'</td>';
      html+='<td style="padding:4px">'+swatch+escHtml(a.name)+'</td>';
      html+='<td style="padding:4px;max-width:100px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">'+escHtml(a.text)+rem+'</td>';
      html+='<td style="padding:4px">'+(a.duration||'auto')+'s</td>';
      html+='<td style="padding:4px;text-align:right"><button class="bp" style="padding:3px 8px;background:#dc3545" onclick="deleteCustomApp(\''+escHtml(a.name)+'\')">&#x2715;</button></td>';
      html+='</tr>';
    });
    html+='</table>';
    el.innerHTML=html;
  }).catch(function(){document.getElementById('ca-list').innerHTML='<div style="font-size:.78em;color:#f44">Error loading apps.</div>';});
}
function escHtml(s){return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');}
function toggleCustomApp(name){
  fetch('/api/custom_apps/toggle?name='+encodeURIComponent(name),{method:'POST'})
    .then(function(){loadCustomApps();}).catch(function(){});
}
function deleteCustomApp(name){
  if(!confirm('Delete app "'+name+'"?'))return;
  fetch('/api/custom_apps?name='+encodeURIComponent(name),{method:'DELETE'})
    .then(function(){loadCustomApps();}).catch(function(){});
}
function sendCustomApp(){
  var json=document.getElementById('ca-json').value.trim();
  if(!json){alert('Enter a JSON payload.');return;}
  fetch('/api/custom_apps',{method:'POST',
    headers:{'Content-Type':'application/json'},
    body:json
  }).then(function(r){return r.json();}).then(function(d){
    if(d.ok){document.getElementById('ca-json').value='';loadCustomApps();}
    else alert('Error: '+(d.error||'unknown'));
  }).catch(function(){alert('Request failed.');});
}
var _curFace=-1;
function updateFaceButtons(f){
  _curFace=f;
  for(var i=0;i<6;i++){
    var b=document.getElementById('cf-'+i);
    if(!b)continue;
    b.style.background=(i===f)?'#00bcd4':'#444';
    b.style.color=(i===f)?'#000':'#fff';
  }
}
function setFace(f){
  updateFaceButtons(f);
  fetch('/api/clockface',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'face='+f
  }).catch(function(){});
}
(function init(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    document.getElementById('h1').textContent=d.hostname;
    document.getElementById('sub').textContent=d.ip;
    document.getElementById('tog-auto').checked=d.auto_brightness;
    document.getElementById('sld-bright').disabled=d.auto_brightness;
    document.getElementById('tog-lbl').textContent=d.auto_brightness?'Auto':'Manual';
    document.getElementById('sld-bright').value=d.brightness;
    document.getElementById('bright-num').textContent=fmtBright(d.brightness);
    updatePresetButtons(d.brightness);
    if(d.auto_brightness)startAutoPoller();
    var ri=d.rotate_interval||5;
    document.getElementById('tog-rot').checked=d.rotate_enabled;
    document.getElementById('sld-rot').value=ri;
    document.getElementById('rot-num').textContent=ri+'s';
  }).catch(function(){});
  fetch('/api/indicators').then(function(r){return r.json();}).then(function(d){
    document.getElementById('tog-ind').checked=d.enabled;
  }).catch(function(){});
  fetch('/api/colors').then(function(r){return r.json();}).then(function(d){
    if(d.clock)document.getElementById('col-clock').value=d.clock;
    if(d.poc)document.getElementById('col-poc').value=d.poc;
    if(d.tmp_thr_lo!==undefined)document.getElementById('t-thr-lo').value=d.tmp_thr_lo;
    if(d.tmp_thr_hi!==undefined)document.getElementById('t-thr-hi').value=d.tmp_thr_hi;
    if(d.t_lo)document.getElementById('t-col-lo').value=d.t_lo;
    if(d.t_mid)document.getElementById('t-col-mid').value=d.t_mid;
    if(d.t_hi)document.getElementById('t-col-hi').value=d.t_hi;
    if(d.hum_thr_lo!==undefined)document.getElementById('h-thr-lo').value=d.hum_thr_lo;
    if(d.hum_thr_hi!==undefined)document.getElementById('h-thr-hi').value=d.hum_thr_hi;
    if(d.h_lo)document.getElementById('h-col-lo').value=d.h_lo;
    if(d.h_mid)document.getElementById('h-col-mid').value=d.h_mid;
    if(d.h_hi)document.getElementById('h-col-hi').value=d.h_hi;
    if(d.bat_thr_lo!==undefined)document.getElementById('b-thr-lo').value=d.bat_thr_lo;
    if(d.bat_thr_hi!==undefined)document.getElementById('b-thr-hi').value=d.bat_thr_hi;
    if(d.b_lo)document.getElementById('b-col-lo').value=d.b_lo;
    if(d.b_mid)document.getElementById('b-col-mid').value=d.b_mid;
    if(d.b_hi)document.getElementById('b-col-hi').value=d.b_hi;
  }).catch(function(){});
  fetch('/api/clockface').then(function(r){return r.json();}).then(function(d){
    updateFaceButtons(d.face||0);
  }).catch(function(){});
  fetch('/api/screens').then(function(r){return r.json();}).then(function(d){
    var mask=d.screens!=null?d.screens:0x0F;
    for(var i=0;i<4;i++){
      var cb=document.getElementById('scr-'+i);
      if(cb)cb.checked=!!(mask&(1<<i));
    }
  }).catch(function(){});
  loadTransition();
  loadCustomApps();
  fetch('/api/screensaver').then(function(r){return r.json();}).then(function(d){
    document.getElementById('tog-ss').checked=d.enabled;
    document.getElementById('ss-timeout').value=d.timeout||60;
    updateSsBrightButtons(d.brightness!=null?d.brightness:-1);
    fetch('/api/fs/ls?path=/screensaver').then(function(r){return r.json();}).then(function(ls){
      var files=(ls.entries||[]).filter(function(e){return !e.isDir&&/\.gif$/i.test(e.name);});
      if(files.length===0){document.getElementById('ss-no-files').style.display='block';}
      var sel=document.getElementById('ss-file');
      files.forEach(function(e){
        var opt=document.createElement('option');
        opt.value=e.path;opt.textContent=e.name;
        if(e.path===d.file)opt.selected=true;
        sel.appendChild(opt);
      });
      if(d.file&&!sel.value)sel.value='';
    }).catch(function(){});
  }).catch(function(){});
})();
</script>
</body></html>
)html";
