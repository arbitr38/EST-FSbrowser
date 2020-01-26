- веб-браузер для файловой системы SPIFFS (на esp32 wrover TTGO-T18V2.2 https://github.com/LilyGO/TTGO-T-Lion-T18V2.2- )
- библиотеки Arduino и:

+ пример работы с OLED SSD1306 дисплеем
+ пример работы с кнопками на устройстве


Кроме скетчей, для загрузки файлов с данными (jpg, pdf, txt и тд) на устройство нужно расширение к Arduino IDE: 

Arduino ESP32/ESP8266 filesystem uploader
https://github.com/me-no-dev/arduino-esp32fs-plugin/releases/download/1.0/ESP32FS-1.0.zip
https://github.com/esp8266/arduino-esp8266fs-plugin/releases/download/0.5.0/ESP8266FS-0.5.0.zip


распаковать в <home_dir>/Arduino/tools/ESP32FS/tool/esp32fs.jar или esp8266fs.jar).
Перезагрузить Arduino IDE. 

+ Установить поддержку esp32/esp8266 через менеджер плат в Arduino IDE
+ Для прошивки выбрать плату ESP32 Wrover или esp 8266 generic

.
