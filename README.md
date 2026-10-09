# libespnow 0.1.1

An original, readable C implementation of the specified ESP-NOW lifecycle,
peer/key management, send/callback dispatch, rate validation and off-channel
adapters. Applications keep including ESP-IDF's `esp_now.h` and calling
`esp_now_*`. The component builds `libespnow.a` and replaces the pinned vendor
archive through the supplied CMake hook.

This is a procedural specification/implementation split, not a legal
certification. Implementation-side inputs are recorded by SHA-256 in
`PROVENANCE.json`. No original executable, disassembly, reconstructed library,
network service or hardware was accessed by the implementation author.

## Scope and status

The target ABI is **ESP32-S3 / ESP-IDF 6.0.3** only. The portable core is tested
on a 64-bit host with strict warnings, AddressSanitizer and UndefinedBehaviorSanitizer.
Host layout is not the target ABI. Target-only assertions check the supplied
32-bit public/private layouts against the real SDK headers.

This component still requires the SDK MAC/PHY/radio, crypto and OS adapters.
It does not implement the complete Wi-Fi stack, packet CCMP, over-air framing,
channel hardware or PHY. Host tests use explicitly artificial crypto and radio
mocks. Passing them establishes neither AES correctness nor over-air
interoperability. Independent supervisor target checks and two-board results are summarized in
`validation/` and `docs/VALIDATION.txt`. Stock/stock, replacement/stock,
stock/replacement and replacement/replacement passed cold and warm boots:
272 byte-verified deliveries (144 encrypted), lengths 32/250/251/1470, and 16
wrong-key rejection observations. These bounded tests do not establish all
inputs, timing, concurrency, rate/offchannel combinations or RF performance.

## Host verification

```sh
make test
```

The build enables `-Wall -Wextra -Werror -Wconversion -Wshadow
-Wstrict-prototypes`, ASan and UBSan. If execution runs under ptrace and
LeakSanitizer reports that it cannot operate, use:

```sh
ASAN_OPTIONS=detect_leaks=0 make test
```

This environment required that fallback. ASan/UBSan remain enabled; the mock
allocator also asserts that all owned allocations are released. The suite
covers failure injection, callback timing, peer/key/rate rollback and 4,000
independently modeled peer operations. See `docs/TESTING.txt`.

## Using the replacement in an IDF application

Place this directory under the application's `components/libespnow`, or add it
to `EXTRA_COMPONENT_DIRS` before the application `project()` call. Ensure the
component participates in the build: add `libespnow` to your main component's
`REQUIRES`, or include it in `COMPONENTS` if that list is restricted.

After `project(...)`, include:

```cmake
include("${CMAKE_CURRENT_LIST_DIR}/components/libespnow/cmake/replace_vendor_archive.cmake")
```

Adjust that relative location if using `EXTRA_COMPONENT_DIRS`. The hook verifies
the target/version and changes the existing imported `esp_wifi_espnow` target's
`IMPORTED_LOCATION` to the built replacement archive. It keeps the SDK's link
dependencies. It does not leave a second vendor ESP-NOW archive competing for
symbols. Do not add a direct dependency from the imported target to the source
component: the SDK's mutually linked static libraries make that a strong cycle.

The component must be linked normally so its archive is built before final
linking. Inspect the final map: ESP-NOW definitions must come from this
component's `libespnow.a`, with no vendor `libespnow.a` members. This hook is
specific to the pinned SDK and must not be silently carried into another ABI.

`examples/basic` is a small IDF application that starts STA Wi-Fi on channel 6,
registers a receive callback, adds a plaintext broadcast peer and submits one
message. Submission success means queued; peer receipt is asynchronous.

## Deliberate behavior changes and limits

* `esp_now_set_user_oui(NULL)` restores `18:fe:34` as the public header promises.
* PMK changes are rejected with `ESP_ERR_ESPNOW_ARG` while any encrypted peer
  exists, even if the supplied PMK equals the existing one. Remove encrypted
  peers first; set the new PMK; then re-add them.
* Failed peer/key/rate updates leave the old configuration intact. Crypto
  failure never installs or exports a fabricated plaintext key.
* Padding and reserved bytes are initialized. OUI copies handle overlap.
* Wake helper return values are ignored, following the supplied baseline
  policy. The recorded requested window can differ from actual hardware state
  after a lower-driver failure.
* `esp_now_send(NULL, ...)` visits registered unicast peers in insertion order,
  skips multicast/broadcast peers, stops at the first error and returns
  `ESP_ERR_ESPNOW_INTERNAL` for that partial-send failure. No eligible peers
  returns `ESP_ERR_ESPNOW_NOT_FOUND`.
* Initialization and first use of the retained public mutex must be serialized.
  Driver callbacks do not acquire it. Until driver borrow lifetimes are audited,
  externally serialize peer/rate mutations and deinitialization with outstanding
  transmissions/callbacks. See `docs/SEMANTICS.txt` for the exact constraints.
* Rate validation includes specified model branches for channels above 14 and
  HE/ERSU/DCM; this makes no claim of S3 support for those radio capabilities.

## License

New implementation, tests (except adapted public type declarations), example
and build files are MPL-2.0 under `LICENSE`. The supplied SDK header/type excerpts
and the adapted `tests/host/esp_wifi_types.h` retain Apache-2.0, whose text is in
`spec/public-sdk/LICENSE`. `PROVENANCE.json` records that boundary.
