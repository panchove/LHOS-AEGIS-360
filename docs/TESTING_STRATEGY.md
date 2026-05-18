# TESTING STRATEGY

## Policy

- All C++ tests must be ESP-IDF/Unity-based.
- No plain CMake, target_link_libraries, add_executable para tests.
- Test sources están localizados en `tests/` como componentes ESP-IDF.
- Static, heapless, contractual checks enforced in tests.
- Test app link must validate for compliance (make test).

## Running Tests
- Use the unified ESP-IDF commands for building and running tests.
- Recommended command for CI/local: `idf.py build && idf.py -T all test`
- If `idf.py -T all test` is not available in a particular component, run `idf.py build` and the component-level tests separately using `idf.py -T <test_name> test`.
