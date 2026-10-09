This directory is the implementation side of a specification/implementation
split. Use only files inside this directory as task knowledge. Do not inspect
parent or sibling research, reconstruction, disassembly, original radio binary
archives, prior implementations, private tests/oracles, credentials, or other
agents' output. No network calls, no hardware access, no external uploads.
Standard installed compilers/system headers may be used for host builds.
Permitted inputs: spec/*.txt and spec/public-sdk/*, plus later explicitly
provided supervisor feedback/tests here. Record input paths and content hashes
in PROVENANCE.json. Public SDK interface excerpts retain their Apache2.0
license; newly authored implementation isMPL-2.0 (LICENSE). Write original,
readable semantic C, not register-machine transcription. Only edit this tree.
Project/component name: libespnow. Public API: existing esp_now_* names from
installed ESP-IDF esp_now.h. Do not invent a renamed application API.
Private mt_* and globals are retained solely where binary compatibility calls
for them, with SDK-specific layouts isolated and documented. The supervisor
will build against actual SDK, independently review, and run hardware tests.
