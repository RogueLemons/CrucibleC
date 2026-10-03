[← Previous: Building and testing](building.md)

# Roadmap

WorkshopC is in **alpha**: rules, options and presets may still change between versions. This page lists what is planned.

## Contents

- [Toward beta](#toward-beta)
- [After version 1](#after-version-1)

## Toward beta

- [x] Verify the build on Windows
- [ ] Verify the build on Linux
- [ ] Publish Linux and Windows builds with GitHub Releases
- [ ] Add Language Server Protocol (LSP) support, for diagnostics directly in editors
- [ ] Add contact information to README

## Toward version 1.0.0

- [ ] Test and improve beta based on public feedback

## After version 1

- [ ] Allow references for variables and struct fields, not only for parameters
- [ ] A rule that forbids `int`, `long`, `long long` and `short`
- [ ] Allow `#pragma once` instead of `#ifndef` include guards (under consideration)
- [ ] Optional logging of suppressed lines
- [ ] Let a raii return function be exempt from reverse-order destruction
- [ ] Let a moved raii struct be destroyed out of order, e.g. by allowing the return function in arguments and initializers, by adding a `move_destroy` function, or by treating a move as a destroy
- [ ] Check that a `_private` field is accessed with the right getter or setter, depending on whether it is reached through a const or a mutable pointer
- [ ] Warn about const globals that hold mutable pointers, and possibly about such locals too
- [ ] Add a Python script that installs the dependencies on Windows, Linux and macOS
- [ ] Disallow configs from including settings not part of the project, instead of silently passing them
- [ ] Enforce reassignment of moved struct field pointers
- [ ] Investigate ability/benefits for nevernull config to enforce all arrays end with a sentinel

---

[Back to README](../README.md)
