/*
Para testar a simulação, abra o endereço http://localhost:9080
O gateway IoT requer a assinatura do clube Wokwi
*/
#include "SSD1306.h"
#include <driver/adc.h>
SSD1306 display(0x3c, 5, 4);
#include <WiFi.h>
#include <WebServer.h>
const char* ssid = "Wokwi-GUEST";
const char* password = "";
WebServer server(80);
#include <DHT.h>
#define DHTPIN 13
DHT dht(DHTPIN, DHT22);
char sTemperatura[7];
char sUmidade[7];
unsigned long lastMsg = 0;
const int intervaloMedicao = 2000;
unsigned long now;
const int pinLED = 2;
bool statusLED = LOW;
void mostrarTexto(int col, int lin, int tam, String txt) {
if (tam==0) {
display.setFont(ArialMT_Plain_10);
} else if (tam==1) {
display.setFont(ArialMT_Plain_16);
} else {
display.setFont(ArialMT_Plain_24);
}
display.drawString(col, lin, txt);
}
void mostrarDados(boolean mostraUmidade=true) {
Serial.printf("Temperatura: %s °C\n", sTemperatura);
if (mostraUmidade) {
Serial.printf("Umidade: %s%%\n", sUmidade);
}
Serial.println();
display.clear();
mostrarTexto( 0, 0, 1, "Temp.: " + (String)sTemperatura + " °C");
if (mostraUmidade) {
mostrarTexto( 0, 25, 1, "Umid.: " + (String)sUmidade + "%");
}
mostrarTexto(10, 50, 0, "Cloud & IoT - Unoesc");
display.display();
}
void medirTemperaturaUmidade() {
// Sensor DHT ou nenhum sensor
float temperatura = trunc(dht.readTemperature());
float umidade = trunc(dht.readHumidity());
if (isnan(temperatura) || isnan(umidade)) {
Serial.println("Erro de leitura");
temperatura = random(0, 45);
umidade = random(0, 100);
}
dtostrf(temperatura, 2, 0, sTemperatura);
dtostrf(umidade, 2, 0, sUmidade);
}
void manipularSensor() {
medirTemperaturaUmidade();
mostrarDados();
String strJson = "{\"temperatura\": ";
strJson += sTemperatura;
strJson += ", \"umidade\":";
strJson += sUmidade;
strJson += "}";
server.send(200, "application/json", strJson);
}
void enviarHTML(bool statusLED) {
Serial.print("Estado do GPIO2 (LED): ");
Serial.println(statusLED ? "Ligado" : "Desligado");
digitalWrite(pinLED, statusLED);
server.send(200, "text/html", montarHTML(statusLED));
}
void handle_OnConnect() {
statusLED = LOW;
medirTemperaturaUmidade();
mostrarDados();
Serial.println("Novo cliente conectado");
enviarHTML(statusLED);
}
void handle_ledon() {
statusLED = HIGH;
enviarHTML(statusLED);
}
void handle_ledoff() {
statusLED = LOW;
enviarHTML(statusLED);
}
void handle_NotFound() {
server.send(404, "text/plain", "Página não encontrada");
}
String montarScript() {
String js = R"EOF(
<script>
setInterval(getDadosSensor, 500);
function getDadosSensor() {
var xhttp = new XMLHttpRequest();
xhttp.onreadystatechange = function() {
if (this.readyState == 4 && this.status == 200) {
let jsonResponse = JSON.parse(this.responseText);
document.getElementById("valorTemperatura").innerHTML = jsonResponse.temperatura;
document.getElementById("valorUmidade").innerHTML = jsonResponse.umidade;
}
};
xhttp.open("GET", "lesensor", true);
xhttp.send();
}
</script>
)EOF";
return js;
}
String montarHead() {
String head = R"EOF(
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<link href="https://fonts.googleapis.com/css?family=Open+Sans:300,400,600" rel="stylesheet">
<title>IoT com ESP32</title>
<style>
html {
font-family: 'Open Sans', sans-serif;
display: block;
margin: 0px auto;
text-align: center;color: #333333;
}
body {
margin-top: 50px;
}
h1 {
margin: 50px auto 30px;
}
.side-by-side {
display: inline-block;
vertical-align: middle;
position: relative;
}
.button {
display: block;
width: 80px;
background-color: #3498db;
border: none;
color: white;
padding: 13px 30px;
text-decoration: none;
font-size: 25px;
margin: 0px auto 35px;
cursor: pointer;
border-radius: 4px;
}
.button-on {
background-color: #3498db;
}
.button-on:active {
background-color: #2980b9;
}
.button-off {
background-color: #34495e;
}
.button-off:active {
background-color: #2c3e50;
}
.humidity-icon {
background-color: #3498db;
width: 30px;
height: 30px;
border-radius: 50%;
line-height: 36px;
}
.humidity-text {
font-weight: 600;
padding-left: 15px;
font-size: 19px;
width: 160px;
text-align: left;
}
.humidity {
font-weight: 300;
font-size: 60px;
color: #3498db;
}
.temperature-icon {
background-color: #f39c12;
width: 30px;
height: 30px;
border-radius: 50%;
line-height: 40px;
}
.temperature-text {
font-weight: 600;
padding-left: 15px;
font-size: 19px;
width: 160px;
text-align: left;
}
.temperature {
font-weight: 300;
font-size: 60px;
color: #f39c12;
}
.superscript {
font-size: 17px;
font-weight: 600;
position: absolute;
right: -20px;top: 15px;
}
.data {
padding: 10px;
}
</style>
)EOF";
// JavaScript com Ajax
head += montarScript();
head += "</head>\n";
return head;
}
String montarBody(bool statusLED) {
String body = R"EOF(
<body>
<div id="webpage">
<h1>Servidor Web ESP32</h1>
<h2>Sensor DHT</h2>
<div class="data">
<div class="side-by-side temperature-icon">
<svg version="1.1" id="Layer_1" xmlns="http://www.w3.org/2000/svg"
xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
width="9.915px" height="22px" viewBox="0 0 9.915 22" enable-background="new 0 0 9.915 22"
xml:space="preserve">
<path fill="#FFFFFF" d="M3.498,0.53c0.377-0.331,0.877-0.501,1.374-0.527C5.697-
0.04,6.522,0.421,6.924,1.142
c0.237,0.399,0.315,0.871,0.311,1.33C7.229,5.856,7.245,9.24,7.227,12.625c1.019,0.539,1.855,1.424
,2.301,2.491
c0.491,1.163,0.518,2.514,0.062,3.693c-0.414,1.102-1.24,2.038-2.276,2.594c-1.056,0.583-
2.331,0.743-3.501,0.463
c-1.417-0.323-2.659-1.314-3.3-2.617C0.014,18.26-0.115,17.104,0.1,16.022c0.296-1.443,1.274-
2.717,2.58-3.394
c0.013-3.44,0-6.881,0.007-10.322C2.674,1.634,2.974,0.955,3.498,0.53z"/>
</svg>
</div>
)EOF";
// Inclui o dado da temperatura na página HTML
body += "<div class=\"side-by-side temperature-text\">Temperatura</div>\n";
body += "<div class=\"side-by-side temperature\">";
body += "<span id=\"valorTemperatura\">";
body += sTemperatura;
body += "</span>";
body += "<span class=\"superscript\">&deg;C</span></div>\n";
body += "</div>\n";
body += R"EOF(
<div class="data">
<div class="side-by-side humidity-icon">
<svg version="1.1" id="Layer_2" xmlns="http://www.w3.org/2000/svg"
xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
width="12px" height="17.955px" viewBox="0 0 13 17.955" enable-background="new 0 0 13 17.955"
xml:space="preserve">
<path fill="#FFFFFF"
d="M1.819,6.217C3.139,4.064,6.5,0,6.5,0s3.363,4.064,4.681,6.217c1.793,2.926,2.133,5.05,1.571,7.057
c-0.438,1.574-2.264,4.681-6.252,4.681c-3.988,0-5.813-3.107-6.252-4.681C0.313,11.267,0.026,9.143,1.819,6.217">
</path>
</svg>
</div>
)EOF";
// Inclui o dado da umidade na página HTML
body += "<div class=\"side-by-side humidity-text\">Umidade</div>\n";
body += "<div class=\"side-by-side humidity\">";
body += "<span id=\"valorUmidade\">";
body += sUmidade;
body += "</span>";
body += "<span class=\"superscript\">%</span></div>\n";
body += "</div>\n";
if (statusLED) {
body += "<p>Estado do LED: Ligado</p><a class=\"button button-off\" href=\"/ledoff\">OFF</a>\n";
} else {
body += "<p>Estado do LED: Desligado</p><a class=\"button button-on\" href=\"/ledon\">ON</a>\n";
}
body += "</body>\n";
return body;
}
String montarHTML(bool statusLED) {
String pagina = "<!DOCTYPE html> <html>\n";
pagina += montarHead();
pagina += montarBody(statusLED);
pagina += "</html>\n";
return pagina;
}
void inicializarDisplay() {
display.init();
display.clear();
display.flipScreenVertically();
Serial.println("Display OLED inicializado!");
}
void conectarWiFi() {
Serial.println();
Serial.print("Conectando à rede WiFi: ");
Serial.println(ssid);
WiFi.begin(ssid, password, 6);
while (WiFi.status() != WL_CONNECTED) {
Serial.print(".");
delay(500);
}
Serial.println("");
Serial.print("ESP32 conectado à rede Wi-Fi ");
Serial.println(String(ssid) + "!");
Serial.print("\nEndereco MAC: ");
Serial.println(WiFi.macAddress());
Serial.print("Endereco IP: ");
Serial.println(WiFi.localIP());
Serial.print("Máscara de sub-rede: ");
Serial.println(WiFi.subnetMask());
Serial.print("Gateway: ");
Serial.println(WiFi.gatewayIP());
Serial.print("DNS 1: ");
Serial.println(WiFi.dnsIP(0));
Serial.print("DNS 2: ");
Serial.println(WiFi.dnsIP(1));
}
void configurarEndPoints() {
server.on("/", handle_OnConnect);
server.on("/ledon", handle_ledon);
server.on("/ledoff", handle_ledoff);
server.on("/lesensor", manipularSensor);
server.onNotFound(handle_NotFound);
Serial.println("Endpoints configurados!");
}
void iniciarServidorWeb() {
server.begin();
Serial.println("Servidor HTTP iniciado!");
}
void setup() {
Serial.begin(115200);
Serial.println("Inicializando componentes...");
pinMode(pinLED, OUTPUT);
// Inicializa o display OLED
inicializarDisplay();
// Conecta à rede WiFi
conectarWiFi();
// Configura endpoints
configurarEndPoints();
// Inicia servidor Web
iniciarServidorWeb();
}
void loop() {
now = millis();
// Gerencia os clientes do servidor
server.handleClient();
// Lê periodicamente o sensor com pausa não bloqueante
if (now - lastMsg > intervaloMedicao) {
lastMsg = now;
medirTemperaturaUmidade();
mostrarDados();
}
}
