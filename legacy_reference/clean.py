""" Scan and clean files """
import os
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent

scan_dir = os.path.join(BASE_DIR, 'lib')

for root, dirs, files in os.walk(scan_dir):
  for file in files:
    if file.endswith(':Zone.Identifier'):
      print(f'Cleaning {file}')
      os.remove(os.path.join(root, file))
