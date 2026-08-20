#include "SetupPortal.h"
#include "AppConfig.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_system.h>

static WebServer s_http(80);
static DNSServer s_dns;
static bool s_active = false;
static String s_ap_name;
static String s_ap_pass;

static const char* FORM_HTML = R"(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Clawdito Setup</title>
<style>
 body{font-family:system-ui,sans-serif;background:#17121f;color:#f2eef7;
      max-width:430px;margin:1.2rem auto;padding:0 1rem}
 h1{font-size:1.3rem;color:#e0784f;margin:.4rem 0}
 .sub{color:#b9b0c7;font-size:.85rem;margin-bottom:1rem}
 label{display:block;margin:.8rem 0 .2rem;font-size:.85rem;color:#b9b0c7}
 input,select{width:100%;padding:.55rem;font-size:1rem;box-sizing:border-box;
      background:#2b2436;color:#f2eef7;border:1px solid #4a4158;border-radius:6px}
 button{width:100%;padding:.8rem;font-size:1rem;margin-top:1.1rem;border:none;
      border-radius:6px;background:#e0784f;color:#17121f;font-weight:700}
 .cols{display:flex;gap:.6rem}.cols>*{flex:1}
 .note{color:#b9b0c7;font-size:.8rem;margin-top:1rem}
 code{color:#e0784f}
</style></head><body>
<h1>&#129408; Clawdito</h1>
<div class="sub">Connect me to your WiFi and your bridge.</div>
<form method="POST" action="/apply">
 <label>WiFi network</label>
 <select id="nets" name="ssid"><option value="">scanning&hellip;</option></select>
 <label>Or type the network name</label>
 <input name="ssid_text" placeholder="network name">
 <label>WiFi password</label>
 <input type="password" name="wpass">
 <div class="cols">
  <div><label>Bridge host</label><input name="host" required placeholder="192.168.x.x"></div>
  <div><label>Port</label><input type="number" name="port" value="8787" min="1" max="65535"></div>
 </div>
 <label>Bridge token</label>
 <input type="password" name="token" placeholder="from the bridge terminal">
 <button type="submit">Connect</button>
</form>
<div class="note">The token is printed when you start
 <code>clawdito_bridge.py</code> on your computer. Start the bridge first.</div>
<script>
fetch('/networks').then(r=>r.json()).then(list=>{
  const s=document.getElementById('nets');
  s.innerHTML='<option value="">choose a network</option>';
  list.sort((a,b)=>b.rssi-a.rssi).forEach(n=>{
    const o=document.createElement('option');
    o.value=n.ssid;
    o.textContent=n.ssid+' ('+n.rssi+' dBm)';
    s.appendChild(o);
  });
}).catch(()=>{});
</script>
</body></html>)";

static const char* SAVED_HTML = R"(<!doctype html>
<html><head><meta charset="utf-8"><title>Clawdito</title>
<style>body{font-family:system-ui,sans-serif;background:#17121f;color:#f2eef7;
max-width:430px;margin:2rem auto;padding:0 1rem}h1{color:#8bd450}</style>
</head><body><h1>Saved &mdash; rebooting</h1>
<p>Clawdito is leaving setup mode and joining your network.</p>
<p>If it can't reach the bridge within a minute, hold the BOOT button
for 5 seconds to come back here.</p></body></html>)";

namespace portal {

static String random_password() {
  // unambiguous lowercase+digits alphabet
  static const char* AB = "abcdefghjkmnpqrstuvwxyz23456789";
  String out;
  for (int i = 0; i < 8; i++) out += AB[esp_random() % strlen(AB)];
  return out;
}

static void handle_root() { s_http.send(200, "text/html", FORM_HTML); }

static void handle_networks() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i) json += ",";
    String ssid = WiFi.SSID(i);
    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");
    json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  s_http.send(200, "application/json", json);
}

static void handle_apply() {
  String ssid = s_http.arg("ssid");
  if (ssid.isEmpty()) ssid = s_http.arg("ssid_text");
  String host = s_http.arg("host");
  if (ssid.isEmpty() || host.isEmpty()) {
    s_http.send(400, "text/plain", "WiFi network and bridge host are required.");
    return;
  }
  AppConfig c;
  c.wifi_ssid    = ssid;
  c.wifi_pass    = s_http.arg("wpass");
  c.bridge_host  = host;
  c.bridge_port  = (uint16_t)s_http.arg("port").toInt();
  if (c.bridge_port == 0) c.bridge_port = 8787;
  c.bridge_token = s_http.arg("token");
  config::save(c);

  s_http.send(200, "text/html", SAVED_HTML);
  delay(800);
  ESP.restart();
}

static void handle_missing() {
  // 302 to ourselves: triggers the captive-portal sheet on phones
  s_http.sendHeader("Location", "http://192.168.4.1/", true);
  s_http.send(302, "text/plain", "");
}

void start() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char name[20];
  snprintf(name, sizeof(name), "Clawdito-%02X%02X", mac[4], mac[5]);
  s_ap_name = name;
  s_ap_pass = random_password();

  WiFi.mode(WIFI_AP_STA);   // STA kept up so /networks can scan
  WiFi.softAP(s_ap_name.c_str(), s_ap_pass.c_str());
  s_dns.start(53, "*", WiFi.softAPIP());

  s_http.on("/", handle_root);
  s_http.on("/networks", handle_networks);
  s_http.on("/apply", HTTP_POST, handle_apply);
  s_http.onNotFound(handle_missing);
  s_http.begin();
  s_active = true;

  Serial.printf("[portal] AP=%s pass=%s url=http://192.168.4.1\n",
                s_ap_name.c_str(), s_ap_pass.c_str());
}

void loop() {
  if (!s_active) return;
  s_dns.processNextRequest();
  s_http.handleClient();
}

bool active() { return s_active; }
String ap_name() { return s_ap_name; }
String ap_password() { return s_ap_pass; }

}  // namespace portal
