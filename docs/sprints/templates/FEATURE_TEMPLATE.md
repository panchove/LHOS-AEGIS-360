# Feature – [Nombre de la feature]

## 1. Descripción breve

Explica en una frase qué resuelve/mejora y para qué usuario o sistema es relevante.

## 2. Justificación y contexto contractual
- ¿Qué documento o contrato justifica esta feature?
- ¿Forma parte de sprint/tag específico?
- ¿Cubierto por restricciones ISO/QA? (sí/no, explicar)

## 3. Requisitos funcionales
- [ ] Cada requisito debe ser medible, auditable y tener un campo de validación QA.

## 4. Requisitos no funcionales/arquitectónicos
- [ ] Seguridad, performance, persistencia/logs (solo LittleFS permitido, SPIFFS prohibido en QA y producción), internacionalización, etc.

## 5. Arquitectura / diseño
- [ ] Resumen de flujo de datos, dependencias, restricciones técnicas.

## 6. Checklist de entregables
- [ ] Código/artefactos
- [ ] Documentación actualizada
- [ ] Tests
- [ ] Endpoints/API/documentación Swagger
- [ ] QA validado
- [ ] Build (idf.py build) probado en hardware/runner
- [ ] Flash (idf.py -p <puerto> flash) probado y documentado
- [ ] Monitor (idf.py -p <puerto> monitor) ejecutado, logs visibles

## 7. Acceptance Criteria y QA
- [ ] Criterios para que PO/QA acepte la feature como DONE (incluye evidence práctica: build/flash/monitor OK en ESP-IDF, logs ser auditables, screenshots/log artefacts preferidos)


---

ID de contrato/requisito(s) relacionados: [enlazar docs, sección y/o issue]
