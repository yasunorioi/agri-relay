#pragma once

// ============================================================
// Modbus Master Config Page (GET /modbus)
// ============================================================
void sendModbusConfigPage(WiFiClient& client) {
  sendCommonHead(client, L("Modbus Master", "Modbusマスター"));
  client.println("<style>input[type=number]{width:60px}input[type=text]{width:100px;padding:3px;background:#1a1a1f;color:#eee;border:1px solid #3e3e44;border-radius:3px}select{width:100px}</style></head><body>");
  client.printf("<h2>%s</h2>\n", L("Modbus RTU Master", "Modbus RTUマスター"));
  printNavLinks(client);
  client.printf("<p class=note>%s</p>\n",
    L("RS485 port (GPIO4/5). Controls external Modbus RTU Relay boards (e.g. Waveshare).",
      "RS485ポート(GPIO4/5)。外部Modbus RTUリレーボード(Waveshare等)を制御します。"));

  // Status section
  client.printf("<div class=sec id=mbstat>%s</div>\n", L("Loading...","読み込み中..."));

  client.println("<form action=/api/modbus onsubmit=\"return submitForm(this,this.querySelector('[type=submit]'))\">");
  client.println("<table>");

  // Enable
  client.printf("<tr><th>%s</th><td><input type=checkbox name=mb_en aria-label='%s' value=1%s></td></tr>\n",
    L("Enable","有効"), L("Enable Modbus master","Modbusマスターを有効"), mbMaster.enabled ? " checked" : "");

  // Baud rate
  client.printf("<tr><th>%s</th><td><select name=mb_baud>", L("Baud Rate","ボーレート"));
  const int bauds[] = {4800, 9600, 19200, 38400, 57600, 115200};
  for (int i = 0; i < 6; i++) {
    client.printf("<option value=%d%s>%d</option>", bauds[i],
                  mbMaster.baud_rate == bauds[i] ? " selected" : "", bauds[i]);
  }
  client.println("</select></td></tr>");

  // Poll interval
  client.printf("<tr><th>%s</th><td><input type=number name=mb_poll value=%d min=0 max=300></td></tr>\n",
    L("Poll interval (s)","ポーリング間隔(s)"), mbMaster.poll_interval);

  client.println("</table>");

  // Slave devices table
  client.printf("<h3>%s</h3>\n", L("Slave Devices","スレーブデバイス"));
  client.printf("<table><tr><th>#</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th></tr>\n",
    L("Enable","有効"), L("Address","アドレス"), L("Channels","CH数"), L("Label","ラベル"));

  for (int i = 0; i < MB_MAX_SLAVES; i++) {
    client.printf("<tr><td>%d</td>", i + 1);
    client.printf("<td><input type=checkbox name=se%d aria-label='Enable slave %d' value=1%s></td>",
                  i, i + 1, mbMaster.slaves[i].enabled ? " checked" : "");
    client.printf("<td><input type=number name=sa%d value=%d min=1 max=247></td>",
                  i, mbMaster.slaves[i].addr);
    client.printf("<td><input type=number name=sc%d value=%d min=1 max=8></td>",
                  i, mbMaster.slaves[i].ch_count);
    client.printf("<td><input type=text name=sl%d value='%s' maxlength=15></td></tr>\n",
                  i, mbMaster.slaves[i].label);
    yield();
  }
  client.println("</table>");

  client.printf("<input type=submit value='%s'>\n", L("Save","保存"));
  client.println("</form>");

  // Manual relay control
  client.printf("<div class=sec><h3>%s</h3>\n", L("External Relay Control","外部リレー制御"));
  client.printf("<p class=note>%s</p>\n",
    L("Click to toggle relays on external Modbus RTU boards.",
      "外部Modbusリレーボードのリレーをトグルします。"));
  client.println("<div id=extctrl></div></div>");

  // JavaScript
  client.println("<script>");
  client.print(FORM_JS);

  // Status loader + relay buttons
  client.println("function mbLoad(){");
  client.println("fetch('/api/state').then(function(r){return r.json();}).then(function(d){");
  client.println("var mb=d.modbus||{};var s='<h3>Status</h3>';");
  client.println("s+='<b>Master:</b> '+(mb.enabled?'<span class=on>Enabled</span>':'<span class=off>Disabled</span>');");
  client.println("if(mb.slaves){mb.slaves.forEach(function(sv,i){");
  client.println("  if(!sv.en)return;");
  client.println("  s+='<br><b>'+(sv.label||('Slave '+(i+1)))+'</b> (addr='+sv.addr+'): ';");
  client.println("  for(var c=0;c<sv.ch;c++){");
  client.println("    var on=(sv.state>>c)&1;");
  client.println("    s+='<span class='+(on?'on':'off')+'>CH'+(c+1)+'</span> ';");
  client.println("  }});}");
  client.println("document.getElementById('mbstat').innerHTML=s;");

  // Build relay control buttons
  client.println("var html='';");
  client.println("if(mb.slaves){mb.slaves.forEach(function(sv,i){");
  client.println("  if(!sv.en)return;");
  client.println("  html+='<p><b>'+(sv.label||('Slave '+(i+1)))+'</b> ';");
  client.println("  for(var c=0;c<sv.ch;c++){");
  client.println("    var on=(sv.state>>c)&1;");
  client.println("    html+='<button style=\"margin:2px;padding:4px 8px;background:'+(on?'#1b5e20':'#333')+';color:#fff;border:1px solid #555;border-radius:3px;cursor:pointer\" ");
  client.println("    onclick=\"mbToggle('+i+','+c+','+(on?0:1)+')\">'+'CH'+(c+1)+(on?' ON':' OFF')+'</button> ';");
  client.println("  }html+='</p>';});}");
  client.println("document.getElementById('extctrl').innerHTML=html;");
  client.println("});}");

  // Toggle function
  client.println("function mbToggle(slave,ch,on){");
  client.println("fetch('/api/modbus/relay',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},");
  client.println("body:'slave='+slave+'&ch='+ch+'&on='+on}).then(function(){mbLoad();});}");

  client.println("mbLoad();setInterval(mbLoad,5000);");
  client.println("</script>");
  client.println("</body></html>");
}

// ============================================================
// POST /api/modbus — save config
// ============================================================
void handleModbusConfigPost(WiFiClient& client, const String& body) {
  auto getField = [&](const String& key) -> String {
    String search = key + "=";
    int idx = body.indexOf(search);
    if (idx < 0) return "";
    idx += search.length();
    int end = body.indexOf('&', idx);
    if (end < 0) end = body.length();
    String val = body.substring(idx, end);
    val.replace('+', ' ');
    // URL decode
    String decoded;
    for (int i = 0; i < (int)val.length(); i++) {
      if (val[i] == '%' && i + 2 < (int)val.length()) {
        char hex[3] = { val[i+1], val[i+2], '\0' };
        decoded += (char)strtol(hex, nullptr, 16);
        i += 2;
      } else {
        decoded += val[i];
      }
    }
    return decoded;
  };

  int oldBaud = mbMaster.baud_rate;

  mbMaster.enabled = (getField("mb_en") == "1");

  String baud = getField("mb_baud");
  if (baud.length() > 0) mbMaster.baud_rate = baud.toInt();

  String poll = getField("mb_poll");
  if (poll.length() > 0) mbMaster.poll_interval = constrain(poll.toInt(), 0, 300);

  for (int i = 0; i < MB_MAX_SLAVES; i++) {
    mbMaster.slaves[i].enabled = (getField(String("se") + i) == "1");
    String a = getField(String("sa") + i);
    if (a.length() > 0) mbMaster.slaves[i].addr = constrain(a.toInt(), 1, 247);
    String c = getField(String("sc") + i);
    if (c.length() > 0) mbMaster.slaves[i].ch_count = constrain(c.toInt(), 1, 8);
    String l = getField(String("sl") + i);
    strlcpy(mbMaster.slaves[i].label, l.c_str(), sizeof(mbMaster.slaves[i].label));
  }

  saveModbusMasterConfig();

  // Re-init serial if baud changed
  if (mbMaster.baud_rate != oldBaud) {
    Serial2.end();
    Serial2.begin(mbMaster.baud_rate);
  }
  if (mbMaster.enabled && !mbInitialized) {
    mbNode.begin(1, Serial2);
    mbInitialized = true;
  }

  Serial.printf("[MODBUS-M] Config saved: enabled=%d baud=%d poll=%d\n",
                mbMaster.enabled, mbMaster.baud_rate, mbMaster.poll_interval);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{\"ok\":true}");
}

// ============================================================
// POST /api/modbus/relay — toggle external relay
// ============================================================
void handleModbusRelayPost(WiFiClient& client, const String& body) {
  auto getField = [&](const String& key) -> String {
    String search = key + "=";
    int idx = body.indexOf(search);
    if (idx < 0) return "";
    idx += search.length();
    int end = body.indexOf('&', idx);
    if (end < 0) end = body.length();
    return body.substring(idx, end);
  };

  int slave = getField("slave").toInt();
  int ch    = getField("ch").toInt();
  bool on   = (getField("on") == "1");

  bool ok = mbWriteCoil(slave, ch, on);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.printf("{\"ok\":%s}\n", ok ? "true" : "false");
}
