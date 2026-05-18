import json
import re

def slugify_parameter(param: str) -> str:
  """Convert 'Parameter Name' → 'parameter.name' lowercase."""
  # Convierte el parámetro a formato slug: minúsculas y puntos como separadores
  s = re.sub(r"[^\w]+", ".", param.strip().lower())
  s = re.sub(r"\.{2,}", ".", s).strip(".")
  return s

def main()-> None:
  # Archivo de entrada con los registros originales
  file_name = "modbus_result.json"
  try:
    with open(file_name, "r", encoding="utf-8") as f:
      data = json.load(f)
  except Exception as e:
    print(f"Error al leer {file_name}: {e}")
    return

  # Validar que el archivo contiene un array de objetos
  if not isinstance(data, list):
    print("El archivo JSON no es un array de objetos.")
    return

  result = []
  filter_list = [40010, 40012, 40013, 40018, 40019, 40020, 40021, 40022, 40023, 40024, 40025,
                40026, 40027, 40028, 40029, 40031, 40032, 40033, 40034, 40035,
                40036, 40037, 40038, 40039, 40040, 40041, 40042, 40043, 40044, 40048,
                40061, 40062, 40063, 40064, 40065, 40067, 40068, 40070, 40071, 40237, 40283, 40500, 43745, 43746]
  # Procesar cada objeto del array
  for obj in data:
    # Obtener y transformar campos principales
    parameter = obj.get('Parameter', '')
    label = slugify_parameter(parameter)
    addr = obj.get('Addr.', None)
    addr_int = int(addr) if addr is not None else 0
    if addr_int not in filter_list:
      print(f"Skipping {addr} as it is not in the filter list.")
      continue
    try:
      # Calcular offset a partir de la dirección
      address_offset = addr_int - 40000 if addr != 0 else 0
    except Exception:
      address_offset = 0
    spec = obj.get('Specifications', '')

    # Extraer estados si existen en Specifications
    state_matches = re.findall(r'(\d+)\s*[:=]\s*([^\n\r]+)', spec)
    if state_matches:
      states = []
      for id_str, name in state_matches:
        try:
          id_val = int(id_str)
        except Exception:
          continue
        states.append({"i": id_val, "n": name.strip()})
      # Si hay estados, solo se agregan esos campos
      new_obj = {
        "l": label,
        "ao": address_offset,
        "st": states,
      }
    else:
      # Si no hay estados, extraer todos los valores de Specifications
      spec_dict = {}
      # Valores por defecto
      spec_dict['m'] = 1
      spec_dict['o'] = 0
      spec_dict['bl'] = 16
      spec_dict['is'] = False
      spec_dict['dt'] = 'int'
      spec_dict['min'] = 0
      spec_dict['max'] = 0
      spec_dict['u'] = ''
      # Multiplier: acepta guion normal y guion largo Unicode, y también si solo es el número
      m = re.search(r"Multiplier\s*:\s*([-\d.Ee–]+)", spec)
      if not m:
        m = re.search(r"([\d.Ee\-–]+)", spec)
      if m:
        multiplier_raw = m.group(1).replace('–', '-')
        try:
          spec_dict['m'] = float(multiplier_raw)
        except Exception:
          spec_dict['m'] = multiplier_raw
      # Offset
      o = re.search(r"Offset:\s*([\d.Ee\-–]+)", spec)
      if o:
        spec_dict['o'] = o.group(1)
      # Size (bits)
      s = re.search(r"Size \(bits\):\s*(\d+)", spec)
      if s:
        spec_dict['bl'] = int(s.group(1))
      # Sign
      sign = re.search(r"Sign:\s*([SU])", spec)
      if sign:
        spec_dict['is'] = True if sign.group(1) == 'S' else False
      # Unit
      # unit = re.search(r"Unit:\s*([%\w]+)", spec)
      unit = re.search(
          r"Units?\s*:?\s*(.*?)\s*(?=\n?\s*(?:Lower|Upper|Default|Offset|Size|Sign|Multiplier|This)\b|$)",
          spec,
          re.IGNORECASE | re.DOTALL,
      )
      if unit:
        spec_dict['u'] = unit.group(1)
      # Lower Limit
      minv = re.search(r"Lower Limit:\s*([\d.Ee\-–]+)?", spec)
      if minv and minv.group(1):
        try:
          spec_dict['min'] = float(minv.group(1).replace('–', '-'))
        except Exception:
          spec_dict['min'] = minv.group(1)
      # Upper Limit
      maxv = re.search(r"Upper Limit:\s*([\d.Ee\-–]+)?", spec)
      if maxv and maxv.group(1):
        try:
          spec_dict['max'] = float(maxv.group(1).replace('–', '-'))
        except Exception:
          spec_dict['max'] = maxv.group(1)
      # Construir el objeto final con todos los campos
      new_obj = {
        "l": label,
        "ao": address_offset,
        "bl": spec_dict['bl'],
        "is": spec_dict['is'],
        "m": spec_dict['m'],
        "o": spec_dict['o'],
        "dt": spec_dict['dt'],
        "min": spec_dict['min'],
        "max": spec_dict['max'],
        "u": spec_dict['u']
      }
    result.append(new_obj)
  # Guardar el resultado en un archivo JSON
  with open("holding_registers.json", "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2, ensure_ascii=False)

if __name__ == "__main__":
  main()

