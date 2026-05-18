# Issue – Blackout SPIFFS & Strict Filesystem QA (Sprint 01 — V1)

## Descripción
Asegurar blackout total de SPIFFS en todos los features activos. Validación QA obligatoria, script QA/check-contract.sh debe bloquear cualquier referencia inaceptable.

## Tareas
- [ ] Buscar referencias a SPIFFS en código activo
- [ ] QA/check-contract.sh obligatorio en CI/local
- [ ] Alertar equipo si surge cualquier mención productiva

---

Relacionado a: QA/check-contract.sh, docs/BUILD_SYSTEM.md, docs/legacy_requirements.md