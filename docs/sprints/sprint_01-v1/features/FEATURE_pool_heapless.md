# Feature – Pool heapless (Sprint 01 — V1)

## 1. Descripción breve
Implementa un administrador heapless de buffers fijos, compatible con policy LittleFS-only, C17 puro, con soporte para métricas y validación QA.

## 2. Justificación y contexto contractual
- Basado en API y contratos definidos en ISO12207_GUIDELINES.md y APIS_CORE.md
- Es parte del sprint 01 "infraestructura core"
- Cubierto por restricciones ISO/QA: Sí (heapless core, métricas, sólo LittleFS)

## 3. Requisitos funcionales
- [ ] Alocar y liberar buffers en forma determinista
- [ ] Contrato de ownership estricto
- [ ] Stats de uso y high-water mark

## 4. No funcionales
- [ ] Sin heap ni std::vector/string, puro C17
- [ ] Instrumentable desde tests QA

## 5. Arquitectura/diseño
- API C explícita, integración con workflows de logging/blackbox

## 6. Checklist entregables
- [ ] pool.h/.c, tests, doc integrada
- [ ] QA/check-contract
- [ ] Demo de stress y logs

## 7. Acceptance criteria / QA
- [ ] Stress: sin memory leaks, uso conforme ownership contract

---

ID contrato: APIS_CORE.md sección 1, ISO12207_GUIDELINES.md memory policy
