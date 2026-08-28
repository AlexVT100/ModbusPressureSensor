from datetime import datetime
import re
from platformio.util import exec_command
Import('env')

result = exec_command(['git', 'describe', '--tags', '--match=firmware-*', 'HEAD'])

if result["out"] and not result["err"]:
    match = re.match(r'firmware-[vV](\d+\.\d+\.\d+)(-\d+-g[0-9a-f]+)?$', result["out"].strip())
    revision = match.group(1)
    if match.group(2):
        # The tag is not at HEAD; check, if there is real code difference in firmware/ between HEAD and tag
        result = exec_command(['git', 'diff', f'firmware-V{revision}..HEAD'])
        if result["out"]:
            # There is difference between the code at HEAD and at tag. Append the 2nd part
            revision += match.group(2)
else:
    # Fallback
    result = exec_command(['git', 'branch', '--show-current'])
    branch = result['out'].strip()

    result = exec_command(['git', 'rev-parse', '--short', 'HEAD'])
    commit = result['out'].strip()

    revision = f'{branch}:{commit}'

timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

env.Append(CPPDEFINES=[
        ('GIT_REVISION', env.StringifyMacro(revision)),
        ('BUILD_TIME', env.StringifyMacro(timestamp)),
    ])
print(f'--> Dynamic Build Flags Added: -DGIT_REVISION="{revision}", -DBUILD_TIME="{timestamp}"')
