#pragma once
#include "styles.h"
#include "navigation.h"

// ── Backup page (/backup): config export/import and LittleFS snapshots ──
static const char PAGE_BACKUP[] PROGMEM =
  "<!DOCTYPE html><html lang=\"en\">"
  "<head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Ulanzi Backup</title>"
  "<style>" COMMON_CSS "</style>"
  THEME_INIT_SCRIPT
  "</head><body>"
  NAV_BAR
  NAV_LIVE_MODAL
  R"html(
<div class="container"><div class="grid">

  <!-- Config export / import -->
  <div class="card">
    <h3>Configuration</h3>
    <div style="font-size:.82em;color:var(--text-muted);margin-bottom:12px">
      Export all settings to a text file or restore from a previously exported file.
      WiFi credentials are included — keep the file safe.
    </div>
    <div style="display:flex;flex-direction:column;gap:8px">
      <button onclick="exportConfig()"
              style="background:#00bcd4;color:#000;border:none;padding:8px 18px;
                     border-radius:4px;cursor:pointer;font-weight:bold;font-size:.9em">
        &#8595; Export Config
      </button>
      <button onclick="document.getElementById('imp-file').click()"
              style="background:#444;color:#fff;border:none;padding:8px 18px;
                     border-radius:4px;cursor:pointer;font-weight:bold;font-size:.9em">
        &#8593; Import Config
      </button>
      <input type="file" id="imp-file" accept=".txt,.bak" style="display:none"
             onchange="importConfig(this)">
    </div>
    <div id="cfg-status" style="margin-top:10px;font-size:.82em;min-height:1em"></div>
  </div>

  <!-- Snapshots -->
  <div class="card">
    <h3>Snapshots</h3>
    <div style="font-size:.82em;color:var(--text-muted);margin-bottom:10px">
      Save named snapshots to LittleFS and restore them at any time.
    </div>
    <div style="display:flex;gap:6px;margin-bottom:12px">
      <input type="text" id="snap-name" placeholder="my-backup" maxlength="32"
             style="flex:1;padding:5px 8px;background:var(--bg-secondary);
                    color:var(--text-color);border:1px solid var(--border-color);
                    border-radius:4px;font-size:.88em">
      <button onclick="saveSnapshot()"
              style="background:#28a745;color:#fff;border:none;padding:5px 14px;
                     border-radius:4px;cursor:pointer;font-weight:bold;font-size:.88em;flex-shrink:0">
        Save
      </button>
    </div>
    <div id="snap-list"><span style="color:#888;font-size:.85em">Loading...</span></div>
    <div id="snap-status" style="margin-top:8px;font-size:.82em;min-height:1em"></div>
  </div>

</div></div>
)html"
  "<script>" COMMON_JS NAV_LIVE_JS "</script>"
  R"html(
<script>
function setStatus(id, msg, ok) {
  var el = document.getElementById(id);
  el.textContent = msg;
  el.style.color = ok ? '#28a745' : '#dc3545';
  if (ok) setTimeout(function(){ el.textContent=''; }, 3000);
}
function exportConfig() {
  fetch('/api/backup/export')
    .then(function(r) {
      var cd = r.headers.get('content-disposition') || '';
      var m  = cd.match(/filename="([^"]+)"/);
      var fn = m ? m[1] : 'ulanzi-backup.txt';
      return r.blob().then(function(b){ return {blob:b, fn:fn}; });
    })
    .then(function(d) {
      var a = document.createElement('a');
      a.href = URL.createObjectURL(d.blob);
      a.download = d.fn;
      a.click();
      URL.revokeObjectURL(a.href);
    })
    .catch(function(){ setStatus('cfg-status','Export failed',false); });
}
function importConfig(inp) {
  var file = inp.files[0]; if (!file) return;
  showConfirm('Import "'+file.name+'"?\n\nThis overwrites ALL current settings and reboots the device.', function() {
    var reader = new FileReader();
    reader.onload = function(e) {
      fetch('/api/backup/import', {
        method:'POST',
        headers:{'Content-Type':'text/plain'},
        body: e.target.result
      }).then(function(r){ return r.json(); }).then(function(d) {
        if (d.ok) {
          document.body.innerHTML = '<div style="display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:1.2em">Settings restored — rebooting...</div>';
          setTimeout(function(){ location.href='/'; }, 8000);
        } else {
          setStatus('cfg-status', 'Import failed: '+(d.error||'unknown'), false);
        }
      });
    };
    reader.readAsText(file);
  });
  inp.value = '';
}
function loadSnapshotList() {
  fetch('/api/backup/snapshots').then(function(r){ return r.json(); }).then(function(d) {
    var el = document.getElementById('snap-list');
    if (!d.length) {
      el.innerHTML = '<span style="color:#888;font-size:.85em">No snapshots yet</span>';
      return;
    }
    var html = '<table style="width:100%;border-collapse:collapse;font-size:.85em">';
    d.forEach(function(f) {
      var kb = (f.size/1024).toFixed(1);
      html += '<tr style="border-bottom:1px solid var(--border-color)">'
            + '<td style="padding:4px 6px 4px 0;word-break:break-all">'+f.name+'</td>'
            + '<td style="padding:4px 6px;color:var(--text-muted);white-space:nowrap">'+kb+' KB</td>'
            + '<td style="padding:4px 0;text-align:right;white-space:nowrap">'
            + '<button onclick="downloadSnapshot(\''+f.name+'\')" style="background:#444;color:#fff;border:none;padding:2px 8px;border-radius:3px;cursor:pointer;font-size:.8em;margin-right:4px">DL</button>'
            + '<button onclick="loadSnapshot(\''+f.name+'\')"  style="background:#00bcd4;color:#000;border:none;padding:2px 8px;border-radius:3px;cursor:pointer;font-size:.8em;font-weight:bold;margin-right:4px">Load</button>'
            + '<button onclick="deleteSnapshot(\''+f.name+'\')" style="background:#dc3545;color:#fff;border:none;padding:2px 8px;border-radius:3px;cursor:pointer;font-size:.8em">Del</button>'
            + '</td></tr>';
    });
    el.innerHTML = html + '</table>';
  }).catch(function(){ document.getElementById('snap-list').innerHTML='<span style="color:#dc3545;font-size:.85em">Error loading snapshots</span>'; });
}
function saveSnapshot() {
  var name = document.getElementById('snap-name').value.trim();
  if (!name) { setStatus('snap-status','Enter a name first',false); return; }
  fetch('/api/backup/snapshot/save', {
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'name='+encodeURIComponent(name)
  }).then(function(r){ return r.json(); }).then(function(d) {
    if (d.ok) { setStatus('snap-status','Saved: '+name,true); loadSnapshotList(); document.getElementById('snap-name').value=''; }
    else       setStatus('snap-status','Save failed: '+(d.error||'unknown'),false);
  }).catch(function(){ setStatus('snap-status','Save failed',false); });
}
function downloadSnapshot(name) {
  var a = document.createElement('a');
  a.href = '/api/backup/snapshot/download?name='+encodeURIComponent(name);
  a.download = name+'.txt';
  a.click();
}
function loadSnapshot(name) {
  showConfirm('Load snapshot "'+name+'"?\n\nThis overwrites ALL current settings and reboots the device.', function() {
    fetch('/api/backup/snapshot/load', {
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:'name='+encodeURIComponent(name)
    }).then(function(r){ return r.json(); }).then(function(d) {
      if (d.ok) {
        document.body.innerHTML = '<div style="display:flex;justify-content:center;align-items:center;height:100vh;font-family:sans-serif;font-size:1.2em">Snapshot "'+name+'" restored — rebooting...</div>';
        setTimeout(function(){ location.href='/'; }, 8000);
      } else {
        setStatus('snap-status','Load failed: '+(d.error||'unknown'),false);
      }
    });
  });
}
function deleteSnapshot(name) {
  showConfirm('Delete snapshot "'+name+'"?', function() {
    fetch('/api/backup/snapshot/delete', {
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:'name='+encodeURIComponent(name)
    }).then(function(r){ return r.json(); }).then(function(d) {
      if (d.ok) { setStatus('snap-status','Deleted: '+name,true); loadSnapshotList(); }
      else       setStatus('snap-status','Delete failed',false);
    });
  });
}
loadSnapshotList();
</script>
</body></html>
)html";
