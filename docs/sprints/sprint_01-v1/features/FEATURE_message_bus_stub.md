# Feature – Message Bus STUB (Sprint 01 — V1)

## 1. Descripción breve
Stub inicial del message bus con ownership contract, publish/subscribe dummy y validación QA (sin implementación de backlog/blackbox todavía).

## 2. Justificación y contexto contractual
- API y restricciones MessageBus en APIS_CORE.md sección 4
- Parte de core infra (Sprint 01)
- QA: Ownership transfer, dummy, integración con parser/pool

## 3. Requisitos funcionales
- [ ] publish(pool_buf_t*) ownership
- [ ] subscribe/callback
- [ ] test API contract

## 4. Checklist
- [ ] message_bus.h/.c, tests

## 5. Acceptance/QA
- [ ] Ownership policy audible y contrato visible

---

ID contrato: APIS_CORE.md sección 4