import json
import re

# File paths
input_file_path = "CSS-Electronics-11-bit-OBD2-v2.2.json"  # Input file
output_path = "obd2PID.json"

def convert_label(input_str):
    if not input_str:
        return None
    try:
        # Updated regex to match hexadecimal PIDs (00 to FF)
        match = re.match(r"(S\d{2}PID([0-9A-Fa-f]{2}))_(.*)", input_str)
        if not match:
            raise ValueError(f"Invalid format: {input_str}")
        
        pid_part, pid_hex, rest = match.groups()

        # Convert the PID from hex to decimal
        pid_decimal = int(pid_hex, 16)

        # Build the initial prefix with decimal PID
        prefix = f"s01.pid.{pid_decimal}"

        # Function to add dots between:
        # 1. Lowercase to Uppercase
        # 2. Letter to number
        # 3. Number to letter
        def add_dots(s):
            # Lowercase to Uppercase (camel case)
            s = re.sub(r'(?<=[a-z])(?=[A-Z])', '.', s)

            # Letter to number (e.g., bank2 -> bank.2)
            s = re.sub(r'(?<=[a-zA-Z])(?=[0-9])', '.', s)

            # Number to letter (e.g., 2Bank -> 2.bank)
            s = re.sub(r'(?<=[0-9])(?=[a-zA-Z])', '.', s)

            return s

        # Process and clean up the rest of the label
        rest_part = add_dots(rest).replace("_", ".").lower()

        # Final assembled result
        return f"obd2.{prefix}.{rest_part}"

    except ValueError as e:
        print(f"Error converting label: {e}")
        return None

def getPID(name) -> int:
    if name:
        # If the name contains "S02", return None
        if "S02" in name:
            return None
        # split the name by underscore and return the last two characters of the first part
        hexPID = name.split("_")[0][-2:]
        try:
            return int(hexPID, 16)
        except ValueError:
            return None
    return None

# Load the original JSON
with open(input_file_path, "r", encoding="utf-8") as f:
    original_data = json.load(f)

# Output structure
included_pids = {"pids": []}

# Process the input data and group signals by multiplexerValue (PID)
pid_map = {}

for param in original_data.get("params", []):
    for signal in param.get("signals", []):
        pid = getPID(signal.get("name"))
        if pid is None:
            continue

        # If this PID is new, initialize it
        if pid not in pid_map:
            print(f"Processing PID {pid}...")
            pid_map[pid] = {
                "p": pid,
                "s": []
            }

        # Create the base signal entry with abbreviated tags
        signal_entry = {
            "l": convert_label(signal.get("name")),
            "sb": signal.get("startBit") - 31,
            "bl": signal.get("bitLength"),
            "il": signal.get("isLittleEndian"),
            "is": signal.get("isSigned"),
            "f": signal.get("factor"),
            "o": signal.get("offset"),
            "mn": signal.get("min"),
            "mx": signal.get("max"),
            "u": signal.get("sourceUnit"),
        }

        # If `states` exists, convert and add it with abbreviated keys
        if "states" in signal and signal["states"]:
            signal_entry["st"] = [{"v": state["value"], "st": state["state"]} for state in signal["states"]]

        # Add the signal entry to the appropriate PID's signals list
        pid_map[pid]["s"].append(signal_entry)

# Convert the pid_map into the final list
included_pids["pids"] = list(pid_map.values())

# Save the new JSON to file
with open(output_path, "w", encoding="utf-8") as f:
    # json.dump(included_pids, f, separators=(",", ":"))  # minimized JSON
    json.dump(included_pids, f, indent=4, separators=(",", ":"))  # Pretty JSON

print(f"Processed and saved to {output_path}")
