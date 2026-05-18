# LHOS-AEGIS-360

### Sistema de Supervisión, Control y Telemetría modular sobre ESP32/ESP-IDF (Stack Layrz Edition)

## TL;DR
- **Cero herencia de código legacy**: la arquitectura, API y todas las capas de software están definidas desde cero, alineadas con contratos técnicos y procesos ISO/IEC 12207.
- **Documentación exhaustiva**: cada restricción, variante hardware y comportamiento del sistema es auditable y versionado.
- **Política de licencias**: el código legacy incluía MIT, Apache 2.0, BSD, GPLv3/LGPL y otros OSS. Esta nueva generación es 100% independiente de ese código (ver HANDOF.md y ISO12207_GUIDELINES.md para detalles legales y de proceso).

## Documentación clave
- [docs/sprints/ISO12207_GUIDELINES.md](docs/sprints/ISO12207_GUIDELINES.md)
- [docs/sprints/APIS_CORE.md](docs/sprints/APIS_CORE.md)
- [docs/sprints/SPRINT_MATRIX.md](docs/sprints/SPRINT_MATRIX.md)
- [docs/sprints/HTTP_MONITOR_OPENAPI.yaml](docs/sprints/HTTP_MONITOR_OPENAPI.yaml)
- [docs/sprints/sprint_06/features/FEATURE_monitor_uiux.md](docs/sprints/sprint_06/features/FEATURE_monitor_uiux.md)
- [legacy_reference/](legacy_reference/) (SOLO PARA REFERENCIA DE CONTRATOS, JAMÁS CÓDIGO)

## Directrices rápidas para contribución y uso
- Lee primero [docs/sprints/ISO12207_GUIDELINES.md](docs/sprints/ISO12207_GUIDELINES.md) y HANDOF.md para política de independencia y control de licencias.
- NO REUTILIZAR código de `legacy_reference`, sólo extraer contratos/diagramas requeridos.
- Toda contribución (feature, fix, refactor) debe estar trazada en documentación contractual y cumplir requisitos ISO/QA.

---

**Para cualquier uso o derivación:** verifica licencias, dependencias y sigue las normativas legales documentadas.