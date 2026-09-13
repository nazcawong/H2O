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

## Documents

| | English | 中文 |
|---|---|---|
| Index | [docs/en/index.md](docs/en/index.md) | [docs/zh-Hant/index.md](docs/zh-Hant/index.md) |
| Language | [docs/en/language.md](docs/en/language.md) | [docs/zh-Hant/language.md](docs/zh-Hant/language.md) |
| Status | [docs/en/status.md](docs/en/status.md) | [docs/zh-Hant/status.md](docs/zh-Hant/status.md) |

REPL lessons (`? 0`–`? 11`) are English: [docs/learn](docs/learn). Examples: [core0_hello](examples/core0_hello.h2o), [count](examples/count/), [wc](examples/wc/).

Design notes and version gates (Chinese archive): [docs/00-principles.md](docs/00-principles.md) … [docs/20-core11.md](docs/20-core11.md).

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
./h2o repl          # ? 0 table of contents (English); ? 11 once / Vect / Wasm
./test
```

`--native` turns each `def` into a C function and then `cc` (machine code, still tagged `Val` + GC). `--vm` is the interpreter image. `--target wasm` is still the same VM cross-compiled to WASI. `]` has no package registry. `h2o vendor <git-url> [name]` copies a git tree into `vendor/<name>/`; after that `h2o run vendor/name` and `:load vendor/name/` behave like a local directory.

If `bin/h2o` is missing, `./h2o` compiles `compiler/seed.c` with `cc` (a portable C snapshot; no Python). After changing `compiler/*.h2o`, refresh the snapshot with:

```
./h2o build --vm --no-check compiler/h2oc.h2o -o /tmp/h2o-seed
cp /tmp/h2o-seed.c compiler/seed.c
```
