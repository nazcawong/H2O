# Status

**English** | [中文](../zh-Hant/status.md)

**V3.1.0.** This is what `./h2o` does, not a manifesto. Where the Core 1 report is bigger than the compiler, the compiler wins as “today”.

H2O is a language for **long-running local CLIs / data transforms**. It is not a GHC replacement, not pip, not cargo.

## The machine

- Entry: POSIX `./h2o` → `bin/h2o`. Missing image: `cc` compiles `compiler/seed.c`. Daily commands do **not** start Python.
- Compiler: `compiler/*.h2o` (a small subset: no `|>` / `_` / f-string in the compiler itself). Weak fixed point: same bytecode array.
- Runtime: tagged `Val` + mark–sweep GC in `compiler/rt.c`.
- `--native`: each `def` becomes a C function, then `cc`. Machine code, still boxed `Val`.
- `--vm`: interpreter image.
- `--target wasm`: the **same** VM cross-compiled to WASI, not Wasm instructions.
- Packages: local directories. `h2o vendor` copies a git tree into `vendor/<name>/`. No registry.
- Checker: HM subset, closed rows, `F[_]` kinds, overlapping impls only error. No effect-row variable `...E`, no overlapping solver, no unbox.

## Versus other languages

| | H2O today | They win on |
|---|---|---|
| **GHC** | Strict default, quiet surface, `Text` not `[Char]`, CLIs run | Types, optimization, libraries, laziness as a first-class story |
| **OCaml** | Pseudocode surface, one spelling | Full HM, real modules, industrial native + GC |
| **Koka** | Effect syntax + `handle` can swap test IO | Effect rows that infer and elaborate |
| **Python** | Immutable default, `{IO}` in the type, `.` is not a method | Libraries. H2O borrowed noise reduction only |
| **Go** | ADTs / effects / `Maybe` in the contract | Tools, native backend, concurrency. `h2o fmt` only strips space |
| **Elixir** | `|>` is the only multi-step spelling | BEAM / OTP. No actors here |
| **Rust** | `once` counts uses | Borrow checker, LLVM, no GC |

Meaning is Haskell. Surface is Python-like. The compiler is written in H2O. That position holds. Calling it an industrial compiler or a Koka-class effect language does not.

## Not in the language

Package registry, semver solver, GHC compatibility, `$`, method chains, pragmas, rewriting the compiler in C/Rust.
