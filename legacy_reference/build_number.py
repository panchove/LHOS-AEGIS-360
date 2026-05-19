import os
import time
from SCons.Script import Import

Import("env")

build_number = os.getenv("LAYRZ_BUILD_NUMBER")
build_id = os.getenv("BUILD_ID")

if build_number is None:
    try:
        import requests

        print("[INFO] No LAYRZ_BUILD_NUMBER in environment. Fetching from API...")

        url = f"https://api.layrz.com/firmwares/{build_id}"
        response = requests.get(url, timeout=5)

        if response.status_code == 200:
            builds = response.json()
            if builds:
                # Get the max "build" value
                build_number = max(entry.get("build", 0) for entry in builds) + 1
            else:
                print("[WARN] API returned empty list.")
                build_number = 0
        else:
            print(f"[WARN] API returned status {response.status_code}")
            build_number = 0

    except Exception as e:
        print(f"[ERROR] Failed to fetch build number from API: {e}")
        build_number = 0

# Ensure it's a string for appending as flag
build_number = str(build_number)

print(f"[INFO] BUILD_NUMBER set to: {build_number}")

# Inject macro into PlatformIO environment
env.Append(
    BUILD_FLAGS=[f"-DBUILD_NUMBER={build_number}"]
)
