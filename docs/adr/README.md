# Architecture Decision Records

Each ADR captures one significant decision: its context, the choice, and the
consequences. They use a light template — Status / Context / Decision /
Consequences / Alternatives. Numbered in reading order, not strict chronology.

| # | Title | Status |
| - | ----- | ------ |
| [0001](0001-display-and-rendering-architecture.md) | Display & rendering architecture | Accepted |
| [0002](0002-concurrency-model.md) | Concurrency & core allocation | Accepted |
| [0003](0003-audio-subsystem.md) | Audio subsystem: raw I²S + manual ES8311 | Accepted |
| [0004](0004-storage-and-metadata-model.md) | Storage & metadata model | Accepted |
| [0005](0005-time-model.md) | Time model: RTC, UTC storage, configurable local | Accepted |
| [0006](0006-power-and-sleep-model.md) | Power & sleep model | Accepted (depth provisional) |
| [0007](0007-connectivity-and-sync-seam.md) | Connectivity & sync seam (BLE-first) | Accepted |
| [0008](0008-edge-and-error-state-ui.md) | Edge & error-state UI | Accepted |
| [0009](0009-button-input-handling.md) | Button input handling via OneButton | Accepted |

Requirements these decisions serve live in `../requirements/`.
