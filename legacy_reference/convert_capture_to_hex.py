import argparse
import asyncio
import time
from datetime import datetime
from pathlib import Path
from typing import Iterable, Optional

try:
    import serial
    import serial_asyncio
except ImportError as exc:  # pragma: no cover - guard for environments sin pyserial
    raise SystemExit("pyserial es requerido para leer desde el puerto serie") from exc


def detect_encoding(data: bytes, candidates: Optional[Iterable[str]] = None) -> str:
    """
    Try to guess the encoding by attempting strict decode/encode round-trips.
    Returns the first encoding that round-trips perfectly; falls back to
    Windows-1252 if none match.
    """
    if candidates is None:
        candidates = ("utf-8-sig", "utf-8", "windows-1252", "latin-1", "cp850", "cp437")

    for enc in candidates:
        try:
            text = data.decode(enc)
            if text.encode(enc) == data:
                return enc
        except UnicodeDecodeError:
            continue
    return "windows-1252"


def render_hex_lines(raw: bytes) -> tuple[str, list[str]]:
    """
    Turn each logical line into either spaced hex (if it contains STX/ETX) or a
    readable ASCII string. NUL bytes in ASCII mode are rendered as "00". Literal
    "<LF>" tokens are treated as real line breaks, but newline bytes inside a
    command (between STX/ETX) are preserved as data, not as line separators.
    """

    def split_capture_bytes(data: bytes) -> list[bytes]:
        data = data.replace(b"<LF>", b"\n")
        lines: list[bytes] = []
        buf = bytearray()
        inside_frame = False
        for b in data:
            if b == 0x02:
                inside_frame = True
            if not inside_frame and b in (0x0A, 0x0D):
                if buf:
                    lines.append(bytes(buf))
                    buf.clear()
                continue
            buf.append(b)
            if inside_frame and b == 0x03:
                inside_frame = False
        if buf:
            lines.append(bytes(buf))
        return lines

    encoding = detect_encoding(raw)
    lines: list[str] = []
    for line_bytes in split_capture_bytes(raw):
        contains_command = any(b in (0x02, 0x03) for b in line_bytes)
        if contains_command:
            rendered = " ".join(f"{byte:02X}" for byte in line_bytes)
        else:
            parts = []
            for b in line_bytes:
                if b == 0x00:
                    parts.append("00")
                else:
                    char = bytes([b]).decode(encoding, errors="ignore")
                    if char and char.isprintable():
                        parts.append(char)
                    elif 32 <= b <= 126:
                        parts.append(chr(b))
                    else:
                        parts.append(f"{b:02X}")
            rendered = "".join(parts)
        lines.append(rendered)
    return encoding, lines


async def write_with_prefix(
    prefix: str,
    hex_lines: list[str],
    outfile,
    lock: asyncio.Lock,
) -> None:
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:23]
    async with lock:
        for hex_line in hex_lines:
            outfile.write(f"{prefix} {timestamp} - {hex_line}\n")
        outfile.flush()


def read_serial_capture(
    port: str,
    baudrate: int = 115200,
    timeout: float = 0.1,
    idle_seconds: float = 1.0,
) -> bytes:
    """
    Listen on the serial port and accumulate data until an idle period elapses.
    Idle is defined as `idle_seconds` with no incoming bytes. Press Ctrl+C to stop.
    """
    buffer = bytearray()
    with serial.Serial(
        port=port,
        baudrate=baudrate,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=timeout,
    ) as ser:
        last_data = time.time()
        try:
            while True:
                chunk = ser.read(4096)
                if chunk:
                    buffer.extend(chunk)
                    last_data = time.time()
                elif buffer and (time.time() - last_data) >= idle_seconds:
                    break
        except KeyboardInterrupt:
            pass
    return bytes(buffer)


async def read_serial_capture_async(
    port: str,
    baudrate: int,
    idle_seconds: float,
    on_message,
    source_prefix: str,
) -> None:
    """
    Async variant: listens and flushes buffer after idle_seconds without data.
    """
    reader, _ = await serial_asyncio.open_serial_connection(
        url=port,
        baudrate=baudrate,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
    )
    buffer = bytearray()
    last_data = time.time()
    try:
        while True:
            try:
                chunk = await asyncio.wait_for(reader.read(4096), timeout=idle_seconds)
            except asyncio.TimeoutError:
                chunk = b""

            if chunk:
                buffer.extend(chunk)
                last_data = time.time()
                continue

            if buffer and (time.time() - last_data) >= idle_seconds:
                await on_message(bytes(buffer), source_prefix)
                buffer.clear()
    except asyncio.CancelledError:
        if buffer:
            await on_message(bytes(buffer), source_prefix)
        raise


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Sniffer serie dual (SGC_MOBILE y RI505) que convierte los datos a "
            "representacion hexadecimal ASCII separada por espacios. Los literales "
            "<LF> se reemplazan por saltos de linea."
        )
    )
    parser.add_argument(
        "--from-file",
        metavar="PATH",
        help="Leer datos desde un archivo en lugar de puerto serie (fallback/manual).",
    )
    parser.add_argument(
        "--port-sgc",
        default="COM3",
        help="Puerto serie a supervisar para SGC_MOBILE (por defecto COM3).",
    )
    parser.add_argument(
        "--port-ri",
        default="COM20",
        help="Puerto serie a supervisar para RI505 (por defecto COM20).",
    )
    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Baud rate del puerto (por defecto 115200).",
    )
    parser.add_argument(
        "--idle-ms",
        type=int,
        default=1000,
        help="Tiempo en milisegundos sin datos para cerrar la captura (por defecto 1000ms).",
    )
    parser.add_argument(
        "output_file",
        nargs="?",
        default="ri505_capture_hex.txt",
        help="Ruta del archivo de salida a generar.",
    )
    args = parser.parse_args()

    output_path = Path(args.output_file)
    if args.from_file:
        raw = Path(args.from_file).read_bytes()
        encoding, hex_lines = render_hex_lines(raw)
        with output_path.open("w", encoding="utf-8", newline="\n") as outfile:
            # Prefijo estatico para modo archivo
            asyncio.run(write_with_prefix("FILE", hex_lines, outfile, asyncio.Lock()))
        print(f"Archivo generado: {output_path}")
        print(f"Codificacion detectada: {encoding}")
        return

    async def run_dual_capture() -> None:
        lock = asyncio.Lock()
        with output_path.open("w", encoding="utf-8", newline="\n") as outfile:

            async def on_message(raw: bytes, source_prefix: str) -> None:
                encoding, hex_lines = render_hex_lines(raw)
                await write_with_prefix(source_prefix, hex_lines, outfile, lock)
                print(f"[{source_prefix}] Capturados {len(hex_lines)} lineas (enc: {encoding})")

            idle_seconds = args.idle_ms / 1000.0
            tasks = [
                asyncio.create_task(
                    read_serial_capture_async(
                        port=args.port_sgc,
                        baudrate=args.baudrate,
                        idle_seconds=idle_seconds,
                        on_message=on_message,
                        source_prefix="SGC_MOBILE-",
                    )
                ),
                asyncio.create_task(
                    read_serial_capture_async(
                        port=args.port_ri,
                        baudrate=args.baudrate,
                        idle_seconds=idle_seconds,
                        on_message=on_message,
                        source_prefix="RI505-",
                    )
                ),
            ]
            try:
                await asyncio.gather(*tasks)
            except KeyboardInterrupt:
                for t in tasks:
                    t.cancel()
                await asyncio.gather(*tasks, return_exceptions=True)

    print(
        f"Escuchando {args.port_sgc} (SGC_MOBILE) y {args.port_ri} (RI505) "
        f"a {args.baudrate} bps (8N1). Inactividad cierre: {args.idle_ms} ms."
    )
    asyncio.run(run_dual_capture())


if __name__ == "__main__":
    main()
