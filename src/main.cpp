#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
//#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>


#include <FS.h>
#include <SPIFFS.h>


#define LED_RED    27  
#define LED_GREEN   4  
#define LED_YELLOW   22  


const char* CARDS_FILE  = "/usuarios.txt";
const char* ADMINS_FILE = "/funcionarios.txt";


// SPI / RC522
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};


// --------- helpers ----------
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


// retorna true se uid (string minúscula, sem espaços) estiver no arquivo
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




// --------- remove um UID do arquivo /usuarios.txt ----------
static bool tryRemoveUidFrom(const char* path, const String& uidNorm) {
  File f = SPIFFS.open(path, FILE_READ);
  if (!f) {
    Serial.printf("Aviso: arquivo %s nao encontrado.\n", path);
    return false; // não impediu a remoção, apenas não há onde remover
  }

  String newContent = "";
  bool found = false;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();                   // remove \r e espaços
    if (!line.length()) continue;  // ignora linhas em branco

    String cmp = line;
    cmp.toLowerCase();             // normaliza linha para comparar
    if (cmp == uidNorm) {
      found = true;                // achou -> NÃO copia para o novo conteúdo
    } else {
      newContent += line + "\n";   // mantém as outras linhas exatamente como estavam
    }
  }
  f.close();

  if (!found) return false;

  // sobrescreve o arquivo sem o UID removido
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

bool deleteCard(const String &uidToRemoveRaw) {
  // normaliza o UID informado
  String uid = uidToRemoveRaw;
  uid.trim();
  uid.toLowerCase();

  // 1) tenta remover dos cartões
  if (tryRemoveUidFrom(CARDS_FILE, uid)) {
    return true;
  }

  // 2) se não achou, tenta remover dos admins
  if (tryRemoveUidFrom(ADMINS_FILE, uid)) {
    return true;
  }

  // 3) só aqui avisa que não encontrou em nenhum dos dois
  Serial.println("UID nao encontrado em CARDS_FILE nem em ADMINS_FILE.");
  return false;
}



// --------- cadastro: espera um cartao e salva o UID em /usuarios.txt ----------
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




// --------- verificação de acesso: agora usa o arquivo /usuarios.txt ----------
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


  // LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);


  // SPIFFS
  if (!SPIFFS.begin(true)) { // true: formata se precisar
    Serial.println("ERRO: SPIFFS nao inicializado.");
  } else {
    Serial.println("SPIFFS OK. Arquivo de cadastros: /usuarios.txt");
  }


  // RFID
  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  Serial.println(F("Scan PICC to see UID"));
  Serial.println(F("Comandos: 'c' = cadastrar novo UID | 'l' = listar UIDs | 'd' = deletar UID | 'a' = cadastrar novo admin | 'L' = listar admins "));
}


void loop() {
  // comandos por serial
  if (Serial.available()) {
  char c = Serial.read();
  if (c == 'c' || c == 'C') registerCard(CARDS_FILE);   // cadastra em /usuarios.txt
  if (c == 'a' || c == 'A') registerCard(ADMINS_FILE);  // cadastra em /funcionarios.txt
  if (c == 'l')           listRegistered(CARDS_FILE);
  if (c == 'L')           listRegistered(ADMINS_FILE);
  if (c == 'd' || c == 'D') {
    Serial.println("Digite o UID a deletar:");
    while (!Serial.available()) delay(10);
    String uid = Serial.readStringUntil('\n');
    uid.trim();
    uid.toLowerCase();
    deleteCard(uid);
  }
}


  // leitura "normal" (continua funcionando)
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial())   return;


  Serial.print("Card UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();


  String uidString = uidToString(mfrc522.uid);
  Serial.print("UID em texto: ");
  Serial.println(uidString);


  // agora valida contra a lista gravada
  checkCardRegistered(uidString);


  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
