# Glossary

| Term | Meaning |
|---|---|
| Agent | An AI model that writes game scripts through the script API and MCP. |
| Vibecoder | End user who describes a game in natural language; never writes code. |
| Core | Dependency-free C++ domain code in `src/core`; pure, fast to test. |
| Entity | ECS identifier (EnTT) for a game object; scripts see an opaque handle. |
| Script API | The QuickJS-ng surface agents program against. The product. |
| Gate | `tools/gate.py`: build + tests + limits + CRAP + duplication. |
