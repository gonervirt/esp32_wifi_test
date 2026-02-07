/**
 * @file main.cpp
 * @brief ESP32 WiFi Test Application
 * 
 * This application configures the ESP32 as an Access Point (AP) and hosts a 
 * web server. The web interface provides diagnostic tools to test the WiFi 
 * module, including scanning for nearby networks and displaying system status.
 * 
 * Features:
 * - Access Point Mode (SSID: ESP32_WiFi_Test)
 * - Modern, responsive Web Interface
 * - AJAX/Fetch API for non-blocking UI updates
 * - WiFi Scanning capability
 * 
 * @version 1.0.0
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include <lwip/stats.h>

// -------------------------------------------------------------------------
// Configuration Constants
// -------------------------------------------------------------------------
const char* AP_SSID = "ESP32_WiFi_Test";
const char* AP_PASS = "12345678"; // Min 8 chars for WPA2
const int   WEB_PORT = 80;

// -------------------------------------------------------------------------
// Global Objects
// -------------------------------------------------------------------------
WebServer server(WEB_PORT);
volatile uint32_t disconnect_count = 0;

// -------------------------------------------------------------------------
// Web Content (HTML/CSS/JS)
// -------------------------------------------------------------------------
// Stored in PROGMEM to save RAM
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 WiFi Diagnostic Tool</title>
    <style>
        :root {
            --primary-color: #2563eb;
            --bg-color: #f3f4f6;
            --card-bg: #ffffff;
            --text-color: #1f2937;
            --border-color: #e5e7eb;
        }
        body {
            font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-color);
            margin: 0;
            padding: 20px;
            line-height: 1.5;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: var(--card-bg);
            padding: 2rem;
            border-radius: 12px;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
        }
        h1 { color: var(--primary-color); margin-top: 0; }
        .card {
            background: #f8fafc;
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 1rem;
            margin-bottom: 1.5rem;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 1rem;
        }
        .stat-item { display: flex; flex-direction: column; }
        .stat-label { font-size: 0.875rem; color: #6b7280; }
        .stat-value { font-size: 1.125rem; font-weight: 600; }
        
        button {
            background-color: var(--primary-color);
            color: white;
            border: none;
            padding: 0.75rem 1.5rem;
            border-radius: 6px;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.2s;
        }
        button:hover { background-color: #1d4ed8; }
        button:disabled { background-color: #9ca3af; cursor: not-allowed; }
        
        table { width: 100%; border-collapse: collapse; margin-top: 1rem; }
        th, td { text-align: left; padding: 0.75rem; border-bottom: 1px solid var(--border-color); }
        th { background-color: #f1f5f9; font-weight: 600; }
        tr:hover { background-color: #f8fafc; }
        
        .badge {
            padding: 0.25rem 0.5rem;
            border-radius: 9999px;
            font-size: 0.75rem;
            font-weight: 500;
        }
        .badge-good { background-color: #dcfce7; color: #166534; }
        .badge-weak { background-color: #fee2e2; color: #991b1b; }
    </style>
</head>
<body>
    <div class="container">
        <h1>WiFi Diagnostic Tool</h1>
        
        <div class="card">
            <div class="grid">
                <div class="stat-item">
                    <span class="stat-label">AP IP Address</span>
                    <span class="stat-value" id="ip">Loading...</span>
                </div>
                <div class="stat-item">
                    <span class="stat-label">MAC Address</span>
                    <span class="stat-value" id="mac">Loading...</span>
                </div>
                <div class="stat-item">
                    <span class="stat-label">Uptime</span>
                    <span class="stat-value" id="uptime">0s</span>
                </div>
                <div class="stat-item">
                    <span class="stat-label">Free Heap</span>
                    <span class="stat-value" id="heap">0 KB</span>
                </div>
                <div class="stat-item">
                    <span class="stat-label">TX Power</span>
                    <span class="stat-value" id="txpower">0 dBm</span>
                </div>
                <div class="stat-item">
                    <span class="stat-label">CPU Freq</span>
                    <span class="stat-value" id="cpu_freq">0 MHz</span>
                </div>
                <div class="stat-item">
                    <span class="stat-label">TCP Retries</span>
                    <span class="stat-value" id="tcprexmit">0</span>
                </div>
                <div class="stat-item">
                    <span class="stat-label">Disconnects</span>
                    <span class="stat-value" id="disconnects">0</span>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>Diagnostics</h2>
            <div class="grid">
                <div class="stat-item">
                    <span class="stat-label">Connected Clients</span>
                    <span class="stat-value" id="clientCount">0</span>
                </div>
                <div class="stat-item">
                    <span class="stat-label">Signal Strength (RSSI)</span>
                    <span class="stat-value" id="clientRssi">--</span>
                </div>
            </div>
            <div style="margin-top: 1rem; display: flex; gap: 10px; flex-wrap: wrap;">
                <button onclick="runPingTest()" id="btnPing">Test Latency</button>
                <button onclick="runSpeedTest()" id="btnSpeed">Test Throughput</button>
                <button onclick="runUploadTest()" id="btnUpload">Test Upload</button>
            </div>
            <div id="testResults" style="margin-top: 1rem; font-family: monospace; white-space: pre-wrap; background: #eee; padding: 10px; border-radius: 6px; display: none;"></div>
        </div>

        <div class="controls">
            <button id="scanBtn" onclick="scanNetworks()">Scan Nearby Networks</button>
            <span id="statusMsg" style="margin-left: 10px; color: #6b7280;"></span>
        </div>

        <div style="overflow-x: auto;">
            <table id="wifiTable">
                <thead>
                    <tr>
                        <th>SSID</th>
                        <th>RSSI</th>
                        <th>Channel</th>
                        <th>Security</th>
                    </tr>
                </thead>
                <tbody id="wifiList">
                    <tr><td colspan="4" style="text-align:center">Ready to scan</td></tr>
                </tbody>
            </table>
        </div>
    </div>

    <script>
        function updateStatus() {
            fetch('/api/status')
                .then(res => res.json())
                .then(data => {
                    document.getElementById('ip').textContent = data.ip;
                    document.getElementById('mac').textContent = data.mac;
                    document.getElementById('uptime').textContent = formatUptime(data.uptime);
                    document.getElementById('heap').textContent = (data.heap / 1024).toFixed(1) + ' KB';
                    document.getElementById('txpower').textContent = data.tx_power + ' dBm';
                    document.getElementById('cpu_freq').textContent = data.cpu_freq + ' MHz';
                    document.getElementById('tcprexmit').textContent = data.tcp_rexmit;
                    document.getElementById('disconnects').textContent = data.disconnects;
                })
                .catch(e => console.error('Status error:', e));

            fetch('/api/clients')
                .then(res => res.json())
                .then(data => {
                    document.getElementById('clientCount').textContent = data.length;
                    if(data.length > 0) {
                        // Show RSSI of the first connected client (likely the tester)
                        document.getElementById('clientRssi').textContent = data[0].rssi + ' dBm';
                    } else {
                        document.getElementById('clientRssi').textContent = '--';
                    }
                })
                .catch(e => console.error('Clients error:', e));
        }

        function formatUptime(seconds) {
            const h = Math.floor(seconds / 3600);
            const m = Math.floor((seconds % 3600) / 60);
            const s = seconds % 60;
            return `${h}h ${m}m ${s}s`;
        }

        function scanNetworks() {
            const btn = document.getElementById('scanBtn');
            const tbody = document.getElementById('wifiList');
            const msg = document.getElementById('statusMsg');
            
            btn.disabled = true;
            btn.textContent = 'Scanning...';
            msg.textContent = 'Please wait...';
            tbody.innerHTML = '<tr><td colspan="4" style="text-align:center">Scanning in progress...</td></tr>';

            fetch('/api/scan')
                .then(res => res.json())
                .then(data => {
                    tbody.innerHTML = '';
                    if (data.length === 0) {
                        tbody.innerHTML = '<tr><td colspan="4" style="text-align:center">No networks found</td></tr>';
                    } else {
                        data.forEach(net => {
                            const rssiClass = net.rssi > -70 ? 'badge-good' : 'badge-weak';
                            const row = `<tr>
                                <td><strong>${net.ssid}</strong></td>
                                <td><span class="badge ${rssiClass}">${net.rssi} dBm</span></td>
                                <td>${net.channel}</td>
                                <td>${net.auth}</td>
                            </tr>`;
                            tbody.innerHTML += row;
                        });
                    }
                    msg.textContent = `Found ${data.length} networks`;
                })
                .catch(e => {
                    console.error('Scan error:', e);
                    tbody.innerHTML = '<tr><td colspan="4" style="text-align:center; color:red">Scan failed</td></tr>';
                    msg.textContent = 'Error occurred';
                })
                .finally(() => {
                    btn.disabled = false;
                    btn.textContent = 'Scan Nearby Networks';
                });
        }

        async function runPingTest() {
            const btn = document.getElementById('btnPing');
            const out = document.getElementById('testResults');
            btn.disabled = true;
            out.style.display = 'block';
            out.textContent = 'Running Latency Test (20 packets)...\n';
            
            let times = [];
            let lost = 0;
            const count = 20;

            for(let i=0; i<count; i++) {
                const start = performance.now();
                try {
                    await fetch('/api/ping', {cache: "no-store"});
                    const rtt = performance.now() - start;
                    times.push(rtt);
                    out.textContent += `Seq=${i+1}: ${rtt.toFixed(2)} ms\n`;
                } catch(e) {
                    lost++;
                    out.textContent += `Seq=${i+1}: LOST\n`;
                }
                // Small delay to prevent flooding
                await new Promise(r => setTimeout(r, 100));
            }

            if (times.length > 0) {
                const avg = times.reduce((a, b) => a + b, 0) / times.length;
                const min = Math.min(...times);
                const max = Math.max(...times);
                out.textContent += `\n--- Results ---\nPackets: ${count}, Lost: ${lost} (${(lost/count*100).toFixed(1)}%)\nRTT Min/Avg/Max: ${min.toFixed(2)} / ${avg.toFixed(2)} / ${max.toFixed(2)} ms`;
            } else {
                out.textContent += `\nAll packets lost.`;
            }
            btn.disabled = false;
        }

        async function runSpeedTest() {
            const btn = document.getElementById('btnSpeed');
            const out = document.getElementById('testResults');
            btn.disabled = true;
            out.style.display = 'block';
            out.textContent = 'Running Download Speed Test (1 MB)...\n';

            const sizeBytes = 1024 * 1024; // 1MB
            const start = performance.now();
            
            try {
                const res = await fetch('/api/download?size=' + sizeBytes);
                const blob = await res.blob(); // Read body
                const durationSec = (performance.now() - start) / 1000;
                const bits = sizeBytes * 8;
                const mbps = (bits / durationSec) / (1024 * 1024);
                
                out.textContent += `Transferred: ${(sizeBytes/1024).toFixed(0)} KB\nTime: ${durationSec.toFixed(2)} s\nSpeed: ${mbps.toFixed(2)} Mbps`;
            } catch(e) {
                out.textContent += `Error: ${e.message}`;
            }
            btn.disabled = false;
        }

        async function runUploadTest() {
            const btn = document.getElementById('btnUpload');
            const out = document.getElementById('testResults');
            btn.disabled = true;
            out.style.display = 'block';
            out.textContent = 'Running Upload Speed Test (1 MB)...\n';

            const sizeBytes = 1024 * 1024; // 1MB
            const data = new Uint8Array(sizeBytes).fill(0xAA);
            const blob = new Blob([data]);
            const formData = new FormData();
            formData.append("file", blob, "test.bin");

            const start = performance.now();
            try {
                await fetch('/api/upload', { method: 'POST', body: formData });
                const durationSec = (performance.now() - start) / 1000;
                const bits = sizeBytes * 8;
                const mbps = (bits / durationSec) / (1024 * 1024);
                out.textContent += `Transferred: ${(sizeBytes/1024).toFixed(0)} KB\nTime: ${durationSec.toFixed(2)} s\nSpeed: ${mbps.toFixed(2)} Mbps`;
            } catch(e) {
                out.textContent += `Error: ${e.message}`;
            }
            btn.disabled = false;
        }

        // Initial load
        updateStatus();
        // Poll status every 2 seconds
        setInterval(updateStatus, 2000);
    </script>
</body>
</html>
)rawliteral";

// -------------------------------------------------------------------------
// Helper Functions
// -------------------------------------------------------------------------

/**
 * @brief Convert WiFi encryption type to string
 */
String translateEncryptionType(wifi_auth_mode_t encryptionType) {
    switch (encryptionType) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
        default: return "Unknown";
    }
}

/**
 * @brief WiFi Event Handler
 * Tracks disconnections to monitor stability
 */
void onWiFiEvent(WiFiEvent_t event) {
    if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED || event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        disconnect_count++;
    }
}

// -------------------------------------------------------------------------
// Request Handlers
// -------------------------------------------------------------------------

/**
 * @brief Serve the main HTML page
 */
void handleRoot() {
    server.send(200, "text/html", index_html);
}

/**
 * @brief API Endpoint: Get System Status
 * Returns JSON: { "ip": "...", "mac": "...", "uptime": 123 }
 */
void handleStatus() {
    int8_t power = 0;
    esp_wifi_get_max_tx_power(&power);
    String json = "{";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"tx_power\":" + String(power * 0.25) + ",";
    json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz());
    
    #if LWIP_STATS && LWIP_TCP
    json += ",\"tcp_rexmit\":" + String(lwip_stats.tcp.rexmit);
    #endif
    json += ",\"disconnects\":" + String(disconnect_count);
    json += "}";
    server.send(200, "application/json", json);
}

/**
 * @brief API Endpoint: Scan WiFi Networks
 * Performs a blocking scan and returns JSON array of networks
 */
void handleScan() {
    // Perform scan
    int n = WiFi.scanNetworks();
    
    String json = "[";
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"channel\":" + String(WiFi.channel(i)) + ",";
        json += "\"auth\":\"" + translateEncryptionType(WiFi.encryptionType(i)) + "\"";
        json += "}";
    }
    json += "]";
    
    server.send(200, "application/json", json);
    WiFi.scanDelete(); // Clean up RAM
}

/**
 * @brief API Endpoint: Ping
 * Used for latency testing. Returns simple timestamp.
 */
void handlePing() {
    server.send(200, "text/plain", String(millis()));
}

/**
 * @brief API Endpoint: Download Test
 * Sends a stream of dummy data to test throughput.
 * Query Param: size (bytes), default 1MB
 */
void handleDownload() {
    size_t size = 1024 * 1024; // Default 1MB
    if (server.hasArg("size")) {
        size = server.arg("size").toInt();
    }

    // Send headers
    server.setContentLength(size);
    server.send(200, "application/octet-stream", "");

    // Send data in chunks
    uint8_t buf[4096]; // Use a larger buffer (4K) for better performance
    memset(buf, 0xAA, sizeof(buf)); 
    
    size_t sent = 0;
    WiFiClient client = server.client();
    client.setNoDelay(true); // Disable Nagle Algorithm for lower latency
    
    while (sent < size && client.connected()) {
        size_t toSend = (size - sent) > sizeof(buf) ? sizeof(buf) : (size - sent);
        
        // Capture actual bytes written. If buffer is full, this returns 0.
        size_t written = client.write(buf, toSend);
        
        if (written > 0) {
            sent += written;
        } else {
            // Buffer is full, give the LwIP stack a moment to drain
            delay(1);
        }
    }
}

/**
 * @brief API Endpoint: Upload Test
 * Receives data to test upload throughput.
 */
void handleUpload() {
    server.send(200, "text/plain", "OK");
}

/**
 * @brief API Endpoint: Get Connected Clients (AP Mode)
 * Returns JSON array of connected stations with RSSI.
 */
void handleClients() {
    wifi_sta_list_t wifi_sta_list;
    esp_wifi_ap_get_sta_list(&wifi_sta_list);

    String json = "[";
    for (int i = 0; i < wifi_sta_list.num; i++) {
        if (i > 0) json += ",";
        json += "{";
        
        // Format MAC
        char macStr[18];
        uint8_t *mac = wifi_sta_list.sta[i].mac;
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        json += "\"mac\":\"" + String(macStr) + "\",";
        json += "\"rssi\":" + String(wifi_sta_list.sta[i].rssi);
        json += "}";
    }
    json += "]";
    server.send(200, "application/json", json);
}

/**
 * @brief Handle 404 errors
 */
void handleNotFound() {
    server.send(404, "text/plain", "404: Not Found");
}

// -------------------------------------------------------------------------
// Main Setup & Loop
// -------------------------------------------------------------------------

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n--- ESP32 WiFi Tester Starting ---");

    // Register Event Handler
    WiFi.onEvent(onWiFiEvent);

    // Set WiFi Mode to AP_STA (Access Point + Station)
    // Station mode is required for scanning to work properly while AP is active
    WiFi.mode(WIFI_AP_STA);

    // Disable WiFi Power Save Mode to maximize throughput
    WiFi.setSleep(false);

    // Configure Access Point
    Serial.print("Setting up Access Point... ");
    if (WiFi.softAP(AP_SSID, AP_PASS)) {
        Serial.println("Success");
        Serial.print("AP IP Address: ");
        Serial.println(WiFi.softAPIP());
        Serial.print("SSID: ");
        Serial.println(AP_SSID);
    } else {
        Serial.println("Failed!");
    }

    // Setup Web Server Routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/scan", HTTP_GET, handleScan);
    server.on("/api/ping", HTTP_GET, handlePing);
    server.on("/api/download", HTTP_GET, handleDownload);
    server.on("/api/upload", HTTP_POST, handleUpload, []() { server.upload(); });
    server.on("/api/clients", HTTP_GET, handleClients);
    server.onNotFound(handleNotFound);

    // Start Server
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    // Handle incoming client requests
    server.handleClient();
    
    // Add a small delay to prevent CPU hogging if needed, 
    // though handleClient is usually sufficient.
    delay(2); 
}