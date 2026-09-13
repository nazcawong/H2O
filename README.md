# H2O

**English** | [中文](README.zh-Hant.md)

**V3.1.0** — A language you can use to write long-running local CLIs. `--native` emits C functions and then `cc` (machine code, still tagged `Val`). `--vm` is the interpreter. `h2o vendor` turns a git tree into a directory. Core 1.1: closed rows, `F[_]` kinds, overlapping instances only error. Not GHC, pip, or cargo. No package registry.

**A successor to Haskell — not another language.**

H2O keeps the few core ideas the 1987–1990 committee actually meant to preserve, and drops thirty years of historical baggage, inconsistent defaults, and engineering friction. Anything new must **reduce** the number of extensions, not grow another pile of pragmas.

> Keep what types can say clearly and what equations can prove.  
> Drop what only works by historical habit, partial functions, default laziness, and extension switches.

## In one sentence

Pure, strict by default, ADTs, type classes, effects in the type.  
`Text` and row records are everyday; laziness, linearity, and dependent types are capabilities, not a tax.  
**Surface:** Python noise reduction (indentation, `and`/`or`, `if cond:`) + Elixir pipelines `|>` + Go’s one way to write a thing. Haskell keeps the meaning, not `$` / juxtaposition / `do`.

## Is this still Haskell?

Yes. The soul did not change:

1. Purity and referential transparency — effects must appear in the type
2. Strong static types + Hindley–Milner-style inference + higher-kinded types
3. Algebraic data types + exhaustive pattern matching
4. Type classes as an abstraction (the hierarchy changes, the idea does not)
5. Immutability by default, shared structure
6. A pure core with an explicit effect boundary
7. Free theorems from parametric polymorphism

Laziness stays **as a capability**. What should change is “lazy by default everywhere”, not a ban on laziness.

Languages that already paid tuition: PureScript, Idris 2, Unison, Roc, Koka, OCaml 5. They are not rivals; they are design notes.

## Layers

Three stable reports instead of “GHC2024 + twenty pragmas”:

| Layer | Contents | How you use it |
|---|---|---|
| **Core** | Pure, strict default, ADTs, pattern matching, HM + HKT, type classes, effect rows, row records, a safe Prelude | Default. Most programs stay here |
| **Typed** | Linear / affine, indexed / dependent subset, totality annotations, visible quantification | Opt-in annotations, not another dialect |
| **Systems** | Native representation, unlifted types, FFI, linear buffers, backend primitives | Runtime and interop |

Going up a layer means adding annotations, not flipping incompatible extensions.

## If you can only do four things

1. **Effects in the core** — kill the most common architecture pain
2. **Row records / open sums** — kill the most common everyday pain
3. **Strict default + safe text / Prelude** — kill the most common runtime pain
4. **Linear resources + practical dependent types** — make “illegal states unrepresentable” a normal tool

## Documents

Most of `docs/` is still Traditional Chinese. The two READMEs are the bilingual entry.

| Document | Contents |
|---|---|
| [docs/00-principles.md](docs/00-principles.md) | Trade-offs and success criteria |
| [docs/01-keep.md](docs/01-keep.md) | What must stay of the Haskell soul |
| [docs/02-discard.md](docs/02-discard.md) | Defaults to drop or flip |
| [docs/03-redesign.md](docs/03-redesign.md) | Worth keeping, harmful if copied as-is |
| [docs/04-must-add.md](docs/04-must-add.md) | Must be in the core; cannot be faked with extensions |
| [docs/05-engineering.md](docs/05-engineering.md) | Diagnostics, modules, metaprogramming, compilation, stdlib |
| [docs/06-optional.md](docs/06-optional.md) | Optional layer: do not repeat extension explosion |
| [docs/07-do-not-add.md](docs/07-do-not-add.md) | Surface features not to import from other languages |
| [docs/08-layers.md](docs/08-layers.md) | Core / Typed / Systems report boundaries |
| [ROADMAP.md](ROADMAP.md) | Zero to V1.0 (historical; done) |
| [docs/compare.md](docs/compare.md) | **Status:** vs GHC / OCaml / Koka / Python / Go / Elixir / Rust |
| [docs/13-next.md](docs/13-next.md) | After V1.0 (V1.0.1, bootstrap, proto removal: done) |
| [docs/17-v2.md](docs/17-v2.md) | **To V2.0.0:** a language you can develop with (D0–D4, done) |
| [docs/18-v2x.md](docs/18-v2x.md) | **To V2.5:** Typed 1 then Systems 1 (E0–E4, done) |
| [docs/19-v3.md](docs/19-v3.md) | **V3.0.0:** `--native` emits machine code; packages stay directories |
| [docs/20-core11.md](docs/20-core11.md) | **V3.1.0:** Core 1.1 row inference / HKT kinds / overlapping only errors |
| [docs/15-hm.md](docs/15-hm.md) | T0–T4: HM subset (done) |
| [docs/16-repl.md](docs/16-repl.md) | REPL: `?` tutorial (`docs/learn`); `;` shell; `]` pkg |
| [docs/14-bootstrap.md](docs/14-bootstrap.md) | Bootstrap gates (historical B0–B5; see the top of that file for status) |
| [docs/09-roadmap.md](docs/09-roadmap.md) | Priorities and stages (table) |
| [docs/core/CORE-0.md](docs/core/CORE-0.md) | v0.1 Core-0 spec (implementation is no longer in proto) |
| [docs/10-syntax.md](docs/10-syntax.md) | **Concrete syntax:** indentation, parentheses, grammar |
| [docs/11-readability.md](docs/11-readability.md) | Readability: one thing each from Python / Elixir / Go / Haskell |
| [docs/syntax-cheatsheet.md](docs/syntax-cheatsheet.md) | One-page cheat sheet |
| [notes/related-languages.md](notes/related-languages.md) | Design notes from languages that already paid tuition |
| [examples/core0_hello.h2o](examples/core0_hello.h2o) | v0.1 single-file acceptance |
| [examples/main.h2o](examples/main.h2o) | v0.2 multi-file acceptance |
| [examples/traits.h2o](examples/traits.h2o) | v0.3 trait / impl / HKT |
| [examples/wc/](examples/wc/) | V2.0.0 golden tool: multi-file CLI (`pub` + directory package) |
| [examples/count/](examples/count/) | **V2.5.0 golden tool:** `once` for files + `Vect` for fixed length |

## Tree

```
H2O/
├── README.md           English
├── README.zh-Hant.md   Traditional Chinese
├── ROADMAP.md          zero to V1.0 (historical)
├── h2o                 POSIX wrapper: exec bin/h2o
├── test                POSIX test harness (no Python)
├── compiler/           H2O compiler + C VM kernel + C seed
├── prelude/
├── tests/
├── docs/
├── examples/
└── notes/
```

There is no `proto/`. The daily entry is `bin/h2o` (C bytecode VM, mark–sweep GC). Status: [docs/compare.md](docs/compare.md). Contract: [docs/core/REPORT.md](docs/core/REPORT.md). “Can develop” path (done): [docs/17-v2.md](docs/17-v2.md).

## 30 minutes

```
./h2o run examples/core0_hello.h2o
./h2o run examples/count examples/count/sample.txt    # prints count 3; once + Vect
./h2o build --native examples/count -o /tmp/count     # C functions, then cc; machine code
/tmp/count run examples/count examples/count/sample.txt
./h2o build --target wasm examples/count -o /tmp/count.wasm
wasmtime --dir=. /tmp/count.wasm run examples/count examples/count/sample.txt
# Packages are directories. No registry.
# ./h2o vendor <git-url> [name]   → vendor/<name>/
./h2o repl          # ? 0 table of contents; ? 11 once / Vect / Wasm
./test
```

`--native` turns each `def` into a C function and then `cc` (machine code, still tagged `Val` + GC). `--vm` is the interpreter image. `--target wasm` is still the same VM cross-compiled to WASI. `]` has no package registry. `h2o vendor <git-url> [name]` copies a git tree into `vendor/<name>/`; after that `h2o run vendor/name` and `:load vendor/name/` behave like a local directory.

If `bin/h2o` is missing, `./h2o` compiles `compiler/seed.c` with `cc` (a portable C snapshot; no Python). After changing `compiler/*.h2o`, refresh the snapshot with:

```
./h2o build --vm --no-check compiler/h2oc.h2o -o /tmp/h2o-seed
cp /tmp/h2o-seed.c compiler/seed.c
```
