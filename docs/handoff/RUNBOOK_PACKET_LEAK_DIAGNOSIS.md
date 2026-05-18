# RUNBOOK: Packet Leak Diagnosis (Diagnóstico de Fugas de Paquetes)

Propósito
- Procedimiento operativo para detectar, reproducir y corregir fugas de buffers/paquetes en runtime.

1. Señales de alerta
- pool_free_count decremental sostenido
- incrementos en queue_full_counters
- crashes relacionados con memoria o corrupción

2. Recolección de evidencia
- Habilitar trace circular de alloc/release (últimas N operaciones)
- Capturar snapshot de métricas: pool_free_count, uptime, cpu usage, queues
- Recolectar logs y stack traces (si aplica)

3. Diagnóstico rápido
- Verificar last N ops: identificar stage que no liberó (owner_id)
- Reproducir con load script (scripts/load_uart.sh o script de network)
- Si double_free sospechada: habilitar heap sanitizer o macros debug

4. Remediación
- Si stale ownership: corregir handoff path (asegurar release en error y success paths)
- Si double release: introducir guardas (pointer=NULL tras release) y asserts en pool_release
- Si exhaustion por bursts: implementar temporary drop/circuit-breaker + alarm

5. Validación post-fix
- Soak test 4-24 horas con métricas estables
- Revisión de trace circular para confirmar paridad alloc/release
