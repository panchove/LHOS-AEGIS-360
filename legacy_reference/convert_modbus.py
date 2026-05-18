#!/usr/bin/env python3
"""
Convert “modbus_multi_level … .xlsx” files into the multi-level JSON
format (devices ➊ → holding_registers ➋ with optional states ➌).

Usage
-----
    python convert_modbus.py input.xlsx output.json
"""

import sys
import json
import math
from pathlib import Path
import pandas as pd


def safe_num(value):
    """Return int / float if numeric, else raw (or None)."""
    if pd.isna(value):
        return None
    try:
        # Use int if the float is whole, else float
        fval = float(value)
        return int(fval) if fval.is_integer() else fval
    except Exception:  # not numeric
        return value


def convert_workbook(df: pd.DataFrame) -> list[dict]:
    """Parse the dataframe into a list of device-dicts."""
    devices: list[dict] = []
    current_device: dict | None = None
    i, n = 0, len(df)
    print(f"Converting {n} rows…")

    while i < n:
        row = df.iloc[i]

        # ── ➊ New-device header row ──────────────────────────────
        if not pd.isna(row.get("device_id")):
            if current_device:
                devices.append(current_device)

            current_device = {
                "device_id": int(row["device_id"]),
                "manufacturer": row.get("manufacturer", ""),
                "model": row.get("model", ""),
                "description": row.get("description", ""),
                "prefix": row.get("prefix", ""),
                "holding_registers": [],
            }

        # ── ➋ Register definition row ────────────────────────────
        if current_device and pd.notna(row.get("holding_registers")):
            reg = {
                "label": str(row["holding_registers"]).strip(),
                "offset": safe_num(row["Unnamed: 6"]),
                "bit_length": safe_num(row["Unnamed: 7"]),
                "is_little_endian": bool(row["Unnamed: 8"])
                if not pd.isna(row["Unnamed: 8"])
                else False,
                "is_signed": bool(row["Unnamed: 9"])
                if not pd.isna(row["Unnamed: 9"])
                else False,
                "multiplier": safe_num(row["Unnamed: 10"]),
                "offset_correction": safe_num(row["Unnamed: 11"]),
                "data_type": row.get("Unnamed: 12")
                if not pd.isna(row.get("Unnamed: 12"))
                else None,
                "min": safe_num(row["Unnamed: 13"]),
                "max": safe_num(row["Unnamed: 14"]),
                "units": row.get("Unnamed: 15")
                if not pd.isna(row.get("Unnamed: 15"))
                else "",
            }

            # ── ➌ Collect any following “state” rows ─────────────
            states = []
            j = i
            if not pd.isna(df.iloc[j]["Unnamed: 16"]):
                # If the next row has a state label, we start collecting states
                state_label = str(df.iloc[j]["Unnamed: 16"]).strip()
                state_val = safe_num(df.iloc[j]["Unnamed: 17"])
                states.append({"label": state_label, "value": state_val})
                j += 1
            while (
                j < n
                and pd.isna(df.iloc[j]["holding_registers"])
                and not pd.isna(df.iloc[j]["Unnamed: 16"])
            ):
                state_label = str(df.iloc[j]["Unnamed: 16"]).strip()
                state_val = safe_num(df.iloc[j]["Unnamed: 17"])
                states.append({"label": state_label, "value": state_val})
                j += 1

            if states:
                reg["states"] = states

            current_device["holding_registers"].append(reg)

        i += 1

    if current_device:
        devices.append(current_device)

    return devices


def main(in_path: str, out_path: str):
    df = pd.read_excel(in_path)
    devices = convert_workbook(df)

    with open(out_path, "w", encoding="utf-8") as f:
        json.dump({"devices": devices}, f, indent=2)

    print(f"✅  Wrote {len(devices)} device(s) → {out_path}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python convert_modbus.py input.xlsx output.json")
        sys.exit(1)

    main(sys.argv[1], sys.argv[2])
