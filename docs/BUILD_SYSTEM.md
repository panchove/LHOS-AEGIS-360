# BUILD SYSTEM y Workflows ESP-IDF

## 1. Comandos principales (ESP-IDF)

- **Build:**
  ```sh
  idf.py build
  ```
- **Flash (sube el binario; reemplaza el puerto por el correcto):**
  ```sh
  idf.py -p /dev/ttyUSB0 flash
  # El puerto de referencia oficial es /dev/ttyUSB0. Si difiere en tu entorno valida con `ls /dev/ttyUSB*`
  ```
- **Monitor (ver logs serial):**
  ```sh
  idf.py -p /dev/ttyUSB0 monitor
  ```
- **Build + Flash + Monitor, todo seguido:**
  ```sh
  idf.py -p /dev/ttyUSB0 flash monitor
  ```
- **Clean/erase:**
  ```sh
  idf.py fullclean
  idf.py erase-flash
  ```

## 2. Estructura mínima de componentes
- Cada módulo funcional debe tener su propio CMakeLists.txt (ver ejemplos en src/ y tests/)
- Los tests deben vivir en tests/ y ser registrados con `idf_component_register`

## 3. Tests
- Usar Unity framework (ESP-IDF standard)
- Comando típico:
  ```sh
  idf.py -T <test_name> test
  ```
- Puede ejecutarse en CI usando runners oficiales Espressif

## 4. Consideraciones
- Nunca incluir ni depender de Arduino, PlatformIO u otros stacks en código productivo.
- Toda persistencia y logging debe ser sobre LittleFS (ver políticas en ISO12207_GUIDELINES.md, README.md)

## 5. Troubleshooting
- Si tienes problemas de permisos en el puerto USB:
  ```sh
  sudo usermod -a -G dialout $(whoami) && sudo udevadm control --reload-rules
  ```

---

Para detalles avanzados, ver docs oficiales Espressif: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/