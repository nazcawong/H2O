# H2O

**English** | [中文](README.zh-Hant.md)

Not the HTTP server, not [H2O.ai](https://h2o.ai). This is a **Haskell successor**: meaning from Haskell, surface from Python / Elixir / Go. License: [MIT](LICENSE).

**V3.1.x** is the compiler. **Core 1** is the frozen language contract. They are not the same number.

`--native` is a **C-function backend over boxed `Val` + GC**, then `cc`. `--vm` is the bytecode interpreter. `--target wasm` is that same VM cross-compiled to WASI. `h2o vendor` copies a git tree into a directory. No GHC, pip, cargo, or package registry.

**A successor to Haskell — not another language.**

| Can | Cannot |
|---|---|
| Long-running local CLI / data transforms | Replace GHC, OCaml, or Koka |
| `h2o check`: HM subset, closed rows, `F[_]` kinds | Infer effect-row variables `...E` |
| `{IO}` as a **declared** tag (undeclared `println` is an error) | Infer which effect row is missing from the body |
| `--native` C functions (boxed `Val`); `native-fast` must beat `h2o run` | Unboxed native / LLVM |
| `h2o fmt` strips trailing space | `gofmt`-style AST reprint (next knife) |
| Directory packages + `h2o vendor` | A registry, semver solver, lockfile graph |
| `once` counts uses; `Vect[n,a]` is Peano | Rust borrow checking, Idris dependents |

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

`--native` turns each `def` into a C function and then `cc` (boxed `Val` + GC, not unbox). `--vm` is the interpreter image. `--target wasm` is still the same VM cross-compiled to WASI. `]` has no package registry. `h2o vendor <git-url> [name]` copies a git tree into `vendor/<name>/`; after that `h2o run vendor/name` and `:load vendor/name/` behave like a local directory.

`h2o fmt` only strips trailing whitespace. `h2o build -o *.py` / `compiler/rt.py` is an **unsupported debug** channel, not a second semantics.

Need `cc` (Xcode Command Line Tools on macOS). Wasm needs a [WASI SDK](compiler/wasi-sdk.sh) and `wasmtime`; without them those tests skip.

If `bin/h2o` is missing, `./h2o` compiles `compiler/seed.c` with `cc` (a portable C snapshot; no Python). After changing `compiler/*.h2o`, refresh the snapshot with:

```
./h2o build --vm --no-check compiler/h2oc.h2o -o /tmp/h2o-seed
cp /tmp/h2o-seed.c compiler/seed.c
```
