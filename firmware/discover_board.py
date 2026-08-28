import json
import re
from platformio.util import exec_command
Import("env")

# 1. Run 'pio device list --json-output' programmatically
result = exec_command(["pio", "device", "list", "--json-output"])
devices = json.loads(result["out"])

# Parse the connected device parameters
for device in devices:
    if (m := re.search(r"VID:PID=([0-9A-F]{4}):([0-9A-F]{4})", device.get("hwid", ""))):
        macro = f"BOARD_{m.group(1)}_{m.group(2)}"
        break
else:
    macro = "BOARD_NONE"
    print(f"--> No Board Is Connected")

env.Append(CPPDEFINES=[macro])
print(f"--> Dynamic Build Flag Added: -D{macro}")
