# Handover

## Start Here

This repo is now developed inside the ESPHome devcontainer. Read:

1. `AGENTS.md` for operational rules and verification commands.
2. `docs/features.md` for project status and next feature slices.
3. `docs/testing/backlog.md` for detailed testing tasks.
4. `docs/protocol/menu-packet-map.md` before protocol, helper, or active-control changes.
5. `docs/protocol/research-backlog.md` for uncertain fields that need human analysis.
6. `analysis/README.md` for field annotator, firmware web dashboard, annotation export, and screenshot capture workflow.
7. `docs/hwp-simulator.md` for user-facing simulator setup, entities, playbooks, commands, and current echo boundary.
8. `docs/tmp-hwp-salvage.md` only if a task explicitly needs archival tmp reference material.

## Current State

- ESPHome external component lives in `components/hwp/`.
- The component revision stamp lives in `components/hwp/hwp_version.h`. It is logged at setup and shown in `dump_config` as `component_revision`; bump it when asking Home Assistant users to verify that a rebuilt firmware really picked up this repo's latest component code.
- Devcontainer builds from `ghcr.io/esphome/esphome:latest`, adds declared native/QEMU-adjacent tools, and keeps generated firmware output outside the repo through `/build`.
- ESPHome compile fixtures, Python schema tests, packet fixture validation, native protocol tests, runners, and CI are in place.
- Headless analysis tooling is packaged under `analysis/`. Use `python -m analysis.hwp_analyze fixtures`, `log`, `prove`, `active-tx`, and `prove-active-tx` to validate fixtures, summarize hardware logs, and prove fixture packets appear in traces.
- The field annotator GUI is available through `python -m analysis.hwp_logs_annotator`. Workstation use outside the devcontainer needs `python -m pip install -r requirements-annotator.txt`; this intentionally installs `aioesphomeapi` plus lightweight PyYAML, not the full ESPHome package. `--help`, `--setup` without `--yaml`, and `--show-profiles` avoid live API imports. First-time setup can use `python -m analysis.hwp_logs_annotator --setup`, which writes `.esphome-config/hwp-tools.json`; `--show-profiles` lists saved profiles without API keys. It also resolves CLI args, environment variables, or legacy YAML fallback. See `docs/analysis-field-annotator.md`.
- The annotator right pane has Dashboard, Packets, and Graphs tabs. The dashboard and graph views use `analysis.hwp_field_dashboard` to decode known fields from the shared parser plus menu/packet metadata; the packet viewer still uses `analysis.hwp_packet_view` and highlights checksum failures, fixture matches, known byte locations, and changed bytes.
- The annotator can change the live ESPHome API log subscription level, request dump-config refresh, and invoke restart through a discovered ESPHome restart button entity. Devices need a `button: - platform: restart` entity for restart; there is no generic native API reboot method used by the tool.
- The component can also serve a lightweight read-only HWP dashboard from the firmware when ESPHome `web_server:` is configured. The optional climate config block is:

```yaml
web_server:
  auth:
    username: !secret web_user
    password: !secret web_password

climate:
  - platform: hwp
    web_ui:
      enabled: true
```

  The default path is `/hwp`; `/hwp/state.json` returns the latest decoded fields, latest frame snapshots, packet buffer, retained graph samples, revision, bus mode, and status; `/hwp/events` streams SSE updates. It is a tablet-friendly field-analysis view only, not a heater-control or annotation surface.
- The firmware web dashboard has **Values**, **Frames**, **Packets**, and **Graphs** tabs. `Frames` is a browser-independent latest-frame snapshot for each frame/source/length, including unknown packets. `Graphs` uses a firmware-retained sliding sample window so a newly opened browser starts with recent data; the browser draws this with the embedded canvas renderer, not an external chart CDN. The browser-local **Annotate** helper is fixed to the bottom of the viewport; it stores `Start`, `End`, and `Mark Event` entries in `localStorage`, exports JSON from the browser, and does not persist data on the ESP32. Keep raw exports local unless a field session produces evidence worth curating into fixtures or notes.
- Web UI screenshots can be regenerated from a live device with `python -m analysis.hwp_web_capture --base-url http://<device-address>/hwp --out-dir analysis/screenshots --readme-size tablet`. `analysis/screenshots/*` is generated and ignored by default except for the curated README tablet image. The analysis workflow and safety notes live in `analysis/README.md`.
- A first-class simulator component now lives in `components/hwp_simulator/`. It is a separate ESPHome component for a separate ESP32 bridge node, with an entity-first API surface for playbook, active state, interval scale, command input, start/pause/step/reset/inject buttons, and diagnostic sensors. `examples/hwp-simulator.yaml` compiles as the simulator fixture.
- User-facing simulator setup is documented in `docs/hwp-simulator.md`; keep that file aligned with `examples/hwp-simulator.yaml` and the component schema when changing simulator options or entities.
- The simulator engine is fixture-derived and native-tested. Playbooks cover `normal_idle`, `config_refresh`, `active_defrost_echo`, `rx_stress`, and `paused`; config registry write/echo behavior is pinned to command fixtures and active-TX evidence. The simulator emits playbook packets through software-timed open-drain GPIO pulses and listens for controller-originated packets through ESP-IDF 5 RMT RX on the configured half-duplex pin.
- Simulator RX/listen support follows the heat-pump split between sensor/status families and config registries: checksum-valid controller writes to `CONFIG_1` through `CONFIG_6` update the simulator's replayed runtime config state, and only the first changed registry packet in a decoded burst is echoed back after the controller burst. The writable registry inventory is tracked in `docs/protocol/simulator-config-registry-inventory.md`. The RX path decodes repeated controller command groups and de-duplicates identical repeats before updating state. Unknown valid controller packets update diagnostics only; invalid checksum/length packets increment simulator errors and do not produce synthetic heater behavior.
- Simulator orchestration should use ESPHome native API entities instead of raw UART whenever possible. Start with `python -m analysis.hwp_sim_orchestrate --device <sim-node> list`, then use `set-playbook` or `command` against the simulator text entity. Use `python -m analysis.hwp_sim_orchestrate --device <sim-node> transcript --duration 60 --out sim-session.jsonl --firmware-device <firmware-node>` to capture a two-node API transcript. Workstation use needs `aioesphomeapi`, the same optional dependency used by the GUI annotator.
- Manual annotation windows are parsed by the same CLI. Use `python -m analysis.hwp_analyze annotations --input tmp/hwp/POOL_esphome_logs.log.2024-11-01` and `prove-annotations` for curated tagger windows.
- The first native C++ seam lives in `components/hwp/protocol_core.*` and covers dependency-light packet helpers plus fan mode, defrost, flow-meter, and heat-pump restriction conversions.
- Component-local adapter headers keep climate/logger/time/RMT coupling out of core frame tests where practical. `HWP_NATIVE_TEST` uses those adapters to compile runtime frame contracts without full ESPHome or hardware RMT dependencies.
- Curated packet fixtures from `tmp/hwp/analysis/simulator/DemoFrames.h` live under `tests/fixtures/packets/` and validate with stdlib Python tests. Those simulator packets are hardware-derived reference frames, not synthetic-only samples.
- The remaining useful simulator `COND_2` temperature example, `p_28`, is tracked as passive read/decode evidence. It pins current outlet/coil/exhaust/aux decode behavior without proving broader temperature encoding edges.
- A curated subset from the recent ignored `tmp/hwp/POOL_esphome_logs.log` trace is tracked as `tests/fixtures/packets/hwp_hardware_log_2025_06_24.json`; it covers representative real RX/change frames for `0x81`, `0x82`, `0x83`, `0x84`, `0x85`, `0x86`, `0xD1`, `0xD2`, `0xDD`, plus clock/controller frames. The proof CLI found all 15 checksum-valid packets from that fixture in the full ignored log.
- A curated annotation fixture from `tmp/hwp/POOL_esphome_logs.log.2024-11-01` lives under `tests/fixtures/annotations/`; it captures fan-control tagger windows for F01 and F02-F13 as read/write packet contracts. The remaining fan edge windows are now imported: F02 41.0, F03 15.5, F08 1, F10 coil source, F12 50, and F13 99/100.
- Some annotation sessions came from checking read-only technical-menu/status values, not changing writable settings. Treat those as read/decode evidence only unless paired with controller command bytes, simulator command examples, or live echo evidence.
- `tests/fixtures/packets/hwp_demo_command_contracts.json` tracks simulator/demo command examples for `CONFIG_1` R01-R07 and `CONFIG_3` R09-R11. R01/R02/R04-R07 and R09-R11 have one-byte pair contracts; R03 is fixture-backed as an observed byte because the source corpus does not contain a clean one-byte before/after pair.
- `tests/fixtures/packets/hwp_defrost_demo_command_contracts.json` tracks simulator/demo command examples for `CONFIG_2` D01 and `CONFIG_5` D05/D06. D01 and D05 now have passive one-byte command contracts; D06 still has the stronger live echo fixture for active-control claims.
- `tests/fixtures/packets/hwp_config1_temperature_extended_regression.json` tracks the issue #11 high-setpoint regression. `CONFIG_1` R01/R02/R03 setpoints must use extended temperature encoding, with 33.5C/34.0C/35.0C mapping to `0x7F`/`0x80`/`0x82` rather than wrapping through the short temperature format.
- The remaining ignored annotation logs and Arduino simulator packets are inventoried by `python -m analysis.hwp_analyze evidence --limit 25`. The current scan found 65 annotation windows, 54 packet-bearing windows, and 43 simulator packets. All F01-F13 fan packet windows are now tracked; remaining uncovered annotation windows are non-fan 2024-10-31 condition/clock/test snippets.
- Technical manual menu options are mapped to frame/byte/encoding/evidence status in `docs/protocol/menu-packet-map.md`, with machine-readable metadata in `analysis/hwp_menu_map.py`. Check this map before adding or exposing protocol behavior.
- Uncertain or unknown fields are tracked in `docs/protocol/research-backlog.md` so they can be revisited with human review and future analysis tooling instead of drifting into implementation by accident.
- Community installation examples live under `docs/success-stories/`. The tracked reports preserve discussion #9's HP55TR E08 bypass result and discussion #17's HP65A/PC1000 compatibility report, with local photos, as documentation evidence only.
- Fan field candidates from the tmp tree are reviewed in `docs/protocol/fan-field-review.md` and covered by fixture-backed Python tests.
- Runtime decode naming for F02-F09, F10, F11, and F13 is merged in the frame structs. F10/F11 dependency-light conversions are covered in `protocol_core`.
- Runtime `FrameConf1/2/4/5` matching and parsing are covered by adapter-backed native tests against the F01-F13 packet contracts.
- Passive runtime frame contracts are covered by the same adapter-backed native executable. `FrameConditions1/1B/2/2B/D`, `FrameClock`, `FrameConf3`, and `FrameConf6` now match tracked fixture packets; parsed condition temperatures, S02 water flow, and config-3 setpoint limits are compared with pure `protocol_core` helpers.
- Firmware log formatter contracts are covered by native frame tests. Packet headers keep fixed long/short byte layout and changed payload bytes are inverse-highlighted; representative decoded CONFIG, COND, and CLOCK formatters also assert inverse highlighting so future analysis tooling can trust the minimum log shape.
- Fan helper exposure and command parity are merged for F02-F13. Helpers publish decoded values and feed command calls; native byte-helper tests pin F01-F13 read decode and passive command byte mutations before live hardware validation.
- The copied `tmp/hwp` merge track is closed. It is ignored and summarized in `docs/tmp-hwp-salvage.md` as archival reference only.
- Manual hardware-in-the-loop validation gates live in `docs/testing/manual-hil.md`; active fan writes are byte-tested and based on tmp/hardware-derived packet evidence, but each live setting still needs supervised field confirmation before claiming operational safety.
- The bus implementation now uses ESP-IDF 5 RMT RX/TX channels on the same half-duplex GPIO. RX is initialized before TX, both channels use 312.5 kHz resolution, TX uses the copy encoder, and decoder-facing pulse durations are normalized to microseconds through `hwp_pulse_symbol_t`.
- The 312.5 kHz RMT resolution is intentional for classic ESP32: it is the lowest valid rate from an 80 MHz RMT source with the hardware divider capped at 256. A 100 kHz target trips the ESP32 HAL divider assertion at runtime.
- Hardware testing found that disabling the HWP component brings the test device back online, narrowing the remaining reboot loop to HWP setup/runtime. `PoolHeater::setup()` now attaches the data model before starting the bus, `Bus::finalize_frame()` guards against a missing data model, and `start_bus_on_setup: false` can boot HWP entities without starting RX/TX for isolation.
- Follow-up hardware isolation showed no crash with bus startup enabled when no signal is present, pointing at RX traffic handling. The RMT RX callback now copies raw RMT symbol batches into the ring buffer and leaves duration normalization/decoding to the RX task, keeping the interrupt path smaller.
- Supervised hardware validation has advanced: passive RX is stable with bus startup enabled, defrost eco mode active TX changed `CONFIG_5` to `d06 defrost: ECO`, and a follow-up write changed it back to `d06 defrost: NORMAL`. RX remained alive afterward. TX cleanup avoids re-arming RX twice after a send, which previously produced `Failed to arm RMT RX: 259` while recovering.
- The CONFIG_5 write/echo evidence is tracked as `tests/fixtures/active_tx/hwp_active_tx_config5_defrost_2026_05_12.json`. It records the accepted ECO/NORMAL commands, heater echoes, valid checksums, D06 byte-2 bit-6 transitions, and observed heater normalization of byte 4.
- `climate.py` includes the built-in `esp_driver_rmt` IDF component during codegen; the default normal and pulse-debug compile fixtures now target `framework: type: esp-idf`.
- For Home Assistant hardware testing from a moving branch, use `refresh: 0s` under `external_components` or pin to a commit SHA. Otherwise ESPHome can reuse its cached external component source even when the repo branch changed.
- Current ESPHome version verified in the devcontainer: `2026.4.5`.
- Compile logs confirmed `framework-espidf @ 3.50504.0 (5.5.4)`. The normal and pulse-debug fixtures compile without the legacy RMT deprecation warning.

## Last Verified

Inside the devcontainer:

```sh
./scripts/test-native.sh
python -m unittest discover -s tests -p 'test_*.py'
python -m analysis.hwp_analyze fixtures
python -m analysis.hwp_analyze active-tx --fixture tests/fixtures/active_tx/hwp_active_tx_config5_defrost_2026_05_12.json
python -m analysis.hwp_analyze evidence --menu --limit 25
python -m analysis.hwp_logs_annotator --help
python -m analysis.hwp_web_capture --help
python -m analysis.hwp_sim_orchestrate --help
python -m analysis.hwp_analyze prove --input tmp/hwp/POOL_esphome_logs.log --fixture tests/fixtures/packets/hwp_hardware_log_2025_06_24.json
python -m analysis.hwp_analyze prove-annotations --input tmp/hwp/POOL_esphome_logs.log.2024-11-01 --fixture tests/fixtures/annotations/hwp_annotated_fan_control_2024_11_01.json
./scripts/test-esphome.sh --local
git diff --check
bash -n scripts/test-esphome.sh scripts/test-native.sh
```

The local runner validated and compiled the normal, pulse-debug, web-dashboard, and simulator ESPHome fixtures after Python and native tests. Generated output stayed outside the repo.

## Suggested Next Steps

Choose the next slice from normal project priorities rather than tmp merge work.

Good candidates:

- review `COND_1`/`COND_2` temperature encoding separately if hardware evidence shows values above the short-format range; the issue #11 fix intentionally changed only `CONFIG_1` setpoint encoding in this slice
- mine the remaining non-fan 2024-10-31 condition/clock `test` windows only if they add decode coverage beyond current passive runtime contracts; `COND_D` remains research-only until at least one field meaning is named
- add the next analysis CLI slice for grouped frame diffs/candidate fields now that firmware formatter output has native alignment/highlight contracts
- use the restored GUI annotator during the next field session to create smaller tagged windows for unknown or uncertain menu/status values; convert useful windows into tracked fixtures before changing runtime behavior
- run the next supervised active TX validation for `u02_pulses_per_liter` only after the remaining fixture evidence has been triaged; capture command and echo packets before adding the next active-TX fixture
- add the next low-level native seam for queue behavior
- extract capture conversion as repo-native tooling
- document simulator feasibility using fixture replay versus Arduino/PlatformIO versus ESPHome/QEMU
- run the first two-node simulator HIL session with `hwp_sim_orchestrate transcript`, then decide whether to add more fixture-backed command/echo rules or improve timing/fault playbooks
- probe QEMU or ESPHome `host:` only as feasibility work

QEMU and ESPHome `host:` remain later feasibility slices, not the default runner.

## Active Risks

- Active heater control remains safety-sensitive and needs manual procedures before new real-device claims.
- ESPHome compile success does not prove electrical safety or live heater behavior.
- The ESP-IDF 5 RMT RX/TX path now has initial supervised hardware validation for passive RX and CONFIG_5 defrost active TX. Broader active-control settings still need one-at-a-time supervised validation.
- Debian `qemu-system-xtensa` is available in the devcontainer, but `idf.py`, `IDF_PATH`, and an ESP32 QEMU machine are not present.
- `tmp/hwp` is closed as an active merge source, but its recent logs are valid archival evidence to mine into small tracked fixtures. Future runtime changes still need focused tests and hardware-safety review.

## Working Tree Note

Check `git status --short` before editing. Do not revert existing user or agent changes unless explicitly asked.
