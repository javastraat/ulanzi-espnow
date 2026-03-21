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
(function init(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    document.getElementById('h1').textContent=d.hostname;
    document.getElementById('sub').textContent=d.ip;
  }).catch(function(){});
  loadCustomApps();
})();
</script>
</body></html>
)html";
