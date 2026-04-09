#pragma once

// ============================================================
// Greenhouse Control Page (GET /greenhouse)
// ============================================================
void sendGreenhousePage(WiFiClient& client) {
  sendCommonHead(client, "Greenhouse Control");
  client.println("<style>input[type=number]{width:60px}select{width:80px}</style></head><body>");
  client.println("<h2>Greenhouse Control</h2>");
  client.print(NAV_LINKS); client.println();
  client.println("<p class=note>Temperature-based proportional relay control. CCM commands take priority when active.</p>");
  client.println("<div class=sec id=ghstat>Loading...</div>");
  client.println("<div class=sec id=ghrun>Loading...</div>");

  // Config form
  client.println("<form action=/api/greenhouse onsubmit=\"return ghSubmit(this,this.querySelector('[type=submit]'))\">");
  client.println("<table><tr><th>Rule</th><th>Enable</th><th>CH</th><th>Sensor</th><th title='Relay starts opening at this temp'>Open(C)</th><th title='100% duty at this temp'>Full(C)</th><th>Cycle(s)</th></tr>");
  for (int i = 0; i < GH_CTRL_SLOTS; i++) {
    client.printf("<tr><td>%d</td>", i + 1);
    client.printf("<td><input type=checkbox name=en%d id=en%d aria-label='Enable rule %d' value=1%s></td>", i, i, i+1, ghCtrl[i].enabled ? " checked" : "");
    // CH select
    client.printf("<td><select name=ch%d>", i);
    client.printf("<option value=-1%s>-</option>", ghCtrl[i].ch < 0 ? " selected" : "");
    for (int c = 0; c < 8; c++) {
      client.printf("<option value=%d%s>CH%d</option>", c, ghCtrl[i].ch == c ? " selected" : "", c + 1);
    }
    client.printf("</select></td>");
    // Sensor select
    client.printf("<td><select name=ss%d>", i);
    client.printf("<option value=0%s>SHT40</option>", ghCtrl[i].sensor_src == 0 ? " selected" : "");
    client.printf("<option value=1%s>DS18B20</option>", ghCtrl[i].sensor_src == 1 ? " selected" : "");
    client.printf("</select></td>");
    client.printf("<td><input type=number name=to%d value=%.1f min=-10 max=60 step=0.5></td>", i, ghCtrl[i].temp_open);
    client.printf("<td><input type=number name=tf%d value=%.1f min=-10 max=60 step=0.5></td>", i, ghCtrl[i].temp_full);
    client.printf("<td><input type=number name=cy%d value=%d min=10 max=600></td></tr>", i, ghCtrl[i].cycle_sec);
  }
  client.println("</table>");
  client.println("<input type=submit value='Save'>");
  client.println("</form>");
  client.println("<p class=note>Open: relay starts at this temp. Full: 100% duty at this temp. Cycle: ON+OFF period in seconds.</p>");
  // Auto-refresh + form JS
  client.println("<script>");
  client.print(FORM_JS);
  client.println("function ghSubmit(form,btn){");
  client.println("var anyEn=false;for(var i=0;i<4;i++){var e=form.querySelector('[name=en'+i+']');if(e&&e.checked)anyEn=true;}");
  client.println("if(!anyEn&&!confirm('All rules will be disabled. Continue?'))return false;");
  client.println("for(var i=0;i<4;i++){");
  client.println("var to=form.querySelector('[name=to'+i+']');");
  client.println("var tf=form.querySelector('[name=tf'+i+']');");
  client.println("var en=form.querySelector('[name=en'+i+']');");
  client.println("if(en&&en.checked&&to&&tf&&parseFloat(to.value)>=parseFloat(tf.value)){");
  client.println("alert('Rule '+(i+1)+': Open temp must be less than Full temp');");
  client.println("return false;}}");
  client.println("return submitForm(form,btn);}");
  client.println("function ghLoad(){");
  client.println("fetch('/api/state').then(function(r){return r.json();}).then(function(d){");
  // Sensor status
  client.println("var s='<h3>Sensors</h3>';");
  client.println("if(d.sht40_ok)s+='<b>SHT40:</b> <span class=on>'+(d.sensor.temp!==null?d.sensor.temp.toFixed(1)+'C':'OK')+'</span> ';");
  client.println("else s+='<b>SHT40:</b> <span class=off>none</span> ';");
  client.println("if(d.sensor.hum!==null)s+='<b>Hum:</b> '+d.sensor.hum.toFixed(1)+'% ';");
  client.println("if(d.ds18b20_ok)s+='| <b>DS18B20:</b> <span class=on>'+(d.sensor.ds18b20_temp!==null?d.sensor.ds18b20_temp.toFixed(1)+'C':'OK')+'</span>';");
  client.println("else s+='| <b>DS18B20:</b> <span class=off>none</span>';");
  client.println("document.getElementById('ghstat').innerHTML=s;");
  // Runtime table
  client.println("var gh=d.greenhouse||[];var h='<h3>Runtime</h3>';");
  client.println("var any=false;for(var i=0;i<gh.length;i++)if(gh[i].enabled)any=true;");
  client.println("if(any){");
  client.println("h+='<table><tr><th>Rule</th><th>CH</th><th>Sensor</th><th>Temp</th><th>Range</th><th>Duty</th><th>State</th></tr>';");
  client.println("for(var i=0;i<gh.length;i++){var g=gh[i];if(!g.enabled)continue;");
  client.println("h+='<tr><td>'+(i+1)+'</td><td>CH'+(g.ch+1)+'</td><td>'+g.sensor+'</td>';");
  client.println("h+='<td style=\"font-size:1.3em;font-weight:bold\">'+(g.temp!==null?g.temp.toFixed(1)+'C':'-')+'</td>';");
  client.println("h+='<td>'+g.temp_open+' - '+g.temp_full+'C</td>';");
  // Duty bar visualization
  client.println("h+='<td><div style=\"background:#2e2e2e;border-radius:3px;height:18px;width:80px;display:inline-block;vertical-align:middle\">';");
  client.println("h+='<div style=\"background:'+(g.duty>70?'#e53935':g.duty>30?'#ffa726':'#43a047')+';height:100%;width:'+g.duty+'%;border-radius:3px\"></div></div> '+g.duty+'%</td>';");
  client.println("h+='<td class='+(g.active?'on':'off')+'><b>'+(g.active?'\\u2713 ON':'\\u2717 OFF')+'</b></td></tr>';}");
  client.println("h+='</table>';}else{h+='<span class=off>No rules active</span>';}");
  client.println("document.getElementById('ghrun').innerHTML=h;");
  client.println("});}");
  client.println("ghLoad();setInterval(ghLoad,3000);");
  client.println("</script>");
  client.println("</body></html>");
}

// ============================================================
// POST /api/greenhouse
// ============================================================
void handleGreenhousePost(WiFiClient& client, const String& body) {
  auto getField = [&](const String& key) -> String {
    String search = key + "=";
    int idx = body.indexOf(search);
    if (idx < 0) return "";
    idx += search.length();
    int end = body.indexOf('&', idx);
    if (end < 0) end = body.length();
    return body.substring(idx, end);
  };

  for (int i = 0; i < GH_CTRL_SLOTS; i++) {
    String en = getField(String("en") + i);
    ghCtrl[i].enabled = (en == "1");

    String ch = getField(String("ch") + i);
    if (ch.length() > 0) ghCtrl[i].ch = ch.toInt();

    String ss = getField(String("ss") + i);
    if (ss.length() > 0) ghCtrl[i].sensor_src = ss.toInt();

    String to = getField(String("to") + i);
    if (to.length() > 0) ghCtrl[i].temp_open = to.toFloat();

    String tf = getField(String("tf") + i);
    if (tf.length() > 0) ghCtrl[i].temp_full = tf.toFloat();

    String cy = getField(String("cy") + i);
    if (cy.length() > 0) ghCtrl[i].cycle_sec = cy.toInt();

    // Reset runtime on config change
    ghRun[i].cycleStart = 0;
  }

  saveGreenhouseConfig();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{\"ok\":true}");
}
