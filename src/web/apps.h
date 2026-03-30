#pragma once
#include "styles.h"
#include "navigation.h"

// ── Apps page (/apps): Custom App slots manager ──
static const char PAGE_APPS[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Apps</title>"
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

  <!-- Card 1: Custom Apps — rotation manager -->
  <div class="card">
    <h3>Custom Apps</h3>
    <div style="font-size:.78em;color:var(--text-muted);margin-bottom:10px;line-height:1.5">
      Apps run as a group after built-in screens: clock → temp → hum → bat → apps → …<br>
      Send via MQTT: <code style="font-size:.85em">{nodeId}/custom_app/{name}/set</code>
    </div>
    <div id="ca-list" style="margin-bottom:10px"></div>
    <div style="display:flex;gap:6px">
      <button class="bp" style="flex:0 0 auto;padding:5px 12px" onclick="loadCustomApps()">Refresh</button>
    </div>
  </div>

  <!-- Card 2: Send / Update App (JSON) -->
  <div class="card">
    <h3>Send / Update App (JSON)</h3>
    <div style="font-size:.78em;color:var(--text-muted);margin-bottom:10px;line-height:1.5">
      POST raw JSON to create or overwrite an app slot. All fields are optional except <code>name</code>.
    </div>
    <textarea id="ca-json" rows="5"
      style="width:100%;box-sizing:border-box;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:6px;font-size:.78em;font-family:monospace;resize:vertical"
      placeholder='{"name":"hello","text":"Hello World","color":"#00FF00","duration":10,"repeat":true}'></textarea>
    <div style="display:flex;gap:6px;margin-top:6px">
      <button class="bp" style="flex:1" onclick="sendCustomApp()">Send</button>
    </div>
  </div>

  <!-- Card 3: App Maker -->
  <div class="card">
    <h3>App Maker</h3>
    <div style="font-size:.78em;color:var(--text-muted);margin-bottom:12px;line-height:1.5">
      Fill in the fields below and click <strong>Save App</strong> to create or update a custom app slot.
    </div>
    <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px 12px;font-size:.82em">
      <div>
        <label style="display:block;margin-bottom:3px;color:var(--text-muted)">Name *</label>
        <input id="am-name" type="text" placeholder="my-app"
          style="width:100%;box-sizing:border-box;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 7px;font-size:1em">
      </div>
      <div>
        <label style="display:block;margin-bottom:3px;color:var(--text-muted)">Color</label>
        <div style="display:flex;align-items:center;gap:6px">
          <input id="am-color" type="color" value="#00FF00"
            style="width:38px;height:30px;border:1px solid var(--border-color);border-radius:4px;background:var(--bg-secondary);cursor:pointer;padding:1px">
          <input id="am-color-hex" type="text" value="#00FF00" maxlength="7"
            style="flex:1;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 7px;font-size:1em"
            oninput="syncColorFromHex()">
        </div>
      </div>
      <div style="grid-column:1/-1">
        <label style="display:block;margin-bottom:3px;color:var(--text-muted)">Text</label>
        <input id="am-text" type="text" placeholder="Hello World!"
          style="width:100%;box-sizing:border-box;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 7px;font-size:1em">
      </div>
      <div>
        <label style="display:block;margin-bottom:3px;color:var(--text-muted)">Duration (s)</label>
        <input id="am-duration" type="number" value="10" min="1" max="3600"
          style="width:100%;box-sizing:border-box;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 7px;font-size:1em">
      </div>
      <div>
        <label style="display:block;margin-bottom:3px;color:var(--text-muted)">Icon (optional)</label>
        <input id="am-icon" type="text" placeholder="/icons/home.gif or 55505"
          style="width:100%;box-sizing:border-box;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 7px;font-size:1em">
      </div>
      <div>
        <label style="display:block;margin-bottom:3px;color:var(--text-muted)">Push Icon</label>
        <select id="am-pushicon"
          style="width:100%;box-sizing:border-box;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 7px;font-size:1em">
          <option value="0">0 — Icon fixed, text scrolls</option>
          <option value="1">1 — Icon scrolls with text</option>
          <option value="2">2 — Icon shown once</option>
        </select>
      </div>
      <div>
        <label style="display:block;margin-bottom:3px;color:var(--text-muted)">Scroll Speed (ms, 0=default)</label>
        <input id="am-scrollspeed" type="number" value="0" min="0" max="9999"
          style="width:100%;box-sizing:border-box;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 7px;font-size:1em">
      </div>
      <div>
        <label style="display:block;margin-bottom:3px;color:var(--text-muted)">Lifetime (s, 0=never expire)</label>
        <input id="am-lifetime" type="number" value="0" min="0" max="86400"
          style="width:100%;box-sizing:border-box;background:var(--bg-secondary);color:var(--text-color);border:1px solid var(--border-color);border-radius:4px;padding:5px 7px;font-size:1em">
      </div>
      <div style="grid-column:1/-1;display:flex;flex-wrap:wrap;gap:16px;margin-top:2px">
        <div style="display:flex;align-items:center;gap:8px">
          <input id="am-repeat" type="checkbox" checked style="width:16px;height:16px;cursor:pointer">
          <label for="am-repeat" style="cursor:pointer;color:var(--text-muted)">Repeat (loop text scroll)</label>
        </div>
        <div style="display:flex;align-items:center;gap:8px">
          <input id="am-center" type="checkbox" style="width:16px;height:16px;cursor:pointer">
          <label for="am-center" style="cursor:pointer;color:var(--text-muted)">Center text (no scroll)</label>
        </div>
      </div>
    </div>
    <div style="margin-top:12px">
      <div style="font-size:.75em;color:var(--text-muted);margin-bottom:4px">Preview JSON</div>
      <div id="am-preview" style="background:var(--bg-secondary);border:1px solid var(--border-color);border-radius:4px;padding:6px;font-size:.75em;font-family:monospace;white-space:pre-wrap;word-break:break-all;color:var(--text-color)"></div>
    </div>
    <div style="display:flex;gap:6px;margin-top:8px">
      <button class="bp" style="flex:1" onclick="saveAppMaker()">Save App</button>
      <button class="bp" style="flex:0 0 auto;padding:5px 10px;background:#555" onclick="clearAppMaker()">Clear</button>
    </div>
    <div id="am-status" style="font-size:.78em;margin-top:6px;min-height:1.2em"></div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
function escHtml(s){return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');}
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
// App Maker helpers
function amBuild(){
  var o={};
  var name=document.getElementById('am-name').value.trim();
  if(name)o.name=name;
  var text=document.getElementById('am-text').value.trim();
  if(text)o.text=text;
  o.color=document.getElementById('am-color').value;
  var dur=parseInt(document.getElementById('am-duration').value,10);
  if(!isNaN(dur)&&dur>0)o.duration=dur;
  var icon=document.getElementById('am-icon').value.trim();
  if(icon)o.icon=icon;
  var pi=parseInt(document.getElementById('am-pushicon').value,10);
  if(pi>0)o.pushIcon=pi;
  var ss=parseInt(document.getElementById('am-scrollspeed').value,10);
  if(!isNaN(ss)&&ss>0)o.scrollSpeed=ss;
  var lt=parseInt(document.getElementById('am-lifetime').value,10);
  if(!isNaN(lt)&&lt>0)o.lifetime=lt;
  o.repeat=document.getElementById('am-repeat').checked;
  o.center=document.getElementById('am-center').checked;
  return o;
}
function amUpdatePreview(){
  var o=amBuild();
  document.getElementById('am-preview').textContent=JSON.stringify(o,null,2);
}
function syncColorFromHex(){
  var hex=document.getElementById('am-color-hex').value.trim();
  if(/^#[0-9a-fA-F]{6}$/.test(hex))document.getElementById('am-color').value=hex;
  amUpdatePreview();
}
(function wireAmInputs(){
  var ids=['am-name','am-text','am-duration','am-icon','am-repeat','am-center','am-scrollspeed','am-lifetime','am-pushicon'];
  ids.forEach(function(id){
    var el=document.getElementById(id);
    if(!el)return;
    if(el.type==='checkbox'||el.tagName==='SELECT')el.addEventListener('change',amUpdatePreview);
    else el.addEventListener('input',amUpdatePreview);
  });
  var cp=document.getElementById('am-color');
  if(cp){
    cp.addEventListener('input',function(){
      document.getElementById('am-color-hex').value=cp.value;
      amUpdatePreview();
    });
  }
})();
function saveAppMaker(){
  var o=amBuild();
  if(!o.name){alert('Name is required.');return;}
  var st=document.getElementById('am-status');
  st.style.color='var(--text-muted)';
  st.textContent='Saving…';
  fetch('/api/custom_apps',{method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(o)
  }).then(function(r){return r.json();}).then(function(d){
    if(d.ok){
      st.style.color='#4caf50';
      st.textContent='Saved "'+o.name+'" successfully.';
      loadCustomApps();
    } else {
      st.style.color='#f44';
      st.textContent='Error: '+(d.error||'unknown');
    }
  }).catch(function(){
    st.style.color='#f44';
    st.textContent='Request failed.';
  });
}
function clearAppMaker(){
  ['am-name','am-text','am-icon'].forEach(function(id){document.getElementById(id).value='';});
  document.getElementById('am-duration').value='10';
  document.getElementById('am-scrollspeed').value='0';
  document.getElementById('am-lifetime').value='0';
  document.getElementById('am-color').value='#00FF00';
  document.getElementById('am-color-hex').value='#00FF00';
  document.getElementById('am-pushicon').value='0';
  document.getElementById('am-repeat').checked=true;
  document.getElementById('am-center').checked=false;
  document.getElementById('am-status').textContent='';
  amUpdatePreview();
}
(function init(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    document.getElementById('h1').textContent=d.hostname;
    document.getElementById('sub').textContent=d.ip;
  }).catch(function(){});
  loadCustomApps();
  amUpdatePreview();
})();
</script>
</body></html>
)html";
