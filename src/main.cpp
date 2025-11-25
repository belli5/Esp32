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


// Variáveis das LEDs e Arquivos
#define LED_RED     27  
#define LED_GREEN    4  
#define LED_YELLOW  22  
const char* CARDS_FILE          = "/usuarios.txt";
const char* ADMINS_FILE         = "/funcionarios.txt";
const char* MOVIMENTACOES_FILE  = "/movimentacoes.txt";


// Configuralção de WiFi e de Fuso
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


// Fução que verifica se um certo UID está cadastrado
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
    Serial.println("Falha ao conectar no WiFi. Hora NTP pode nao funcionar.");
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

void registrarMovimentacao(const String &uidFuncionario, const String &uidUsuario) {
  String dataStr, horaStr;
  if (!obterDataHoraAtual(dataStr, horaStr)) {
    dataStr = "data_indisponivel";
    horaStr = "hora_indisponivel";
  }

  String linha = "-" + uidFuncionario + "- liberou -" + uidUsuario +
                 "- às -" + horaStr + "- do dia -" + dataStr + "-";

  if (appendLine(MOVIMENTACOES_FILE, linha)) {
    Serial.println("Movimentacao registrada: " + linha);
  } else {
    Serial.println("ERRO ao registrar movimentacao em MOVIMENTACOES_FILE.");
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

  // 1) Enfileira o UID lido
  enviarUidParaFila(uidLido);
  cartoesPendentes++;

  // Se ainda só temos 1 cartão, espera o próximo
  if (cartoesPendentes < 2) {
    Serial.println("Aguardando segundo cartao para validacao...");
    return;
  }

  // Já temos 2 cartões enfileirados: zera o contador
  cartoesPendentes = 0;

  // 2) Pega os dois UIDs da fila
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

  // 3) Verifica se cada UID é usuario/funcionario
  bool uid1EhUsuario      = isRegistered(CARDS_FILE,  uid1);
  bool uid1EhFuncionario  = isRegistered(ADMINS_FILE, uid1);

  bool uid2EhUsuario      = isRegistered(CARDS_FILE,  uid2);
  bool uid2EhFuncionario  = isRegistered(ADMINS_FILE, uid2);

  // 3.1) Algum não cadastrado?
  if ((!uid1EhUsuario && !uid1EhFuncionario) ||
      (!uid2EhUsuario && !uid2EhFuncionario)) {
    Serial.println("Falha: um ou ambos os UIDs nao estao cadastrados.");
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }

  // 3.2) Algum está ao mesmo tempo em usuarios e funcionarios? (config errada)
  if ((uid1EhUsuario && uid1EhFuncionario) ||
      (uid2EhUsuario && uid2EhFuncionario)) {
    Serial.println("Falha: UID encontrado em usuarios E funcionarios (configuracao invalida).");
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    delay(2000);
    digitalWrite(LED_RED, LOW);
    return;
  }

  // 3.3) Só é válido se for 1 usuario + 1 funcionario
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

  // 4) Define quem é funcionário e quem é usuário
  String uidFuncionario, uidUsuario;
  if (uid1EhFuncionario && uid2EhUsuario) {
    uidFuncionario = uid1;
    uidUsuario     = uid2;
  } else { // uid1EhUsuario && uid2EhFuncionario
    uidFuncionario = uid2;
    uidUsuario     = uid1;
  }

  Serial.println("✅ Combinacao valida: 1 funcionario + 1 usuario.");

  // 5) Registra no arquivo de movimentacoes com data/hora
  registrarMovimentacao(uidFuncionario, uidUsuario);

  // 6) Acende LED verde para indicar acesso liberado
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  delay(2000);
  digitalWrite(LED_GREEN, LOW);

  // 7) Usa o semáforo (apenas demonstrativo)
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

  // Wi-Fi + NTP
  initWiFi();
  initTime();

  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.println(F("Scan PICC to see UID"));
  Serial.println(F(
    "Comandos: "
    "'c' = cadastrar novo usuario | "
    "'a' = cadastrar novo admin | "
    "'l' = listar usuarios | "
    "'L' = listar admins | "
    "'d' = deletar UID | "
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
  // Comandos via Serial
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') registerCard(CARDS_FILE);
    if (c == 'a' || c == 'A') registerCard(ADMINS_FILE);
    if (c == 'l')             listRegistered(CARDS_FILE);
    if (c == 'L')             listRegistered(ADMINS_FILE);
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

  // Leitura de cartão normal
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
