# HTTP Monitor API – LHOS-AEGIS-360

> Especificación OpenAPI 3.0, versionada en `/api/v1/...`. TODA persistencia es GZIP propio. 
> **La autenticación es OBLIGATORIA:**
> 1. POST `/api/v1/login` retorna un token al recibir las credenciales;
> 2. Toda request a recursos protegidos debe llevar `Authorization: Bearer <token>` en el header;
> 3. El token es válido para toda la sesión.
> 4. HTTPS es opcional, recomendado si hay memoria disponible.

## Endpoints Clave (bajo `/api/v1`)

### POST /api/v1/login
- Recibe `{ "username": "admin", "password": "secret" }`, devuelve `{ "token": "abcdef12345" }`

### GET /api/v1/status
- Estado del sistema; requiere Bearer token
- **Incluye campo `lzv`: versión activa del protocolo Layrz (1, 2, 2.5)**

### POST /api/v1/cfg/lzv
- Cambia la versión activa de protocolo Layrz (requiere Bearer token)
- Recibe `{ "lzv": <1|2|2.5> }`.
- El cambio es persistente y solo toma efecto tras reboot (ver flujo).

### POST /api/v1/recovery
- Dispara safe mode, quarantine o reboot con acción JSON (protegido)

---

- Todos los endpoints de descarga, log o histórico sirven únicamente datos comprimidos bajo formato GZIP exclusivo del proyecto.
- HTTPS es opcional, activable sólo si la memoria del dispositivo lo permite.
- La definición OpenAPI completa se encuentra en `HTTP_MONITOR_OPENAPI.yaml`.

**Notas:**
- El nombre de campo para versión Layrz: `lzv`, máximo 3 caracteres, mnemónico, siempre documentado en OpenAPI.
- El flujo correcto para cambiar versión Layrz es:
    1. `POST /cfg/lzv` con el valor deseado;
    2. Verificar confirmación;
    3. Hacer `POST /recovery` con `{ "action": "reboot" }` para aplicar el cambio;
    4. Confirmar que `GET /status` muestra el valor actualizado tras reboot.
- La UI/UX debe avisar SIEMPRE que el cambio requiere reinicio para aplicar.
