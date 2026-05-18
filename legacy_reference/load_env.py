"""Load env"""
# ruff: noqa

Import('env')
from pathlib import Path

try:
  from dotenv import load_dotenv
except ImportError:
  import os
  import sys

  os.system(f'{sys.executable} -m pip install python-dotenv')
  from dotenv import load_dotenv

BASE_DIR = Path(env['PROJECT_DIR'])

envfile = BASE_DIR / '.env'
if envfile.exists():
  print(f'Loading environment variables from {envfile}')
  load_dotenv(envfile)
