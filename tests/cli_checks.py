"""Focused end-to-end checks. Build signallab first; no third-party packages."""
import os
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

exe = str(Path(sys.argv[1] if len(sys.argv) > 1 else
               ('signallab.exe' if os.name == 'nt' else 'signallab')).resolve())
count = 0


def run(lines, expected, code=0):
    global count
    result = subprocess.run([exe], input=lines, text=True, capture_output=True, timeout=20)
    text = result.stdout + result.stderr
    assert result.returncode == code, text
    for value in expected:
        assert value in text, text
    count += 1
    return text


for choice, name in enumerate(('NRZ-L', 'NRZ-I', 'Manchester', 'Differential Manchester', 'AMI'), 1):
    extra = '1\n' if choice == 5 else ''  # AMI without scrambling.
    run(f'1\n1\n101101\n{choice}\n{extra}2\n',
        [name + ' voltage levels:', 'Starting position (zero-based): 0',
         'Palindrome length: 6', 'Decoded stream: 101101', 'Round trip: MATCH'])

run('1\n1\n100000000\n5\n2\n1\n2\n',
    ['AMI before scrambling:', 'B8ZS voltage levels: +1 0 0 0 +1 -1 0 -1 +1', 'Round trip: MATCH'])
for bits, levels in (('0000', '+1 0 0 +1'), ('10000', '+1 0 0 0 +1'),
                     ('110000', '+1 -1 +1 0 0 +1')):
    run(f'1\n1\n{bits}\n5\n2\n2\n2\n', ['HDB3 voltage levels: ' + levels, 'Round trip: MATCH'])

for mode, length in ((1, 16), (2, 4), (3, 8)):
    text = run(f'1\n2\n{mode}\n{length}\n1\n2\n', ['Round trip: MATCH'])
    data = re.search(r'Binary stream: ([01]+)', text).group(1)
    assert len(data) == length
    if mode > 1:
        assert '0' * length in data
run('1\n2\n1\n0\n', ['Palindrome length: 0', 'Empty stream:'])

run('2\n1\n3\n4\n2\n-1\n1\n-1\n0\n1\n2\n3\n2\n',
    ['PCM bitstream (MSB first, grouped by sample): 00 10 11',
     't=0.25 s: input 0, decoded 0.25', 'Decoded stream: 001011', 'Round trip: MATCH'])
run('2\n2\n8\n8\n3\n-1\n1\n1\n', ['Input samples:', 'Staircase: output/pcm_staircase.svg'])
run('3\n-1 +1 -1 -1 +1 -1\n1\n', ['Decoded stream: 101101'])

for invalid in ('x\n', '1\n9\n', '1\n1\n102\n', '1\n2\n3\n7\n',
                '1\n2\n1\n-2\n', '3\n1e\n', '3\n1 1\n3\n',
                '2\n1\n1\n4\n2\n1\n1\n',
                '2\n1\n1\n4\n2\n-1\n1\n2\n', ''):
    run(invalid, ['Error:'], code=1)

for path in ('output/line_waveform.svg', 'output/pcm_staircase.svg'):
    root = ET.parse(path).getroot()
    assert root.tag.endswith('svg') and len(root.findall('.//{*}polyline')) >= 2
print(f'All {count} CLI runs passed; both exported SVGs parsed successfully.')
