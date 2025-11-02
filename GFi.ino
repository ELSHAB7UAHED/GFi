/*
  GFi.ino
  AP name: "GFi"   password: "GFi"
  Web server that scans nearby Wi-Fi networks and shows them in a
  "دموي هاكينج" UI.
  For authorized testing only.
*/

#include <WiFi.h>
#include <WebServer.h>

// AP credentials
const char* ap_ssid = "GFi";
const char* ap_pass = "GFi";

WebServer server(80);

String makeMainPage() {
  // HTML + CSS + JS - "دموي هاكنج" aesthetic
  String html = R"rawliteral(
<!doctype html>
<html lang="ar">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>GFi — Scanner</title>
<style>
  :root{--bg:#0b0b0b;--deep:#0f0f0f;--accent:#c41616;--muted:#bdbdbd}
  html,body{height:100%;margin:0;font-family:Arial,Helvetica,sans-serif;direction:rtl}
  body{background:linear-gradient(180deg,#050505 0%, #0d0d0d 60%);color:#eee}
  .wrap{max-width:980px;margin:18px auto;padding:18px;border-radius:12px;
        box-shadow:0 10px 40px rgba(0,0,0,0.7);background:linear-gradient(180deg,#0b0b0b, #141414)}
  header{display:flex;align-items:center;gap:12px}
  .logo{width:72px;height:72px;border-radius:12px;background:linear-gradient(135deg,#2b0000, #7b0a0a);
        display:flex;align-items:center;justify-content:center;font-weight:900;font-size:20px;color:#fff}
  h1{margin:0;font-size:20px}
  p.lead{color:var(--muted);margin:6px 0 18px}
  .controls{display:flex;gap:8px;align-items:center;margin-bottom:12px}
  .btn{background:var(--accent);color:#fff;padding:8px 12px;border-radius:8px;border:none;cursor:pointer;font-weight:700}
  .btn.secondary{background:transparent;border:1px solid rgba(255,255,255,0.06)}
  .status{margin-left:auto;color:#f3baba;font-weight:700}
  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(300px,1fr));gap:12px}
  .card{background:linear-gradient(180deg,#0f0f0f,#070606);padding:12px;border-radius:10px;border:1px solid rgba(196,22,22,0.06)}
  .ssid{font-weight:800;color:#fff;display:flex;align-items:center;gap:8px}
  .meta{color:var(--muted);font-size:13px;margin-top:6px}
  .rssi{font-weight:900}
  .spark{height:10px;border-radius:6px;overflow:hidden;margin-top:8px}
  .spark > i{display:block;height:100%;background:linear-gradient(90deg,#ff6b6b, #c41616);width:0%}
  footer{margin-top:14px;color:var(--muted);font-size:13px}
  /* blood splatter decorative */
  .bg-splat {position:fixed;left:-120px;top:-120px;opacity:0.06;transform:rotate(-20deg)}
  @media (max-width:520px){.logo{width:56px;height:56px;font-size:18px}}
</style>
</head>
<body>
<svg class="bg-splat" width="600" height="600" viewBox="0 0 200 200" xmlns="http://www.w3.org/2000/svg">
  <g fill="#c41616">
    <path d="M10 30c20-20 40 0 60-10s30 20 50 10 40 0 50 20-10 40-30 40-40-20-60-10-20 30-40 30S-10 70 10 30z"/>
  </g>
</svg>

<div class="wrap">
  <header>
    <div class="logo">GFi</div>
    <div>
      <h1>GFi — Wi-Fi Scanner</h1>
      <p class="lead">شبكة نقطة وصول تعمل محلياً. مسح الشبكات القريبة وعرض النتائج بشكل مرئي — للاستخدام المصرح فقط.</p>
    </div>
    <div class="status" id="apinfo">AP: جارٍ الإقلاع...</div>
  </header>

  <div class="controls">
    <button class="btn" onclick="doScan()">ابدأ فحص الآن</button>
    <button class="btn secondary" onclick="toggleAuto()"><span id="autoBtn">التحديث التلقائي: إيقاف</span></button>
    <div style="margin-left:8px;color:var(--muted)">عدد النتائج: <span id="count">0</span></div>
  </div>

  <div id="results" class="grid">
    <!-- Cards injected here -->
  </div>

  <footer>ملاحظة: هذه الأداة للغرض التعليمي / اختبار أمني مصرح به فقط. أي استخدام غير مصرح به مسؤوليتك الخاصة.</footer>
</div>

<script>
let auto = false;
let autoInterval = null;

function escapeHtml(s){ return s.replace(/[&<>"']/g, function(m){return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[m];}); }

function renderList(items){
  const results = document.getElementById('results');
  results.innerHTML = '';
  document.getElementById('count').textContent = items.length;
  items.forEach(it=>{
    const div = document.createElement('div');
    div.className = 'card';
    const enc = it.encryption===0 ? 'Open' : (it.encryption===4 ? 'WPA/WPA2' : 'Encrypted');
    const strength = Math.min(100, Math.max(0, 120 + it.rssi)); // map rssi -100..0 to 20..120
    div.innerHTML = `
      <div class="ssid"><span>${escapeHtml(it.ssid||'<hidden>')}</span><span style="margin-right:auto;color:#f7c1c1;font-weight:700">${escapeHtml(it.bssid)}</span></div>
      <div class="meta">القناة: ${it.channel} • ${enc} • RSSI: <span class="rssi">${it.rssi}</span>dBm</div>
      <div class="spark"><i style="width:${strength}%"></i></div>
    `;
    results.appendChild(div);
  });
}

async function doScan(){
  try {
    document.getElementById('apinfo').textContent = 'جارٍ الفحص...';
    const resp = await fetch('/scan');
    const data = await resp.json();
    renderList(data);
    const ap = await (await fetch('/apinfo')).json();
    document.getElementById('apinfo').textContent = `AP: ${ap.ssid} • IP: ${ap.ip}`;
  } catch(e){
    console.error(e);
    document.getElementById('apinfo').textContent = 'خطأ أثناء الفحص';
  }
}

function toggleAuto(){
  auto = !auto;
  document.getElementById('autoBtn').textContent = auto ? 'التحديث التلقائي: تشغيل' : 'التحديث التلقائي: إيقاف';
  if(auto){
    doScan();
    autoInterval = setInterval(doScan, 8000);
  } else {
    clearInterval(autoInterval);
    autoInterval = null;
  }
}

// initial load
window.addEventListener('load', async()=>{
  // show AP info immediately
  try {
    const ap = await (await fetch('/apinfo')).json();
    document.getElementById('apinfo').textContent = `AP: ${ap.ssid} • IP: ${ap.ip}`;
  } catch(e){ /* ignore */ }
  doScan();
});
</script>
</body>
</html>
)rawliteral";
  return html;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", makeMainPage());
}

void handleAPInfo() {
  IPAddress ip = WiFi.softAPIP();
  String json = "{";
  json += "\"ssid\":\"" + String(ap_ssid) + "\",";
  json += "\"ip\":\"" + ip.toString() + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleScan() {
  // perform a synchronous scan (blocking) — returns number of networks found
  int n = WiFi.scanNetworks();
  // build JSON array
  String json = "[";
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    String bssid = WiFi.BSSIDstr(i);
    int32_t rssi = WiFi.RSSI(i);
    int32_t ch = WiFi.channel(i);
    wifi_auth_mode_t encType = WiFi.encryptionType(i);
    int enc = (int)encType;
    json += "{";
    json += "\"ssid\":\"" + ssid + "\",";
    json += "\"bssid\":\"" + bssid + "\",";
    json += "\"rssi\":" + String(rssi) + ",";
    json += "\"channel\":" + String(ch) + ",";
    json += "\"encryption\":" + String(enc);
    json += "}";
    if (i < n - 1) json += ",";
  }
  json += "]";
  // clear scan results cached by the driver to avoid stale data (optional)
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Start AP
  bool ok = WiFi.softAP(ap_ssid, ap_pass);
  if(!ok) {
    Serial.println("Failed to start AP!");
  } else {
    Serial.print("AP started: ");
    Serial.println(ap_ssid);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  // Setup server routes
  server.on("/", handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/apinfo", HTTP_GET, handleAPInfo);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  // keep loop light; heavy work done on-demand via endpoints
  delay(10);
}
