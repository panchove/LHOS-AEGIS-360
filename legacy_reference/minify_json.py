import json
import sys
import os

def minify_json_file(json_path):
    # Check the file exists and has .json extension
    if not os.path.isfile(json_path) or not json_path.endswith('.json'):
        print(f"Error: '{json_path}' is not a valid JSON file.")
        return

    # Load original JSON
    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    # Create output filename
    base, ext = os.path.splitext(json_path)
    output_path = f"{base}_minimal{ext}"

    # Write minified version
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, separators=(',', ':'), ensure_ascii=False)

    print(f"Minified JSON saved to: {output_path}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python minify_json.py <path/to/input.json>")
    else:
        minify_json_file(sys.argv[1])
