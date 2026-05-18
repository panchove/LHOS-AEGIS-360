# TESTING STRATEGY

## Policy

- All C++ tests must be ESP-IDF/Unity-based.
- No plain CMake, target_link_libraries, add_executable para tests.
- Test sources están localizados en `tests/` como componentes ESP-IDF.
- Static, heapless, contractual checks enforced in tests.
- Test app link must validate for compliance (make test).

## Running Tests
- Use `make test` para disparar `idf.py build && idf.py test` si aplica.
- Si `idf.py test` no está disponible, `make test` al menos chequea que los tests compilan.
