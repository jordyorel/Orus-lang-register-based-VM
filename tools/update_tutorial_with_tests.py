#!/usr/bin/env python3
import os
from pathlib import Path

tutorial = Path('docs/TUTORIAL.md')

with tutorial.open() as f:
    lines = f.read().splitlines()

new = []
stop_headers = {'## Test Examples', '## Full Test Listing', '## All Tests'}
for line in lines:
    if line.strip() in stop_headers:
        break
    new.append(line)

new.append('')
new.append('## Test Examples')
new.append('The following section lists every test case included with the repository. Each one is shown in full so you can study real-world usage of every feature.')

for root, _, files in sorted(os.walk('tests')):
    for name in sorted(files):
        if not name.endswith('.orus'):
            continue
        path = Path(root)/name
        rel = path.as_posix()
        new.append('')
        new.append(f'### `{rel}`')
        explanation = []
        with open(path) as fh:
            code_lines = fh.readlines()
        i = 0
        while i < len(code_lines) and code_lines[i].lstrip().startswith('//'):
            explanation.append(code_lines[i].lstrip('/').strip())
            i += 1
        if explanation:
            new.append('\n'.join(explanation))
        new.append('```orus')
        new.extend(line.rstrip('\n') for line in code_lines)
        new.append('```')

with tutorial.open('w') as f:
    f.write('\n'.join(new) + '\n')
