#pragma once

// ============================================================
// WebUI — Dashboard HTML
// ============================================================
static const char HTML_PAGE[] = R"RELAY_HTML(
<!DOCTYPE html>
<html><head>
<meta charset=UTF-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>CCM Relay Node</title>
<style>
body{font-family:sans-serif;margin:16px;background:#0f1011;color:#f7f8f8}
h2{color:#5e6ad2;margin:0 0 10px}h3{color:#d0d6e0;margin:6px 0}
table{border-collapse:collapse;width:100%;margin:6px 0}
th,td{border:1px solid #2e2e2e;padding:5px 8px}
th{background:#191a1b;color:#d0d6e0}
.on{color:#66bb6a;font-weight:bold}.off{color:#ef5350}
.bon{background:#43a047;color:#fff;border:none;padding:4px 8px;border-radius:3px;cursor:pointer}
.bof{background:#e53935;color:#fff;border:none;padding:4px 8px;border-radius:3px;cursor:pointer}
.sec{background:#191a1b;border-radius:6px;padding:12px;margin:8px 0}
input[type=number]{width:55px;padding:3px;background:#1a1a1f;color:#eee;border:1px solid #3e3e44;border-radius:3px}
a{color:#d0d6e0}
.ccm{color:#ffa726;font-size:0.85em}
@media(max-width:600px){table{display:block;overflow-x:auto;-webkit-overflow-scrolling:touch}body{margin:8px}}
</style>
</head><body>
<h2>CCM Relay Node (UECS)</h2>
<div class=sec id=sys></div>
<div class=sec id=net></div>
<div class=sec id=devstat></div>
<div class=sec>
<h3>Relay / CCM Mapping</h3>
<table><tr><th>CH</th><th>CCM Type</th><th>Room</th><th>State</th><th>Control</th></tr>
<tbody id=rtbl></tbody></table>
</div>
<div class=sec>
<h3>Digital Input</h3>
<table><tr><th>CH</th><th>State</th></tr>
<tbody id=dtbl></tbody></table>
</div>
<div class=sec id=sens></div>
<div class=sec id=gh></div>
<div class=sec id=irri></div>
<div class=sec id=prot></div>
<script>
function relay(ch,v){
  fetch('/api/relay/'+ch,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({value:v})}).then(load);
}
function load(){
  fetch('/api/state').then(function(r){return r.json();}).then(function(d){
    document.getElementById('sys').innerHTML=
      '<b>Node:</b> '+d.node_id+' | <b>FW:</b> '+d.version+
      ' | <b>Protocol:</b> <span style="color:#ffa726">UECS-CCM</span>'+
      ' | <b>Uptime:</b> '+d.uptime+'s'+
      ' | <b>Last:</b> '+(d.last_updated||'-')+
      ' | <a href="/ccm">CCM</a> | <a href="/greenhouse">Greenhouse</a> | <a href="/irrigation">Irrigation</a> | <a href="/protection">Protection</a> | <a href="/config">Network</a> | <a href="/ota">FW</a>';
    var mdnsHost=d.mdns_hostname?(' | <b>mDNS:</b> '+d.mdns_hostname):'';
    document.getElementById('net').innerHTML=
      '<h3>Network</h3><b>IP:</b> '+d.ip+
      ' | <b>GW:</b> '+d.gateway+mdnsHost+
      '<br><b>MAC:</b> '+d.mac+
      ' | <b>CCM:</b> 224.0.0.1:16520';
    var solSrc=d.ads1110_ok?'<span class=badge-ok>ADS1110 \u2713</span>':(d.sensor&&d.sensor.solar_wm2!==null?'<span class=badge-ok>CCM \u2713</span>':'<span class=badge-ng>none \u2717</span>');
    var solVal=(d.sensor&&d.sensor.solar_wm2!==null)?' '+d.sensor.solar_wm2.toFixed(0)+'W/m&sup2;':'';
    document.getElementById('devstat').innerHTML=
      '<h3>Device Status</h3>'+
      '<b>I2C:</b> '+(d.sht40_ok?'<span class=badge-ok>SHT40 \u2713</span>':'<span class=badge-ng>SHT40 \u2717</span>')+
      (d.scd41_ok?' <span class=badge-ok>SCD41 \u2713</span>':' <span class=badge-ng>SCD41 \u2717</span>')+
      ' | <b>Solar:</b> '+solSrc+solVal+
      ' | <b>1-Wire:</b> '+(d.ds18b20_ok?'<span class=badge-ok>DS18B20 \u2713</span>':'<span class=badge-ng>DS18B20 \u2717</span>')+
      ' | <b>UART:</b> '+(d.sen0575_ok?'<span class=badge-ok>SEN0575 \u2713</span>':'<span class=badge-ng>SEN0575 \u2717</span>');
    var rt='';
    var ccm=d.ccm_map||[];
    for(var i=0;i<8;i++){
      var s=(d.relay_state>>(i))&1;
      var m=ccm[i]||{};
      var tname=m.type||'(unmapped)';
      rt+='<tr><td>'+(i+1)+'</td><td>'+tname+' <span class=ccm>R'+
        (m.room||'-')+'/Rg'+(m.region||'-')+'/O'+(m.order||'-')+'</span></td>'+
        '<td>'+(m.room||'-')+'</td>'+
        '<td class="'+(s?'on':'off')+'">'+(s?'\u2713 ON':'\u2717 OFF')+'</td>'+
        '<td><button class=bon onclick="relay('+(i+1)+',1)">ON</button> '+
        '<button class=bof onclick="relay('+(i+1)+',0)">OFF</button></td></tr>';
    }
    document.getElementById('rtbl').innerHTML=rt;
    var dt='';
    for(var i=0;i<8;i++){
      var s=(d.di_state>>(i))&1;
      dt+='<tr><td>'+(i+1)+'</td><td class="'+(s?'on':'off')+'">'+(s?'\u2713 ON':'\u2717 OFF')+'</td></tr>';
    }
    document.getElementById('dtbl').innerHTML=dt;
    var sv='<h3>Sensors</h3>';
    if(d.sensor&&d.sensor.temp!==null)sv+='<b>SHT40 Temp:</b> '+d.sensor.temp.toFixed(1)+'C ';
    if(d.sensor&&d.sensor.hum!==null)sv+='<b>Hum:</b> '+d.sensor.hum.toFixed(1)+'% ';
    if(d.sensor&&d.sensor.ds18b20_temp!==null)sv+='<b>DS18B20:</b> '+d.sensor.ds18b20_temp.toFixed(1)+'C ';
    if(d.sensor&&d.sensor.solar_wm2!==null)sv+='<b>Solar:</b> '+d.sensor.solar_wm2.toFixed(1)+' W/m&sup2; ';
    if(d.sensor&&d.sensor.co2!==undefined)sv+='<b>CO2:</b> '+d.sensor.co2+'ppm <b>SCD41:</b> '+d.sensor.scd41_temp.toFixed(1)+'C '+d.sensor.scd41_hum.toFixed(1)+'% ';
    if(d.sensor&&d.sensor.temp===null&&d.sensor.hum===null&&d.sensor.ds18b20_temp===null&&d.sensor.solar_wm2===null&&d.sensor.co2===undefined)sv+='<span class=off>No sensors</span>';
    document.getElementById('sens').innerHTML=sv;
    var gh=d.greenhouse||[];
    var anyGh=false;
    for(var i=0;i<gh.length;i++){if(gh[i].enabled)anyGh=true;}
    if(anyGh){
      var gv='<h3>Greenhouse Control</h3><table><tr><th>Rule</th><th>CH</th><th>Sensor</th><th>Temp</th><th>Range</th><th>Duty</th><th>State</th></tr>';
      for(var i=0;i<gh.length;i++){
        var g=gh[i];
        if(!g.enabled)continue;
        gv+='<tr><td>'+(i+1)+'</td><td>CH'+(g.ch+1)+'</td><td>'+g.sensor+'</td>';
        gv+='<td>'+(g.temp!==null?g.temp.toFixed(1)+'C':'-')+'</td>';
        gv+='<td>'+g.temp_open+'-'+g.temp_full+'C</td>';
        gv+='<td>'+g.duty+'%</td>';
        gv+='<td class='+(g.active?'on':'off')+'>'+(g.active?'\u2713 ON':'\u2717 OFF')+'</td></tr>';
      }
      gv+='</table><p><a href="/greenhouse">Settings</a></p>';
      document.getElementById('gh').innerHTML=gv;
    } else {
      document.getElementById('gh').innerHTML='<h3>Greenhouse Control</h3><span class=off>No rules active</span> — <a href="/greenhouse">Configure</a>';
    }
    var ir=d.irrigation||[];
    var anyIr=false;
    for(var i=0;i<ir.length;i++){if(ir[i].enabled)anyIr=true;}
    if(anyIr){
      var iv='<h3>Solar Irrigation</h3>';
      if(d.sensor&&d.sensor.solar_wm2!==null)iv+='<b>Solar:</b> '+d.sensor.solar_wm2.toFixed(1)+' W/m&sup2; | ';
      iv+='<table><tr><th>Rule</th><th>CH</th><th>Accum</th><th>Threshold</th><th>State</th><th>Today</th></tr>';
      for(var i=0;i<ir.length;i++){var r=ir[i];if(!r.enabled)continue;
        iv+='<tr><td>'+(i+1)+'</td><td>CH'+(r.relay_ch+1)+'</td>';
        iv+='<td>'+r.accum_mj.toFixed(3)+' MJ</td><td>'+r.threshold_mj+' MJ</td>';
        iv+='<td class='+(r.irrigating?'on':'off')+'>'+(r.irrigating?'\u2713 WATERING':'\u2717 Accum.')+'</td>';
        iv+='<td><b>'+r.today_count+'</b></td></tr>';}
      iv+='</table><p><a href="/irrigation">Settings</a></p>';
      document.getElementById('irri').innerHTML=iv;
    } else {
      document.getElementById('irri').innerHTML='<h3>Solar Irrigation</h3><span class=off>Not configured</span> — <a href="/irrigation">Configure</a>';
    }
    var p=d.protection||{};
    var pv='<h3>Protection</h3>';
    var anyP=false;
    if(p.dew&&p.dew.enabled){anyP=true;
      pv+='<b>Dew:</b> '+(p.dew.active?'<span class=on>\u2713 ACTIVE</span>':'\u2717 Standby');
      if(p.dew.sunrise)pv+=' (sunrise '+p.dew.sunrise+') ';
      pv+=' | ';}
    if(p.rate&&p.rate.enabled){anyP=true;
      pv+='<b>Rate Guard:</b> '+(p.rate.active?'<span class=on>\u2713 ACTIVE</span>':'\u2717 Normal');
      if(p.rate.current_rate!==null)pv+=' ('+p.rate.current_rate.toFixed(1)+'C/min)';}
    if(anyP){pv+=' — <a href="/protection">Settings</a>';
      document.getElementById('prot').innerHTML=pv;
    }else{
      document.getElementById('prot').innerHTML='<h3>Protection</h3><span class=off>Not configured</span> — <a href="/protection">Configure</a>';
    }
  });
}
load();setInterval(load,5000);
</script>
</body></html>
)RELAY_HTML";

void sendDashboard(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=UTF-8");
  client.println("Connection: close");
  client.println();
  client.print(HTML_PAGE);
}
