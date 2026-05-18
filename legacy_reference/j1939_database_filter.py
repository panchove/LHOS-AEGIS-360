import json

# File paths
input_file_path = "j1939_utf8.json"  # Input file
output_included_path = "j1939_included_pgn.json"  # Included PGNs JSON file
output_excluded_path = "j1939_excluded_pgn.json"  # Excluded PGNs JSON file

# Load the original JSON
with open(input_file_path, "r", encoding="utf-8") as f:
    original_data = json.load(f)

# Initialize separate datasets
included_pgns = {"p": []}
excluded_pgns = {"p": []}

# Set filtered PGN list
# pgnFilterList = [59904, 59392, 60416, 60160, 60928, 65240, 65226, 65227, 65228, 55552, 55296, 55040,
#                 65242, 64965, 65259, 65040, 65041, 65042, 65043, 65044, 65045, 65046, 65047, 65048,
#                 65049, 65050, 65051, 65052, 65053, 65054, 65055, 65312, 65313, 65314, 65315, 65316,
#                 65317, 65318, 65319, 65320, 65321, 65322, 65323, 65324, 65325, 65326, 65327, 65072,
#                 65073, 65074, 65075, 65076, 65077, 65078, 65079, 65080, 65081, 65082, 65083, 65084,
#                 65085, 65086, 65087, 50688, 65087, 65262, 65271, 61444, 64966, 65257, 65266, 65263,
#                 61443, 65269, 65130, 65164, 65110, 65252, 65247, 64735, 65190, 65276, 64775, 254, 61441]
pgnFilterList = [59904, 59392, 60416, 60160, 60928, 65240, 65226, 65227, 65228, 55552, 55296, 55040,
                65242, 64965, 65259, 65040, 65041, 65042, 65043, 65044, 65045, 65046, 65047, 65048,
                65049, 65050, 65051, 65052, 65053, 65054, 65055, 65312, 65313, 65314, 65315, 65316,
                65317, 65318, 65319, 65320, 65321, 65322, 65323, 65324, 65325, 65326, 65327, 65072,
                65073, 65074, 65075, 65076, 65077, 65078, 65079, 65080, 65081, 65082, 65083, 65084,
                65085, 65086, 65087, 50688, 65087, 65262, 65271, 61444, 64966, 65257, 65266, 65263,
                61443, 65269, 65130, 65164, 65110, 65252, 65247, 64735, 65190, 65276, 64775, 254, 61441, 64914, 65021, 65024, 65027, 65030, 65223]

# Function to modify the label format
def format_label(label):
    if label:
        return "j1939." + label.replace("_", ".")
    return None

# Process the input data
index = 0
# for param in original_data.get("params", []):
#     filtered_param = {
#         "p": param.get("pgn"),  # Renamed `pgn` to `p`
#         "n": param.get("name"),  # Renamed `name` to `n`
#         "i": index,  # Add an index to the parameter
#         "s": [
#             {
#                 "l": format_label(signal.get("label")),  # Transform label
#                 "sb": signal.get("startBit"),  # Renamed `startBit` to `sb`
#                 "bl": signal.get("bitLength"),  # Renamed `bitLength` to `bl`
#                 "f": signal.get("factor"),  # Renamed `factor` to `f`
#                 "o": signal.get("offset"),  # Renamed `offset` to `o`
#                 "mn": signal.get("min"),  # Renamed `min` to `mn`
#                 "mx": signal.get("max"),  # Renamed `max` to `mx`
#                 "u": signal.get("sourceUnit"),  # Renamed `unit` to `u`
#                 # include "st" key only if "states" key exists
                    
#                 "st": [
#                     {
#                         "v": state.get("value"),  # Renamed `value` to `v`
#                         "st": state.get("state"),  # Renamed `state` to `st`
#                     }
#                     for state in signal.get("states", [])  # Added default value
#                 ]
#             }
#             for signal in param.get("signals", [])
#         ]
#     }
for param in original_data.get("params", []):
    filtered_param = {
        "p": param.get("pgn"),  # Renamed `pgn` to `p`
        "n": param.get("name"),  # Renamed `name` to `n`
        "i": index,  # Add an index to the parameter
        "s": [
            {
                "l": format_label(signal.get("label")),
                "sb": signal.get("startBit"),
                "bl": signal.get("bitLength"),
                "il": signal.get("isLittleEndian"),
                "is": signal.get("isSigned"),
                "f": signal.get("factor"),
                "o": signal.get("offset"),
                "mn": signal.get("min"),
                "mx": signal.get("max"),
                "u": signal.get("sourceUnit"),
                **(
                    {
                        "st": [
                            {
                                "v": state.get("value"),
                                "st": state.get("state"),
                            }
                            for state in signal["states"]
                        ]
                    } if "states" in signal else {}
                )
            }
            for signal in param.get("signals", [])
        ]
    }

    gPGN = param.get("pgn")
    if gPGN in pgnFilterList:  # Included PGNs
        included_pgns["p"].append(filtered_param)
    else:  # Excluded PGNs
        excluded_pgns["p"].append(filtered_param)
    index += 1

# Write the filtered PGNs JSON file
with open(output_included_path, "w", encoding="utf-8") as f:
    json.dump(included_pgns, f, separators=(",", ":"))  # minimized JSON
    # json.dump(included_pgns, f, indent=2, separators=(",", ":"))  # Pretty JSON

# Write the excluded PGNs JSON file
with open(output_excluded_path, "w", encoding="utf-8") as f:
    json. dump(excluded_pgns, f, separators=(",", ":"))  # Minimized JSON
    # json.dump(excluded_pgns, f, indent=2, separators=(",", ":"))  # Pretty JSON

print("Done!")
