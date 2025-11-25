# Controle de Entrada e Saída com ESP32 + RFID
### Descrição

Sistema para controle de entrada e saída de crianças em creches/escolas com dupla confirmação: leitura do cartão do funcionário e, em seguida, do responsável. O evento só é registrado quando ambos os identificadores são validados dentro de uma janela de tempo. Os registros são armazenados no sistema de arquivos da ESP32.

### Especificações do Projeto

Placa: ESP32 (framework Arduino, via PlatformIO)

Módulo RFID: RC522 (RoboCore)

Linguagem: C/C++

Baudrate Serial: 9600

### Ligações (SPI – exemplo, ajuste conforme sua placa)

RC522 SDA(SS) → ESP32 GPIO 5

RC522 SCK → ESP32 GPIO 18

RC522 MOSI → ESP32 GPIO 23

RC522 MISO → ESP32 GPIO 19

RC522 RST → ESP32 GPIO 21

RC522 3.3V → ESP32 3V3

RC522 GND → ESP32 GND

### Estrutura Sugerida
src/
  main.cpp          (lógica principal, estados, janela de confirmação)
  rfid.*            (inicialização/leitura MFRC522)
  storage.*         (SPIFFS: ler/gravar UIDs e logs)
  ui.*              (feedback por Serial/LED/Buzzer, comandos)
data/
  cards.txt
  admins.txt
platformio.ini
README.md

<img width="804" height="182" alt="image" src="https://github.com/user-attachments/assets/4d60914b-cc47-4176-a678-4b7ac2629c8f" />


### Dependências (PlatformIO)

platformio.ini (exemplo):

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
  miguelbalboa/MFRC522 @ ^1.4.10

### Passos de Uso

Instalar PlatformIO (VS Code).

Configurar pinos conforme a seção “Ligações”.

(Opcional) Colocar cards.txt e admins.txt em data/ e fazer upload do FS (SPIFFS).

Compilar e fazer upload do firmware.

Abrir Serial Monitor (9600) para acompanhar leituras e registros.

### Observações Técnicas

Comparação de UIDs em minúsculas com trim() para evitar problemas de CRLF.

A remoção de UID procura primeiro em cards.txt; se não encontrar, procura em admins.txt. Só informa “não encontrado” se ausente em ambos.

Manter GND comum e alimentação 3V3 estável; cabos curtos no SPI.
