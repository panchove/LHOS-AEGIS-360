# CI Gating Rules (Políticas de Gateo en CI)

Objetivo
- Definir reglas mínimas que un Pull Request debe cumplir para integrarse en ramas protegidas, garantizando conformidad con ISO/IEC/IEEE 12207 y las políticas de estabilidad del proyecto.

Requisitos mínimos para merge
1. Build: `idf.py build` o equivalente debe completarse con éxito para target objetivo (esp32s3). No se aceptan errores de compilación.
2. Tests: Todos los tests unitarios y de componente definidos deben pasar (`make test` / `idf.py test`) o, si no aplican en el PR, documentar por qué.
3. Contract Check: ejecutar script de verificación estática (tools/contract_check.sh) que detecte uso prohibido de heap en los componentes core y uso de APIs prohibidas.
4. Size/Impact: ejecutar `make size` o script equivalente para detectar cambios de flash/ram por encima del umbral definido (p.ej. +5%); si excede, requerir aprobación de Arquitecto.
5. Docs: cualquier cambio funcional que modifique ownership, APIs públicos o contratos debe actualizar docs/handoff/ y pasar revisión arquitectónica.
6. Lint/Format: pasar `clang-format`/`cppcheck` o las herramientas definidas por el proyecto.

Gating workflow técnico
- PR debe incluir checklist en la descripción con las casillas: build, tests, contract-check, size, docs, reviewers.
- CI debe ejecutar gates en este orden: build → contract-check → tests → size → docs validation.
- En caso de failure en contract-check: bloquear merge y etiquetar con `contract-violation`.

Excepciones
- Hotfixes críticos pueden saltarse parte del pipeline bajo aprobación explícita del Release Manager y notificación en ACTIVE_RISKS.md.

Herramientas recomendadas
- tools/contract_check.sh: búsqueda de patterns (malloc|new|std::vector|std::map) en componentes core.
- scripts/size_check.py: calcular delta de size entre commits y comparar contra umbral.
