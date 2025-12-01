// RFID
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <FS.h>
#include <SPIFFS.h>

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Wi-Fi + data/hora
#include <WiFi.h>
#include <time.h>

// MQTT
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Variáveis das LEDs e Arquivos
#define LED_RED     27  
#define LED_GREEN    4  
#define LED_YELLOW  22  

const char* CARDS_FILE         = "/usuarios.txt";
const char* ADMINS_FILE        = "/funcionarios.txt";
const char* MOVIMENTACOES_FILE = "/movimentacoes.txt";

// Configuração de WiFi e de Fuso
#define WIFI_SSID "iPhone de Gabriel Henriques"
#define WIFI_PASS "bellibelli"

const char* NTP_SERVER      = "pool.ntp.org";
const long  GMT_OFFSET_SEC  = -3 * 3600;
const int   DST_OFFSET_SEC  = 0;

// Definições da RC522
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

// Fila e semáforo (fila não usada no fluxo atual, mas deixada aqui)
QueueHandle_t filaCartoes = NULL;
SemaphoreHandle_t semAcessoLiberado = NULL;

// --------- ESTADO DE MODO / ENTRADA / SAÍDA ---------
enum TipoOperacao {
  MODO_ENTRADA,
  MODO_SAIDA
};

TipoOperacao modoAtual = MODO_ENTRADA;

// ENTRADA: primeiro cartão = usuário, depois funcionário
bool   aguardandoSegundoEntrada   = false;
String uidUsuarioEntradaPendente;

// SAÍDA: primeiro cartão = funcionário, depois usuário
bool   aguardandoSegundoSaida     = false;
String uidFuncionarioSaidaPendente;

// Só processa leitura de cartão quando TRUE
bool leituraHabilitada = false;

// --------- MQTT CONFIG ---------
const char* MQTT_BROKER       = "172.20.10.10";   // IP do PC com o broker
const uint16_t MQTT_PORT      = 1884;
const char* MQTT_CLIENT_ID    = "esp32-portaria-01";
const char* MQTT_TOPIC_MOV    = "portaria/movimentacoes";  // eventos de entrada/saida
const char* MQTT_TOPIC_CMD    = "portaria/comandos";       // comandos vindos do React
const char* MQTT_TOPIC_STATUS = "portaria/status";         // msgs de status/resposta

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --------- Funções auxiliares ----------

String uidToString(const MFRC522::Uid& uid) {
  String s = "";
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) s += "0";
    s += String(uid.uidByte[i], HEX);
  }
  s.toLowerCase();
  return s;
}

bool appendLine(const char* path, const String& line) {
  File f = SPIFFS.open(path, FILE_APPEND);
  if (!f) return false;
  bool ok = (f.print(line) && f.print("\n"));
  f.close();
  return ok;
}

void listRegistered(const char* fileName) {
  File f = SPIFFS.open(fileName, FILE_READ);
  if (!f) {
    Serial.print("Nenhum arquivo ainda (");
    Serial.print(fileName);
    Serial.println(" nao existe).");
    return;
  }
  Serial.print("== UIDs cadastrados em ");
  Serial.print(fileName);
  Serial.println(" ==");
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length()) Serial.println(line);
  }
  f.close();
  Serial.println("== fim ==");
}

// conta e lista os UIDs cadastrados
size_t countRegisteredAndShow(const char* fileName) {
  File f = SPIFFS.open(fileName, FILE_READ);
  if (!f) {
    Serial.println("0");
    return 0;
  }

  size_t count = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length()) {
      count++;
    }
  }
  f.close();

  Serial.println(count);

  if (count == 0) {
    return 0;
  }

  File f2 = SPIFFS.open(fileName, FILE_READ);
  if (!f2) {
    return count;
  }

  while (f2.available()) {
    String line = f2.readStringUntil('\n');
    line.trim();
    if (line.length()) {
      Serial.println(line);
    }
  }
  f2.close();

  return count;
}

// Lê linha "-FUNC- recebeu/liberou -USER- às -HH:MM:SS- do dia -DD/MM/AAAA-"
bool parseMovLine(const String &line,
                  String &uidFunc, String &uidUser,
                  String &hora, String &data)
{
  String s = line;
  s.trim();
  if (!s.length()) return false;

  int firstDash  = s.indexOf('-');
  if (firstDash != 0) return false;
  int secondDash = s.indexOf('-', firstDash + 1);
  if (secondDash < 0) return false;
  uidFunc = s.substring(firstDash + 1, secondDash);
  uidFunc.trim();

  const String padRecebeu = " recebeu -";
  const String padLiberou = " liberou -";

  int idxTok  = s.indexOf(padRecebeu, secondDash);
  int lenTok  = padRecebeu.length();

  if (idxTok < 0) {
    idxTok = s.indexOf(padLiberou, secondDash);
    lenTok = padLiberou.length();
  }
  if (idxTok < 0) return false;

  int uidUserStart = idxTok + lenTok;
  int uidUserEnd   = s.indexOf('-', uidUserStart);
  if (uidUserEnd < 0) return false;
  uidUser = s.substring(uidUserStart, uidUserEnd);
  uidUser.trim();

  const String padHora = " às -";
  int idxHora = s.indexOf(padHora, uidUserEnd);
  if (idxHora < 0) return false;
  int horaStart = idxHora + padHora.length();
  int horaEnd   = s.indexOf('-', horaStart);
  if (horaEnd < 0) return false;
  hora = s.substring(horaStart, horaEnd);
  hora.trim();

  const String padData = " do dia -";
  int idxData = s.indexOf(padData, horaEnd);
  if (idxData < 0) return false;
  int dataStart = idxData + padData.length();
  int dataEnd   = s.indexOf('-', dataStart);
  if (dataEnd < 0) return false;
  data = s.substring(dataStart, dataEnd);
  data.trim();

  return true;
}

// Envia todo o histórico via MQTT
void publishMovHistoryToMQTT() {
  if (!mqttClient.connected()) {
    Serial.println("MQTT: nao conectado, nao envia historico.");
    return;
  }

  File f = SPIFFS.open(MOVIMENTACOES_FILE, FILE_READ);
  if (!f) {
    Serial.println("Nenhum arquivo de movimentacoes para enviar.");
    return;
  }

  Serial.println("Enviando historico de movimentacoes via MQTT...");

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) continue;

    String func, user, hora, data;
    if (!parseMovLine(line, func, user, hora, data)) {
      Serial.println("Linha de movimentacao em formato inesperado, ignorando:");
      Serial.println(line);
      continue;
    }

    String payload = "{";
    payload += "\"funcionario\":\"" + func + "\",";
    payload += "\"usuario\":\""     + user + "\",";
    payload += "\"data\":\""        + data + "\",";
    payload += "\"hora\":\""        + hora + "\"";
    payload += "}";

    bool ok = mqttClient.publish(MQTT_TOPIC_MOV, payload.c_str());
    if (!ok) {
      Serial.println("MQTT: falha ao publicar linha de historico.");
    }
    delay(10);
  }

  f.close();
  Serial.println("Historico enviado.");
}

// Lista movimentações na Serial
void listMovimentacoes() {
  File f = SPIFFS.open(MOVIMENTACOES_FILE, FILE_READ);
  if (!f) {
    Serial.print("Nenhum arquivo de movimentacoes ainda (");
    Serial.print(MOVIMENTACOES_FILE);
    Serial.println(" nao existe).");
    return;
  }

  Serial.println("== Movimentacoes registradas ==");
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length()) {
      Serial.println(line);
    }
  }
  f.close();
  Serial.println("== fim das movimentacoes ==");
}

// Verifica se UID está em um arquivo (usuarios ou funcionarios)
bool isRegistered(const char* fileName, const String &uid) {
  File f = SPIFFS.open(fileName, FILE_READ);
  if (!f) return false;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    line.toLowerCase();
    if (line.length() && line == uid) {
      f.close();
      return true;
    }
  }
  f.close();
  return false;
}

// Remove UID de um arquivo
static bool tryRemoveUidFrom(const char* path, const String& uidNorm) {
  File f = SPIFFS.open(path, FILE_READ);
  if (!f) {
    Serial.printf("Aviso: arquivo %s nao encontrado.\n", path);
    return false;
  }

  String newContent = "";
  bool found = false;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) continue;

    String cmp = line;
    cmp.toLowerCase();
    if (cmp == uidNorm) {
      found = true;
    } else {
      newContent += line + "\n";
    }
  }
  f.close();

  if (!found) return false;

  File w = SPIFFS.open(path, FILE_WRITE);
  if (!w) {
    Serial.printf("Erro ao abrir %s para sobrescrever.\n", path);
    return false;
  }
  w.print(newContent);
  w.close();

  Serial.printf("✅ UID removido de %s com sucesso!\n", path);
  return true;
}

// Deleta UID de usuarios ou funcionarios
bool deleteCard(const String &uidToRemoveRaw) {
  String uid = uidToRemoveRaw;
  uid.trim();
  uid.toLowerCase();

  if (tryRemoveUidFrom(CARDS_FILE, uid)) {
    return true;
  }

  if (tryRemoveUidFrom(ADMINS_FILE, uid)) {
    return true;
  }

  Serial.println("UID nao encontrado em CARDS_FILE nem em ADMINS_FILE.");
  return false;
}

// Cadastro de cartão
void registerCard(const char* fileName, const char* tipoCadastro) {
  Serial.print("\n[CADASTRO] Aproxime um cartao para cadastrar no arquivo ");
  Serial.println(fileName);
  unsigned long t0 = millis();

  if (mqttClient.connected()) {
    String payload = "{";
    payload += "\"context\":\"cadastro\",";
    payload += "\"event\":\"cadastro_start\",";
    payload += "\"status\":\"waiting\",";
    payload += "\"tipo\":\"";     payload += tipoCadastro; payload += "\"";
    payload += "}";
    mqttClient.publish(MQTT_TOPIC_STATUS, payload.c_str());
  }

  while (true) {
    if (millis() - t0 > 10000) {
      Serial.println("[CADASTRO] Tempo esgotado (10s). Cancelado.");
      digitalWrite(LED_RED, HIGH); delay(200);
      digitalWrite(LED_RED, LOW);

      if (mqttClient.connected()) {
        String payload = "{";
        payload += "\"context\":\"cadastro\",";
        payload += "\"event\":\"cadastro_timeout\",";
        payload += "\"status\":\"error\",";
        payload += "\"tipo\":\"";     payload += tipoCadastro; payload += "\"";
        payload += "}";
        mqttClient.publish(MQTT_TOPIC_STATUS, payload.c_str());
      }

      return;
    }

    if (!mfrc522.PICC_IsNewCardPresent()) { delay(50); continue; }
    if (!mfrc522.PICC_ReadCardSerial())   { delay(50); continue; }

    String uidString = uidToString(mfrc522.uid);
    Serial.print("[CADASTRO] UID lido: ");
    Serial.println(uidString);

    // 1) JÁ CADASTRADO -> LED AMARELO + status "exists"
    if (isRegistered(fileName, uidString)) {
      Serial.println("[CADASTRO] UID já cadastrado nesse arquivo.");
      digitalWrite(LED_YELLOW, HIGH); delay(300);
      digitalWrite(LED_YELLOW, LOW);

      if (mqttClient.connected()) {
        String payload = "{";
        payload += "\"context\":\"cadastro\",";
        payload += "\"event\":\"cadastro_already_registered\",";
        payload += "\"status\":\"exists\",";
        payload += "\"tipo\":\"";     payload += tipoCadastro; payload += "\",";
        payload += "\"uid\":\"";      payload += uidString;    payload += "\"";
        payload += "}";
        mqttClient.publish(MQTT_TOPIC_STATUS, payload.c_str());
      }
    }
    // 2) NOVO -> grava no arquivo, LED VERDE + status "success"
    else {
      bool ok = appendLine(fileName, uidString);
      if (ok) {
        Serial.print("[CADASTRO] Salvo em ");
        Serial.println(fileName);

        digitalWrite(LED_GREEN, HIGH); delay(400);
        digitalWrite(LED_GREEN, LOW);

        if (mqttClient.connected()) {
          String payload = "{";
          payload += "\"context\":\"cadastro\",";
          payload += "\"event\":\"cadastro_success\",";
          payload += "\"status\":\"success\",";
          payload += "\"tipo\":\"";     payload += tipoCadastro; payload += "\",";
          payload += "\"uid\":\"";      payload += uidString;    payload += "\"";
          payload += "}";
          mqttClient.publish(MQTT_TOPIC_STATUS, payload.c_str());
        }
      } else {
        Serial.println("[CADASTRO] ERRO ao salvar no arquivo.");
        digitalWrite(LED_RED, HIGH); delay(400);
        digitalWrite(LED_RED, LOW);

        if (mqttClient.connected()) {
          String payload = "{";
          payload += "\"context\":\"cadastro\",";
          payload += "\"event\":\"cadastro_error\",";
          payload += "\"status\":\"error\",";
          payload += "\"tipo\":\"";       payload += tipoCadastro; payload += "\",";
          payload += "\"reason\":\"fs_write_failed\"";
          payload += "}";
          mqttClient.publish(MQTT_TOPIC_STATUS, payload.c_str());
        }
      }
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    break;
  }
}

// Wi-Fi + NTP
void initWiFi() {
  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha ao conectar no WiFi. Hora NTP/MQTT podem nao funcionar.");
  }
}

void initTime() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Aviso: sem WiFi, nao sera possivel sincronizar NTP.");
    return;
  }

  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
  Serial.println("Sincronizando hora com NTP...");

  struct tm timeinfo;
  int tentativas = 0;
  while (!getLocalTime(&timeinfo) && tentativas < 10) {
    Serial.println("Aguardando sincronizacao de tempo...");
    delay(1000);
    tentativas++;
  }

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter hora via NTP.");
    return;
  }

  Serial.print("Hora atual (Brasil): ");
  Serial.println(&timeinfo, "%d/%m/%Y %H:%M:%S");
}

// Helpers de data/hora
bool obterDataHoraAtual(String &dataStr, String &horaStr) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter data/hora do sistema (sem NTP?).");
    return false;
  }

  char bufData[11];
  char bufHora[9];

  strftime(bufData, sizeof(bufData), "%d/%m/%Y", &timeinfo);
  strftime(bufHora, sizeof(bufHora), "%H:%M:%S", &timeinfo);

  dataStr = String(bufData);
  horaStr = String(bufHora);
  return true;
}

// Atrasados depois de 08:15 (primeira ENTRADA do dia)
size_t listarAtrasosDepoisDe815() {
  const String HORA_LIMITE = "08:15:00";

  String dataHoje, horaAgora;
  if (!obterDataHoraAtual(dataHoje, horaAgora)) {
    Serial.println("0");
    return 0;
  }

  File f = SPIFFS.open(MOVIMENTACOES_FILE, FILE_READ);
  if (!f) {
    Serial.println("0");
    return 0;
  }

  const int MAX_UIDS_DIA = 128;
  String uidsDia[MAX_UIDS_DIA];
  String horasPrimeira[MAX_UIDS_DIA];
  int numUidsDia = 0;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) continue;

    String func, user, horaLinha, dataLinha;
    if (!parseMovLine(line, func, user, horaLinha, dataLinha)) {
      continue;
    }

    if (dataLinha != dataHoje) continue;

    // Só entradas (recebeu)
    if (line.indexOf(" recebeu -") < 0) continue;

    String uidUsuario = user;
    uidUsuario.trim();
    uidUsuario.toLowerCase();

    if (!isRegistered(CARDS_FILE, uidUsuario)) {
      continue;
    }

    int idx = -1;
    for (int i = 0; i < numUidsDia; i++) {
      if (uidsDia[i] == uidUsuario) {
        idx = i;
        break;
      }
    }

    if (idx == -1) {
      if (numUidsDia < MAX_UIDS_DIA) {
        uidsDia[numUidsDia]      = uidUsuario;
        horasPrimeira[numUidsDia] = horaLinha;
        numUidsDia++;
      }
    }
  }

  f.close();

  size_t totalAtrasados = 0;
  for (int i = 0; i < numUidsDia; i++) {
    if (horasPrimeira[i] > HORA_LIMITE) {
      totalAtrasados++;
    }
  }

  Serial.println(totalAtrasados);
  if (totalAtrasados == 0) {
    return 0;
  }

  for (int i = 0; i < numUidsDia; i++) {
    if (horasPrimeira[i] > HORA_LIMITE) {
      Serial.println(uidsDia[i]);
    }
  }

  return totalAtrasados;
}

// Usuários que entraram e não saíram (contagem ímpar no dia)
size_t listarUsuariosDentroHoje() {
  String dataHoje, horaAgora;
  if (!obterDataHoraAtual(dataHoje, horaAgora)) {
    Serial.println("0");
    return 0;
  }

  File f = SPIFFS.open(MOVIMENTACOES_FILE, FILE_READ);
  if (!f) {
    Serial.println("0");
    return 0;
  }

  const int MAX_UIDS_DIA = 128;
  String uidsDia[MAX_UIDS_DIA];
  int contagens[MAX_UIDS_DIA];
  int numUidsDia = 0;

  for (int i = 0; i < MAX_UIDS_DIA; i++) {
    contagens[i] = 0;
  }

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) continue;

    String func, user, horaLinha, dataLinha;
    if (!parseMovLine(line, func, user, horaLinha, dataLinha)) {
      continue;
    }

    if (dataLinha != dataHoje) continue;

    String uidUsuario = user;
    uidUsuario.trim();
    uidUsuario.toLowerCase();

    if (!isRegistered(CARDS_FILE, uidUsuario)) {
      continue;
    }

    int idx = -1;
    for (int i = 0; i < numUidsDia; i++) {
      if (uidsDia[i] == uidUsuario) {
        idx = i;
        break;
      }
    }

    if (idx == -1) {
      if (numUidsDia < MAX_UIDS_DIA) {
        uidsDia[numUidsDia] = uidUsuario;
        contagens[numUidsDia] = 1;
        numUidsDia++;
      }
    } else {
      contagens[idx]++;
    }
  }

  f.close();

  size_t totalDentro = 0;
  for (int i = 0; i < numUidsDia; i++) {
    if (contagens[i] % 2 == 1) {
      totalDentro++;
    }
  }

  Serial.println(totalDentro);
  if (totalDentro == 0) {
    return 0;
  }

  for (int i = 0; i < numUidsDia; i++) {
    if (contagens[i] % 2 == 1) {
      Serial.println(uidsDia[i]);
    }
  }

  return totalDentro;
}

void reconnectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("MQTT: sem WiFi, nao conecta no broker.");
    return;
  }

  while (!mqttClient.connected()) {
    Serial.print("Conectando ao broker MQTT em ");
    Serial.print(MQTT_BROKER);
    Serial.print(":");
    Serial.println(MQTT_PORT);

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("MQTT conectado!");
      mqttClient.subscribe(MQTT_TOPIC_CMD);
      Serial.print("Inscrito em ");
      Serial.println(MQTT_TOPIC_CMD);
    } else {
      Serial.print("Falha na conexao MQTT, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" tentando novamente em 2s...");
      delay(2000);
    }
  }
}

// MQTT callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT mensagem recebida em [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) != MQTT_TOPIC_CMD) return;

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.println("JSON invalido no comando MQTT");
    return;
  }

  const char* cmd  = doc["cmd"];
  const char* tipo = doc["tipo"];

  if (!cmd) return;

  if (strcmp(cmd, "get_history") == 0) {
    Serial.println("Comando MQTT: get_history -> enviando arquivo de movimentacoes");
    publishMovHistoryToMQTT();
    return;
  }

  if (strcmp(cmd, "start_register") == 0 && tipo) {

    if (mqttClient.connected()) {
      String payloadStatus = String("{\"context\":\"cadastro\",\"tipo\":\"") +
                             tipo + "\",\"status\":\"waiting\"}";
      mqttClient.publish(MQTT_TOPIC_STATUS, payloadStatus.c_str());
    }

    if (strcmp(tipo, "parent") == 0) {
      registerCard(CARDS_FILE, "parent");
    } else if (strcmp(tipo, "employee") == 0) {
      registerCard(ADMINS_FILE, "employee");
    }
  }
}

// Registrar movimentação
void registrarMovimentacao(const String &uidFuncionario,
                           const String &uidUsuario,
                           const String &tipoMov) {
  String dataStr, horaStr;
  if (!obterDataHoraAtual(dataStr, horaStr)) {
    dataStr = "data_indisponivel";
    horaStr = "hora_indisponivel";
  }

  String linha;
  String tipo = tipoMov;
  tipo.toLowerCase();

  if (tipo == "entrada") {
    linha = "-" + uidFuncionario + "- recebeu -" + uidUsuario +
            "- às -" + horaStr + "- do dia -" + dataStr + "-";
  } else if (tipo == "saída" || tipo == "saida") {
    linha = "-" + uidFuncionario + "- liberou -" + uidUsuario +
            "- às -" + horaStr + "- do dia -" + dataStr + "-";
  } else {
    linha = "-" + uidFuncionario + "- liberou -" + uidUsuario +
            "- às -" + horaStr + "- do dia -" + dataStr + "-";
  }

  if (appendLine(MOVIMENTACOES_FILE, linha)) {
    Serial.println("Movimentacao registrada: " + linha);
  } else {
    Serial.println("ERRO ao registrar movimentacao em MOVIMENTACOES_FILE.");
  }

  if (mqttClient.connected()) {
    String payload = "{";
    payload += "\"funcionario\":\"" + uidFuncionario + "\",";
    payload += "\"usuario\":\""     + uidUsuario     + "\",";
    payload += "\"data\":\""        + dataStr        + "\",";
    payload += "\"hora\":\""        + horaStr        + "\"";
    payload += "}";

    bool ok = mqttClient.publish(MQTT_TOPIC_MOV, payload.c_str());
    if (ok) {
      Serial.println("MQTT: publicado em " + String(MQTT_TOPIC_MOV) + " -> " + payload);
    } else {
      Serial.println("MQTT: FALHA ao publicar movimentacao.");
    }
  } else {
    Serial.println("MQTT: nao conectado, movimentacao nao enviada.");
  }
}

// --------- ENTRADA ----------
// Primeiro: USUÁRIO, depois: FUNCIONÁRIO
void processarEntradaCartao(const String &uidLido) {
  String uid = uidLido;
  uid.trim();
  uid.toLowerCase();

  bool ehUsuario     = isRegistered(CARDS_FILE,  uid);
  bool ehFuncionario = isRegistered(ADMINS_FILE, uid);

  if (!aguardandoSegundoEntrada) {
    // PRIMEIRO CARTÃO: deve ser USUÁRIO
    if (!ehUsuario && !ehFuncionario) {
      Serial.println("Falha (ENTRADA): primeiro cartao nao cadastrado.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      leituraHabilitada = false;
      return;
    }

    if (ehFuncionario && !ehUsuario) {
      Serial.println("Falha (ENTRADA): primeiro cartao deve ser de USUARIO, mas e FUNCIONARIO.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      leituraHabilitada = false;
      return;
    }

    if (ehUsuario && ehFuncionario) {
      Serial.println("Falha (ENTRADA): UID em usuarios E funcionarios (configuracao invalida).");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      leituraHabilitada = false;
      return;
    }

    uidUsuarioEntradaPendente = uid;
    aguardandoSegundoEntrada  = true;

    Serial.print("ENTRADA: cartao de USUARIO OK (");
    Serial.print(uidUsuarioEntradaPendente);
    Serial.println("). Aproxime agora o cartao do FUNCIONARIO.");

    digitalWrite(LED_YELLOW, HIGH);
    delay(300);
    digitalWrite(LED_YELLOW, LOW);
    return;
  } else {
    // SEGUNDO CARTÃO: deve ser FUNCIONARIO
    if (uid == uidUsuarioEntradaPendente) {
      Serial.println("Falha (ENTRADA): mesmo cartao nao pode ser USUARIO e FUNCIONARIO.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      aguardandoSegundoEntrada = false;
      leituraHabilitada        = false;
      return;
    }

    if (!ehUsuario && !ehFuncionario) {
      Serial.println("Falha (ENTRADA): segundo cartao nao cadastrado.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      aguardandoSegundoEntrada = false;
      leituraHabilitada        = false;
      return;
    }

    if (ehUsuario && !ehFuncionario) {
      Serial.println("Falha (ENTRADA): segundo cartao deve ser FUNCIONARIO, mas e USUARIO.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      aguardandoSegundoEntrada = false;
      leituraHabilitada        = false;
      return;
    }

    if (ehUsuario && ehFuncionario) {
      Serial.println("Falha (ENTRADA): segundo UID em usuarios E funcionarios (configuracao invalida).");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      aguardandoSegundoEntrada = false;
      leituraHabilitada        = false;
      return;
    }

    String uidFuncionario = uid;
    String uidUsuario     = uidUsuarioEntradaPendente;

    aguardandoSegundoEntrada  = false;
    uidUsuarioEntradaPendente = "";

    Serial.println("✅ Combinacao valida para ENTRADA (USUARIO + FUNCIONARIO).");
    registrarMovimentacao(uidFuncionario, uidUsuario, "entrada");

    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    delay(2000);
    digitalWrite(LED_GREEN, LOW);

    xSemaphoreGive(semAcessoLiberado);
    if (xSemaphoreTake(semAcessoLiberado, 0) == pdTRUE) {
      Serial.println("Semaforo semAcessoLiberado sinalizado e consumido (entrada).");
    }

    // desabilita leituras até o próximo comando no terminal
    leituraHabilitada = false;
  }
}

// --------- SAÍDA ----------
// Primeiro: FUNCIONARIO, depois: USUARIO
void processarSaidaCartao(const String &uidLido) {
  String uid = uidLido;
  uid.trim();
  uid.toLowerCase();

  bool ehUsuario     = isRegistered(CARDS_FILE,  uid);
  bool ehFuncionario = isRegistered(ADMINS_FILE, uid);

  if (!aguardandoSegundoSaida) {
    // PRIMEIRO CARTÃO: deve ser FUNCIONARIO
    if (!ehUsuario && !ehFuncionario) {
      Serial.println("Falha (SAIDA): primeiro cartao nao cadastrado.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      leituraHabilitada = false;
      return;
    }

    if (ehUsuario && !ehFuncionario) {
      Serial.println("Falha (SAIDA): primeiro cartao deve ser FUNCIONARIO, mas e USUARIO.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      leituraHabilitada = false;
      return;
    }

    if (ehUsuario && ehFuncionario) {
      Serial.println("Falha (SAIDA): UID em usuarios E funcionarios (configuracao invalida).");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      leituraHabilitada = false;
      return;
    }

    uidFuncionarioSaidaPendente = uid;
    aguardandoSegundoSaida      = true;

    Serial.print("SAIDA: cartao de FUNCIONARIO OK (");
    Serial.print(uidFuncionarioSaidaPendente);
    Serial.println("). Aproxime agora o cartao do USUARIO.");

    digitalWrite(LED_YELLOW, HIGH);
    delay(300);
    digitalWrite(LED_YELLOW, LOW);
    return;
  } else {
    // SEGUNDO CARTÃO: deve ser USUARIO
    if (uid == uidFuncionarioSaidaPendente) {
      Serial.println("Falha (SAIDA): mesmo cartao nao pode ser FUNCIONARIO e USUARIO.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      aguardandoSegundoSaida = false;
      leituraHabilitada      = false;
      return;
    }

    if (!ehUsuario && !ehFuncionario) {
      Serial.println("Falha (SAIDA): segundo cartao nao cadastrado.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      aguardandoSegundoSaida = false;
      leituraHabilitada      = false;
      return;
    }

    if (!ehUsuario && ehFuncionario) {
      Serial.println("Falha (SAIDA): segundo cartao deve ser USUARIO, mas e FUNCIONARIO.");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      aguardandoSegundoSaida = false;
      leituraHabilitada      = false;
      return;
    }

    if (ehUsuario && ehFuncionario) {
      Serial.println("Falha (SAIDA): segundo UID em usuarios E funcionarios (configuracao invalida).");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      delay(2000);
      digitalWrite(LED_RED, LOW);
      aguardandoSegundoSaida = false;
      leituraHabilitada      = false;
      return;
    }

    String uidFuncionario = uidFuncionarioSaidaPendente;
    String uidUsuario     = uid;

    aguardandoSegundoSaida      = false;
    uidFuncionarioSaidaPendente = "";

    Serial.println("✅ Combinacao valida para SAIDA (FUNCIONARIO + USUARIO).");
    registrarMovimentacao(uidFuncionario, uidUsuario, "saída");

    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    delay(2000);
    digitalWrite(LED_GREEN, LOW);

    xSemaphoreGive(semAcessoLiberado);
    if (xSemaphoreTake(semAcessoLiberado, 0) == pdTRUE) {
      Serial.println("Semaforo semAcessoLiberado sinalizado e consumido (saida).");
    }

    // desabilita leituras até o próximo comando no terminal
    leituraHabilitada = false;
  }
}

void checkCardRegistered(const String &uidString) {
  if (isRegistered(CARDS_FILE, uidString)) {
    Serial.println("✅ Cartao cadastrado em usuarios.txt! LED VERDE...");
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    delay(2000);
    digitalWrite(LED_GREEN, LOW);
  } else {
    Serial.println("❌ Cartao NAO cadastrado em usuarios.txt! LED VERMELHO...");
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    delay(2000);
    digitalWrite(LED_RED, LOW);
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);

  if (!SPIFFS.begin(true)) {
    Serial.println("ERRO: SPIFFS nao inicializado.");
  } else {
    Serial.println("SPIFFS OK. Arquivo de cadastros: /usuarios.txt");
  }

  initWiFi();
  initTime();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  reconnectMQTT();

  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.println(F("Scan PICC to see UID"));
  Serial.println(F(
    "Comandos (via Serial por enquanto): \n"
    "'c' = cadastrar novo usuario \n"
    "'a' = cadastrar novo admin \n"
    "'l' = listar usuarios (apenas UIDs) \n"
    "'L' = listar admins (apenas UIDs) \n"
    "'u' = quantidade + UIDs de usuarios \n"
    "'f' = quantidade + UIDs de admins \n"
    "'t' = usuarios atrasados (primeira entrada apos 08:15 hoje) \n"
    "'p' = usuarios que estao dentro (registros impares hoje) \n"
    "'e' = iniciar fluxo de ENTRADA (USUARIO -> FUNCIONARIO) \n"
    "'s' = iniciar fluxo de SAIDA   (FUNCIONARIO -> USUARIO) \n"
    "'d' = deletar UID \n"
    "'m' = listar movimentacoes + enviar historico via MQTT"
  ));

  filaCartoes = xQueueCreate(8, sizeof(String));
  if (filaCartoes == NULL) {
    Serial.println("Aviso: filaCartoes nao criada (nao usada no fluxo atual).");
  }

  semAcessoLiberado = xSemaphoreCreateBinary();
  if (semAcessoLiberado == NULL) {
    Serial.println("ERRO: nao foi possivel criar semAcessoLiberado!");
  }

  Serial.println("Modo inicial: ENTRADA, mas leitura de cartoes DESABILITADA.");
  Serial.println("Use 'e' ou 's' no terminal para iniciar um fluxo de entrada/saida.");
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Comandos via Serial
  if (Serial.available()) {
    char c = Serial.read();

    if (c == 'c' || c == 'C') registerCard(CARDS_FILE, "parent");
    if (c == 'a' || c == 'A') registerCard(ADMINS_FILE, "employee");

    if (c == 'l')             listRegistered(CARDS_FILE);
    if (c == 'L')             listRegistered(ADMINS_FILE);
    if (c == 'u' || c == 'U') countRegisteredAndShow(CARDS_FILE);
    if (c == 'f' || c == 'F') countRegisteredAndShow(ADMINS_FILE);
    if (c == 't' || c == 'T') listarAtrasosDepoisDe815();
    if (c == 'p' || c == 'P') listarUsuariosDentroHoje();

    if (c == 'm' || c == 'M') {
      listMovimentacoes();
      publishMovHistoryToMQTT();
    }

    if (c == 'd' || c == 'D') {
      Serial.println("Digite o UID a deletar:");
      while (!Serial.available()) delay(10);
      String uid = Serial.readStringUntil('\n');
      uid.trim();
      uid.toLowerCase();
      deleteCard(uid);
    }

    if (c == 'e' || c == 'E') {
      modoAtual = MODO_ENTRADA;
      aguardandoSegundoEntrada = false;
      aguardandoSegundoSaida   = false;
      leituraHabilitada        = true;
      Serial.println("Fluxo de ENTRADA iniciado. Aproxime o cartao do USUARIO.");
    }

    if (c == 's' || c == 'S') {
      modoAtual = MODO_SAIDA;
      aguardandoSegundoEntrada = false;
      aguardandoSegundoSaida   = false;
      leituraHabilitada        = true;
      Serial.println("Fluxo de SAIDA iniciado. Aproxime o cartao do FUNCIONARIO.");
    }
  }

  // Leitura de cartões só acontece se habilitada pelo terminal
  if (!leituraHabilitada) return;

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial())   return;

  Serial.print("Card UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();

  String uidString = uidToString(mfrc522.uid);
  Serial.print("UID em texto: ");
  Serial.println(uidString);

  if (modoAtual == MODO_ENTRADA) {
    processarEntradaCartao(uidString);
  } else {
    processarSaidaCartao(uidString);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
