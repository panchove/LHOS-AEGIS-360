import json
from docx import Document

def main():
  docx_file_name = "Cummins-PowerCommand-2.2-2.3-3.3-modbus-register-mapping-503-845.docx"
  
  try:
    document = Document(docx_file_name)
    print(f"Archivo DOCX '{docx_file_name}' abierto con éxito.")
  except Exception as e:
    print(f"ERROR: No se pudo abrir el archivo DOCX: {e}")
    return

  all_data = []
  found_valid_table = False
  encabezado_esperado = [
    "Addr.", "Parameter", "Access", "Specifications", "Description", "Control"
  ]

  # Iterar a través de todas las tablas en el documento
  for table_index, table in enumerate(document.tables):
    print(f"DEBUG: Procesando la tabla {table_index + 499}.")
    
    # Obtener el encabezado de la tabla (la primera fila)
    try:
      headers = [cell.text.strip() for cell in table.rows[0].cells]
    except IndexError:
      print(f"DEBUG: La tabla {table_index + 1} no tiene filas. Saltando.")
      continue
    
    print(f"DEBUG: Encabezado extraído de la tabla {table_index + 1}: {headers}")
    
    # Validar que el encabezado coincida con el esperado
    if headers[:len(encabezado_esperado)] == encabezado_esperado:
      print(f"DEBUG: ¡Encabezado de la tabla {table_index + 1} validado con éxito!")
      found_valid_table = True
      
      # Iterar a través de las filas de datos (excluyendo el encabezado)
      for row_index, row in enumerate(table.rows[1:]):
        row_dict = {"pagina": table_index + 499}
        row_data = [cell.text.strip() for cell in row.cells]
        min_len = min(len(headers), len(row_data))
        for i in range(min_len):
          row_dict[headers[i]] = row_data[i]
        for i in range(min_len, len(headers)):
          row_dict[headers[i]] = ""
        all_data.append(row_dict)
      # No break: procesamos todas las filas de todas las tablas válidas

  if not found_valid_table:
    print("ADVERTENCIA: No se encontró ninguna tabla que cumpla los criterios.")

  if all_data:
    with open("modbus_result.json", "w", encoding="utf-8") as f:
      json.dump(all_data, f, indent=2, ensure_ascii=False)
    print(f"\nExtracción finalizada. Datos guardados en 'modbus_result.json'.")
  else:
    print("ADVERTENCIA: No se extrajo ninguna tabla en el rango especificado.")

if __name__ == "__main__":
  main()