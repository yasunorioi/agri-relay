#pragma once

// ============================================================
// MQTT Config Page (GET /mqtt)
// ============================================================
void sendMqttConfigPage(WiFiClient& client) {
  sendCommonHead(client, L("MQTT Config", "MQTT設定"));
  client.println("<style>input[type=number]{width:70px}input[type=text]{width:160px}</style></head><body>");
  client.printf("<h2>%s</h2>\n", L("MQTT Settings", "MQTT設定"));
  printNavLinks(client);

  // Connection status badge
  const char* connBadge = mqttOnline
    ? "<span class=badge-ok>Connected</span>"
    : "<span class=badge-ng>Disconnected</span>";
  client.printf("<p><b>%s:</b> %s &nbsp; <b>Broker:</b> %s:%d &nbsp; <b>House:</b> %s</p>\n",
                L("Status","接続状態"), connBadge,
                mqttCfg.broker, mqttCfg.port, mqttCfg.house_id);

  client.println("<form action=/api/mqtt onsubmit=\"return submitForm(this,this.querySelector('[type=submit]'))\">");

  // MQTT basic settings
  client.printf("<div class=sec><h3>%s</h3>\n", L("Broker Settings", "ブローカー設定"));
  client.printf("<table>");
  client.printf("<tr><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th></tr>\n",
                L("Broker IP","ブローカーIP"), L("Port","ポート"),
                L("House ID","ハウスID"), L("Client ID","クライアントID"),
                L("Publish Interval(s)","送信間隔(秒)"), L("Enable","有効"));
  client.printf("<tr>");
  client.printf("<td><input type=text name=broker value='%s' maxlength=63></td>", mqttCfg.broker);
  client.printf("<td><input type=number name=port value=%d min=1 max=65535></td>", mqttCfg.port);
  client.printf("<td><input type=text name=house_id value='%s' maxlength=15></td>", mqttCfg.house_id);
  client.printf("<td><input type=text name=client_id value='%s' maxlength=31 placeholder='(auto)'></td>", mqttCfg.client_id);
  client.printf("<td><input type=number name=publish_interval value=%d min=1 max=3600></td>", mqttCfg.publish_interval);
  client.printf("<td><input type=checkbox name=enabled value=1%s></td>", mqttCfg.enabled ? " checked" : "");
  client.printf("</tr></table></div>\n");

  // DI Link + WDT table
  client.printf("<div class=sec><h3>%s</h3>\n", L("DI Link & MQTT Watchdog", "DI連動 & MQTTウォッチドッグ"));
  client.printf("<p class=note>%s</p>\n",
                L("DI Link: relay follows DI input. WDT: auto-OFF after N seconds since last MQTT command (0=disabled).",
                  "DI連動: DI入力に従いリレー制御。WDT: 最終MQTT受信後Nでauto-OFF (0=無効)。"));
  client.printf("<table><tr><th>CH</th><th>%s</th><th>%s</th><th>WDT(s)</th></tr>\n",
                L("DI Link","DIリンク"), L("DI Invert","DI反転"));

  for (int i = 0; i < 8; i++) {
    client.printf("<tr><td>%d</td>", i + 1);
    // DI Link dropdown
    client.printf("<td><select name=dil%d>", i);
    client.printf("<option value=-1%s>(none)</option>", relayCh[i].di_link < 0 ? " selected" : "");
    for (int d = 0; d < 8; d++) {
      client.printf("<option value=%d%s>DI%d</option>",
                    d, relayCh[i].di_link == d ? " selected" : "", d + 1);
    }
    client.printf("</select></td>");
    // DI Invert checkbox
    client.printf("<td><label><input type=checkbox name=dii%d value=1%s> inv</label></td>",
                  i, relayCh[i].di_invert ? " checked" : "");
    // WDT
    client.printf("<td><input type=number name=wdt%d value=%d min=0 max=3600></td></tr>\n",
                  i, relayCh[i].watchdog_sec);
  }

  client.println("</table></div>");
  client.printf("<input type=submit value='%s'>\n", L("Save MQTT Config", "MQTT設定を保存"));
  client.println("</form>");
  client.printf("<p class=note>%s</p>\n",
                L("Changes take effect immediately (no reboot required).",
                  "変更は即時反映されます（再起動不要）。"));
  client.println("<script>");
  client.print(FORM_JS);
  client.println("</script>");
  client.println("</body></html>");
}

// ============================================================
// POST /api/mqtt — save MQTT config + relay channel config
// ============================================================
void handleMqttConfigPost(WiFiClient& client, const String& body) {
  auto getField = [&](const String& key) -> String {
    String search = key + "=";
    int idx = body.indexOf(search);
    if (idx < 0) return "";
    idx += search.length();
    int end = body.indexOf('&', idx);
    if (end < 0) end = body.length();
    String val = body.substring(idx, end);
    val.replace('+', ' ');
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

  // MQTT config
  String broker = getField("broker");
  if (broker.length() > 0) {
    strncpy(mqttCfg.broker, broker.c_str(), sizeof(mqttCfg.broker) - 1);
    mqttCfg.broker[sizeof(mqttCfg.broker) - 1] = '\0';
  }

  String port = getField("port");
  if (port.length() > 0) mqttCfg.port = port.toInt();

  String house_id = getField("house_id");
  if (house_id.length() > 0) {
    strncpy(mqttCfg.house_id, house_id.c_str(), sizeof(mqttCfg.house_id) - 1);
    mqttCfg.house_id[sizeof(mqttCfg.house_id) - 1] = '\0';
  }

  String client_id = getField("client_id");
  strncpy(mqttCfg.client_id, client_id.c_str(), sizeof(mqttCfg.client_id) - 1);
  mqttCfg.client_id[sizeof(mqttCfg.client_id) - 1] = '\0';

  String pub_int = getField("publish_interval");
  if (pub_int.length() > 0) mqttCfg.publish_interval = pub_int.toInt();

  mqttCfg.enabled = (getField("enabled") == "1");

  saveMqttConfig();

  // Relay channel config (DI link + WDT)
  for (int i = 0; i < 8; i++) {
    String dl = getField(String("dil") + i);
    if (dl.length() > 0) relayCh[i].di_link = dl.toInt();

    String di = getField(String("dii") + i);
    relayCh[i].di_invert = (di == "1");

    String wdt = getField(String("wdt") + i);
    if (wdt.length() > 0) relayCh[i].watchdog_sec = wdt.toInt();
  }

  saveRelayChConfig();

  // Re-init MQTT with new settings (disconnect triggers reconnect)
  if (mqttClient.connected()) mqttClient.disconnect();
  mqttOnline = false;
  initMqtt();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{\"ok\":true}");
}
