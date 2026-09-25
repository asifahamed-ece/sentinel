## Description

<!-- What does this PR do? Which issue does it close? -->

Closes #

## Type of change

- [ ] Bug fix
- [ ] New feature / sensor support
- [ ] Documentation update
- [ ] Refactor / style (no behavior change)
- [ ] Other (specify)

## Affected component

- [ ] Worker Node firmware (`node_src`)
- [ ] Admin Hub firmware (`admin_src`)
- [ ] Web dashboard (`docs`)
- [ ] Shared packet protocol (`packet_defs.h`)
- [ ] Documentation / tooling

## Testing performed

<!-- Describe hardware used and steps to verify (flash + upload + run). -->

- [ ] Clean build succeeds
- [ ] Dashboard re-uploaded to LittleFS (if dashboard changed)
- [ ] Tested on hardware (board + sensors):
- [ ] Serial/dashboard output confirms expected behavior

> **Packet protocol change?** If you touched `packet_defs.h`, confirm
> **both** copies (`admin_src/` and `node_src/`) were updated in sync and
> the `static_assert` size checks still pass. A mismatch silently corrupts
> the ESP-NOW link.

## Checklist

- [ ] Followed [CONTRIBUTING.md](https://github.com/asifahamed-ece/sentinel/blob/main/CONTRIBUTING.md)
- [ ] Commit messages follow Conventional Commits
- [ ] My changes are covered by this PR's tests/verification
- [ ] I agree to license my contribution under the [Apache License 2.0](https://github.com/asifahamed-ece/sentinel/blob/main/LICENSE)