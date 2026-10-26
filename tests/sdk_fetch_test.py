#!/usr/bin/env python3
"""Network-free checks for SDK acquisition retries and fail-closed behavior."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

HELPER = Path(__file__).resolve().parents[1] / 'tools/get_ac79_sdk.sh'

class SdkFetchTest(unittest.TestCase):
    def invoke(self, succeed, exists=False):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            bindir = root / 'bin'
            bindir.mkdir()
            count = root / 'count'
            target = root / 'sdk'
            if exists:
                target.mkdir()
            (bindir / 'git').write_text('''#!/bin/sh
n=0
[ ! -f "$COUNT" ] || n=$(cat "$COUNT")
n=$((n+1))
echo "$n" > "$COUNT"
for target; do :; done
if [ "$SUCCEED" = 1 ] && [ "$n" = 3 ]; then
 mkdir -p "$target/cpu/wl82/tools"
 touch "$target/cpu/wl82/tools/uboot.boot"
 exit 0
fi
exit 1
''')
            (bindir / 'sleep').write_text('''#!/bin/sh
exit 0
''')
            for tool in bindir.iterdir():
                tool.chmod(0o755)
            env = dict(os.environ, PATH=str(bindir)+os.pathsep+os.environ['PATH'],
                       COUNT=str(count), SUCCEED=str(int(succeed)))
            result = subprocess.run(['sh', str(HELPER), str(target)], env=env,
                                    capture_output=True, text=True)
            return result.returncode, int(count.read_text()) if count.exists() else 0, (target / 'cpu/wl82/tools/uboot.boot').exists()

    def test_third_attempt_succeeds(self):
        self.assertEqual(self.invoke(True), (0, 3, True))

    def test_failure_is_bounded(self):
        self.assertEqual(self.invoke(False), (1, 3, False))

    def test_existing_destination_is_preserved(self):
        self.assertEqual(self.invoke(True, True), (1, 0, False))

if __name__ == '__main__':
    unittest.main()
