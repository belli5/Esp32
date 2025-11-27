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

// Variáveis das LEDs e Arquivos
#define LED_RED     27  
#define LED_GREEN    4  
#define LED_YELLOW  22  

const char* CARDS_FILE         = "/usuarios.txt";
const char* ADMINS_FILE        = "/funcionarios.txt";
const char* MOVIMENTACOES_FILE = "/movimentacoes.txt";

// Configuração de WiFi e de Fuso
#define WIFI_SSID "iPhone de Gabriel"
#define WIFI_PASS "12345678"

const char* NTP_SERVER      = "pool.ntp.org";
const long  GMT_OFFSET_SEC  = -3 * 3600;
const int   DST_OFFSET_SEC  = 0;

// Definições da RC522
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

// Fila e semáforo + contador
QueueHandle_t filaCartoes = NULL;      // fila de String
SemaphoreHandle_t semAcessoLiberado = NULL;
uint8_t cartoesPendentes = 0;

// --------- MQTT CONFIG ---------
const char* MQTT_BROKER       = "172.20.10.2";   // IP do PC com o broker
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

// --------- conta e mostra UIDs cadastrados ----------
// Saída na Serial:
// <quantidade>
// <uid1>
// <uid2>
// ...
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

// Função que lista o arquivo de movimentações
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

// Função que verifica se um certo UID está cadastrado
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

// Função que remove um UID de um arquivo
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

// Função que procura em ambos os arquivos para deletar o UID
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

// Função que registra um UID em um arquivo
void registerCard(const char* fileName) {
  Serial.print("\n[CADASTRO] Aproxime um cartao para cadastrar no arquivo ");
  Serial.println(fileName);
  unsigned long t0 = millis();

  while (true) {
    if (millis() - t0 > 10000) {
      Serial.println("[CADASTRO] Tempo esgotado (10s). Cancelado.");
      digitalWrite(LED_RED, HIGH); delay(200);
      digitalWrite(LED_RED, LOW);
      return;
    }

    if (!mfrc522.PICC_IsNewCardPresent()) { delay(50); continue; }
    if (!mfrc522.PICC_ReadCardSerial())   { delay(50); continue; }

    String uidString = uidToString(mfrc522.uid);
    Serial.print("[CADASTRO] UID lido: ");
    Serial.println(uidString);

    if (isRegistered(fileName, uidString)) {
      Serial.println("[CADASTRO] UID já cadastrado nesse arquivo.");
      digitalWrite(LED_RED, HIGH); delay(300);
      digitalWrite(LED_RED, LOW);
    } else {
      bool ok = appendLine(fileName, uidString);
      if (ok) {
        Serial.print("[CADASTRO] Salvo em ");
        Serial.println(fileName);
        digitalWrite(LED_YELLOW, HIGH); delay(500);
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_GREEN, HIGH); delay(300);
        digitalWrite(LED_GREEN, LOW);
      } else {
        Serial.println("[CADASTRO] ERRO ao salvar no arquivo.");
        digitalWrite(LED_RED, HIGH); delay(400);
        digitalWrite(LED_RED, LOW);
      }
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    break;
  }
}

// Inicialização do Wi-Fi + NTP
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

// --------- Data/hora helpers ----------

bool obterDataHoraAtual(String &dataStr, String &horaStr) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter data/hora do sistema (sem NTP?).");
    return false;
  }

  char bufData[11];  // DD/MM/AAAA
  char bufHora[9];   // HH:MM:SS

  strftime(bufData, sizeof(bufData), "%d/%m/%Y", &timeinfo);
  strftime(bufHora, sizeof(bufHora), "%H:%M:%S", &timeinfo);

  dataStr = String(bufData);
  horaStr = String(bufHora);
  return true;
}

// --------- NOVA FUNÇÃO: atrasados após 08:15 ----------
//
// Lê o MOVIMENTACOES_FILE, considera apenas o dia atual,
// pega apenas o primeiro horário de cada UID de usuário,
// e conta/mostra quem teve primeira passagem após 08:15:00.
//
// Saída na Serial:
// <quantidade_atrasados>
// <uid1>
// <uid2>
// ...
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

    // Extrair data da linha: "... do dia -DD/MM/AAAA-"
    const String padData = " do dia -";
    int idxData = line.indexOf(padData);
    if (idxData < 0) continue;
    int dataStart = idxData + padData.length();
    int dataEnd = line.indexOf("-", dataStart);
    if (dataEnd < 0) continue;
    String dataLinha = line.substring(dataStart, dataEnd);
    dataLinha.trim();

    // Só consideramos o dia atual
    if (dataLinha != dataHoje) continue;

    // Extrair UID do usuário: "-FUNC- liberou -USUARIO- às -HH:MM:SS- ..."
    const String padLiberou = " liberou -";
    int idxLib = line.indexOf(padLiberou);
    if (idxLib < 0) continue;
    int uidUsuarioStart = idxLib + padLiberou.length();
    int uidUsuarioEnd = line.indexOf("-", uidUsuarioStart);
    if (uidUsuarioEnd < 0) continue;
    String uidUsuario = line.substring(uidUsuarioStart, uidUsuarioEnd);
    uidUsuario.trim();
    uidUsuario.toLowerCase();

    // Extrair hora: " às -HH:MM:SS-"
    const String padHora = " às -";
    int idxHora = line.indexOf(padHora);
    if (idxHora < 0) continue;
    int horaStart = idxHora + padHora.length();
    int horaEnd = line.indexOf("-", horaStart);
    if (horaEnd < 0) continue;
    String horaLinha = line.substring(horaStart, horaEnd);
    horaLinha.trim();

    // Garante que é um usuário (e não funcionário)
    if (!isRegistered(CARDS_FILE, uidUsuario)) {
      continue;
    }

    // Se ainda não temos esse UID registrado para o dia, salva a primeira hora
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
        horasPrimeira[numUidsDia] = horaLinha;
        numUidsDia++;
      }
    }
    // se já existia, ignoramos (só o primeiro registro conta)
  }

  f.close();

  // Agora verifica quem tem hora > 08:15:00
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

// --------- NOVA FUNÇÃO: usuários que entraram e não saíram ----------
//
// Lê o MOVIMENTACOES_FILE, considera apenas o dia atual,
// conta quantas vezes cada UID de usuário aparece no log.
// Se a contagem for ímpar, significa que ele está "dentro".
//
// Saída na Serial:
// <quantidade_dentro>
// <uid1>
// <uid2>
// ...
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

  // Inicializa contagens
  for (int i = 0; i < MAX_UIDS_DIA; i++) {
    contagens[i] = 0;
  }

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) continue;

    // Extrair data da linha: "... do dia -DD/MM/AAAA-"
    const String padData = " do dia -";
    int idxData = line.indexOf(padData);
    if (idxData < 0) continue;
    int dataStart = idxData + padData.length();
    int dataEnd = line.indexOf("-", dataStart);
    if (dataEnd < 0) continue;
    String dataLinha = line.substring(dataStart, dataEnd);
    dataLinha.trim();

    // Só consideramos o dia atual
    if (dataLinha != dataHoje) continue;

    // Extrair UID do usuário: "-FUNC- liberou -USUARIO- às -HH:MM:SS- ..."
    const String padLiberou = " liberou -";
    int idxLib = line.indexOf(padLiberou);
    if (idxLib < 0) continue;
    int uidUsuarioStart = idxLib + padLiberou.length();
    int uidUsuarioEnd = line.indexOf("-", uidUsuarioStart);
    if (uidUsuarioEnd < 0) continue;
    String uidUsuario = line.substring(uidUsuarioStart, uidUsuarioEnd);
    uidUsuario.trim();
    uidUsuario.toLowerCase();

    // Garante que é um usuário (e não funcionário)
    if (!isRegistered(CARDS_FILE, uidUsuario)) {
      continue;
    }

    // Procura o UID no array
    int idx = -1;
    for (int i = 0; i < numUidsDia; i++) {
      if (uidsDia[i] == uidUsuario) {
        idx = i;
        break;
      }
    }

    // Se ainda não existe, adiciona
    if (idx == -1) {
      if (numUidsDia < MAX_UIDS_DIA) {
        uidsDia[numUidsDia] = uidUsuario;
        contagens[numUidsDia] = 1; // primeira vez
        numUidsDia++;
      }
    } else {
      contagens[idx]++; // mais uma ocorrência
    }
  }

  f.close();

  // Agora verifica quem tem contagem ímpar
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

// --------- MQTT: callback e reconexão ---------

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT mensagem recebida em [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);
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

// --------- Movimentação: arquivo + MQTT ----------

void registrarMovimentacao(const String &uidFuncionario, const String &uidUsuario) {
  String dataStr, horaStr;
  if (!obterDataHoraAtual(dataStr, horaStr)) {
    dataStr = "data_indisponivel";
    horaStr = "hora_indisponivel";
  }

  String linha = "-" + uidFuncionario + "- liberou -" + uidUsuario +
                 "- às -" + horaStr + "- do dia -" + dataStr + "-";

  if (appendLine(MOVIMENTACOES_FILE, linha)) {
    Serial.println("Movimentacao registrado: " + linha);
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

// --------- envio de UID para a fila (String) ----------

void enviarUidParaFila(const String &uidString) {
  if (filaCartoes == NULL) return;

  String copia = uidString;

  if (xQueueSend(filaCartoes, &copia, pdMS_TO_TICKS(100)) != pdTRUE) {
    Serial.println("Aviso: filaCartoes cheia, UID descartado.");
  } else {
    Serial.print("UID enfileirado: ");
    Serial.println(copia);
  }
}

// --------- TODA a lógica de validação em UMA função ----------

void processarCartaoLido(const String &uidLido) {
  if (filaCartoes == NULL || semAcessoLiberado == NULL) {
    Serial.println("Erro: fila ou semaforo nao inicializados.");
    return;
  }

  enviarUidParaFila(uidLido);
  cartoesPendentes++;

  if (cartoesPendentes < 2) {
    Serial.println("Aguardando segundo cartao para validacao...");
    return;
  }

  cartoesPendentes = 0;

  String uid1, uid2;

  Serial.println("Obtendo primeiro cartao da fila...");
  if (xQueueReceive(filaCartoes, &uid1, pdMS_TO_TICKS(0)) != pdTRUE) {
    Serial.println("Falha ao obter o primeiro cartao da fila.");
    return;
  }

  Serial.println("Obtendo segundo cartao da fila...");
  if (xQueueReceive(filaCartoes, &uid2, pdMS_TO_TICKS(0)) != pdTRUE) {
    Serial.println("Falha ao obter o segundo cartao da fila.");
    return;
  }

  uid1.trim(); uid1.toLowerCase();
  uid2.trim(); uid2.toLowerCase();

  Serial.print("UID1 recebido: "); Serial.println(uid1);
  Serial.print("UID2 recebido: "); Serial.println(uid2);

  bool uid1EhUsuario      = isRegistered(CARDS_FILE,  uid1);
  bool uid1EhFuncionario  = isRegistered(ADMINS_FILE, uid1);

  bool uid2EhUsuario      = isRegistered(CARDS_FILE,  uid2);
  bool uid2EhFuncionario  = isRegistered(ADMINS_FILE, uid2);

  if ((!uid1EhUsuario && !uid1EhFuncionario) ||
      (!uid2EhUsuario && !uid2EhFuncionario)) {
    Serial.println("Falha: um ou ambos os UIDs nao estao cadastrados.");
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }

  if ((uid1EhUsuario && uid1EhFuncionario) ||
      (uid2EhUsuario && uid2EhFuncionario)) {
    Serial.println("Falha: UID encontrado em usuarios E funcionarios (configuracao invalida).");
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }

  bool casoValido =
      ( (uid1EhUsuario && uid2EhFuncionario) ||
        (uid1EhFuncionario && uid2EhUsuario) );

  if (!casoValido) {
    Serial.println("Falha: combinacao invalida (dois usuarios ou dois funcionarios).");
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }

  String uidFuncionario, uidUsuario;
  if (uid1EhFuncionario && uid2EhUsuario) {
    uidFuncionario = uid1;
    uidUsuario     = uid2;
  } else {
    uidFuncionario = uid2;
    uidUsuario     = uid1;
  }

  Serial.println("✅ Combinacao valida: 1 funcionario + 1 usuario.");

  registrarMovimentacao(uidFuncionario, uidUsuario);

  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  delay(2000);
  digitalWrite(LED_GREEN, LOW);

  xSemaphoreGive(semAcessoLiberado);
  if (xSemaphoreTake(semAcessoLiberado, 0) == pdTRUE) {
    Serial.println("Semaforo semAcessoLiberado sinalizado e consumido.");
  }
}

// ainda existe, mas não está sendo usada nesse fluxo de dupla
void checkCardRegistered(const String &uidString) {
  if (isRegistered(CARDS_FILE, uidString)) {
    Serial.println("✅ Cartao cadastrado em usuarios.txt! LED VERDE...");
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    delay(2000);
    digitalWrite(LED_GREEN, LOW);
  } else {
    Serial.println("❌ Cartao NÃO cadastrado em usuarios.txt! LED VERMELHO...");
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
    "'t' = usuarios atrasados (primeira passagem apos 08:15 hoje) \n"
    "'p' = usuarios que estao dentro (registros impares hoje) \n"
    "'d' = deletar UID \n"
    "'m' = listar movimentacoes"
  ));

  filaCartoes = xQueueCreate(8, sizeof(String));
  if (filaCartoes == NULL) {
    Serial.println("ERRO: nao foi possivel criar filaCartoes!");
  }

  semAcessoLiberado = xSemaphoreCreateBinary();
  if (semAcessoLiberado == NULL) {
    Serial.println("ERRO: nao foi possivel criar semAcessoLiberado!");
  }
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') registerCard(CARDS_FILE);
    if (c == 'a' || c == 'A') registerCard(ADMINS_FILE);
    if (c == 'l')             listRegistered(CARDS_FILE);
    if (c == 'L')             listRegistered(ADMINS_FILE);
    if (c == 'u' || c == 'U') countRegisteredAndShow(CARDS_FILE);
    if (c == 'f' || c == 'F') countRegisteredAndShow(ADMINS_FILE);
    if (c == 't' || c == 'T') listarAtrasosDepoisDe815();
    if (c == 'p' || c == 'P') listarUsuariosDentroHoje();
    if (c == 'm' || c == 'M') listMovimentacoes();
    if (c == 'd' || c == 'D') {
      Serial.println("Digite o UID a deletar:");
      while (!Serial.available()) delay(10);
      String uid = Serial.readStringUntil('\n');
      uid.trim();
      uid.toLowerCase();
      deleteCard(uid);
    }
  }

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial())   return;

  Serial.print("Card UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();

  String uidString = uidToString(mfrc522.uid);
  Serial.print("UID em texto: ");
  Serial.println(uidString);

  processarCartaoLido(uidString);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
