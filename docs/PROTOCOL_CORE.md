# ---
owner: ODIN
last_reviewed: 2026-05-18
status: draft
doc_id: PROTOCOL_CORE
# ---

# LHOS-AEGIS-360 — Layrz Protocol Core (Sprint 00)

---

Nota contractual:
El FSM parser opera sobre streams/fragments sólo dentro de session window interna; su output y todo interaction con EventBus, Retry y métricas se da por packet completo únicamente. Véase docs/PACKET_CENTRIC_MODEL.md

---

## Architecture

```
[Lexer]→[FSM Parser]→[CRC Validator]→[Registry]→[Dispatcher]
         │              │               │         │
         └──────────────┴───────────────┴─────────┘
                        [Streaming/Partial Buffer]
```

## Packet Groups
| Group | Prefix | Direction         | Notes                  |
|-------|--------|-------------------|------------------------|
| A*    | <A*>   | Server→Device     | e.g. Ab, Ac, Ao, ...   |
| P*    | <P*>   | Device→Server     | e.g. Pa, Pb, ...       |
| T*    | <T*>   | Trips/service     | e.g. Ts, Te            |
| I*    | <I*>   | AI                | e.g. Im                |
| Rsv   | -      | Reserved/Future   | Protocol upgrades      |

## Global Invariants
| Property            | Value                                                                   |
|---------------------|-------------------------------------------------------------------------|
| Framing             | <XX> ... </XX>                                                          |
| Delimiters          | ; (main), : , , as per packet spec                                      |
| CRC                 | CRC16 mandatory                                                         |
| Escaping            | Not used (for v2.6); reserved                                           |
| Field Limits        | Per packet spec, hard checked                                            |
| Packet Max Size     | 512 bytes typical (board-specific)                                       |
| Streaming           | FSM handles partial/fragmented packets across calls                      |
| Multi-packet        | Buffering supported, parsed in order                                     |
| Timeout             | Hard timeout per packet/field (500ms default)                            |

## FSM Parser States
| State      | Rule/Entry                          | Exit To        |
|------------|-------------------------------------|----------------|
| IDLE       | Await start delimiter               | SYNC           |
| SYNC       | < found, read header                | HEADER/BODY    |
| HEADER     | Type/len check                      | BODY           |
| BODY       | Read up to ; or packet end          | CRC            |
| CRC        | Parse+verify, commit fail/ok        | DISPATCH/ERROR |
| DISPATCH   | Registry/Dispatcher callback        | IDLE           |
| ERROR      | Log+drop+recover                    | RECOVERY/IDLE  |
| RECOVERY   | Flush buffer/advance to IDLE        | IDLE           |

## Ownership / Memory
- All streaming decode happens into pre-assigned PSRAM buffer
- Zero-copy to registry/handlers if static size
- Partial packets only promote to registry after CRC-ok

## Failure Paths
- Incomplete: Wait more (FSM holds state)
- Bad CRC: Drop, log, buffer advance
- Overrun: Log+reset FSM
- Underrun: Wait, or reset after timeout

## Protocol Capability Model
- Contract per device/variant (ALL handled by registry)
- Version-future safe; reserved fields not matched in-light implementations
