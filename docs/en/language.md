# Language

**English** | [中文](../zh-Hant/language.md)

Core 1 is frozen. No pragmas, no second syntax. Full contract: [core/REPORT.md](../core/REPORT.md) (Chinese).

## Surface

4-space indent. `and` / `or` / `not`. No `;`, no block `{}`. Calls are `f(x, y)`. Multi-step data uses only `|>`. `.` is field or module, not a method.

```
def add(x: Int, y: Int) -> Int:
    x + y

users
    |> filter(_.active)
    |> map(_.name)
    |> join(", ")

if user_is_active and data_is_ready:
    start()

match xs:
    case []: 0
    case [n, *rest]: n + sum(rest)

p = {name: "Ada", age: 36}
p.name
{p..., age: 37}
```

| Do | Write | Do not write |
|---|---|---|
| Condition | `if a and b:` | `if (a && b) {` |
| Call | `f(x)` | `f x` |
| Field | `p.name` | `xs.map(f)` |
| Several steps | `xs \|> f() \|> g()` | `$`, method chains |
| One list step | `[e for x in xs if p]` | mix with `|>` as style |

## Layers

| Layer | Use |
|---|---|
| **Core** | Default. Pure, strict, ADTs, HM + HKT, traits, effect rows, row records |
| **Typed** | Opt-in: `once`, `total def`, `Vect[n, a]` |
| **Systems** | Runtime: `--native`, WASI, FFI |

Going up a layer adds annotations, not incompatible extensions.

## Core 1 (what the checker enforces)

- Pure. Effects in the type: `-> {IO, State[S], Except[E]} A`. Undeclared effects are errors. Unannotated `main` implies `{IO}`.
- Strict by default. Lazy structure is `Stream`, not `List`.
- Exhaustive `match`. `Text` is not `[Char]`. `xs[i]` is `Maybe`.
- Closed record `{name: Text}` cannot gain or drop fields. `{name: Text, ...R}` may have extras.
- `trait Functor[F[_]]`: `Int` is not `F[_]`. Two impls for one type error; no overlapping solver.
- `once T`: use exactly once. Not a borrow checker.

Never: GHC compatibility, `$`, juxtaposition, method chains, custom infix, a package registry, pragma lists.

## Commands

`h2o run` · `h2o check` · `h2o fmt` (strip trailing space, not a pretty-printer) · `h2o build --native` · `h2o build --vm` · `h2o build --target wasm` · `h2o vendor` · `h2o repl`

REPL: `?` lessons, `;` shell, `]` packages (directories only). Lessons are English (`docs/learn`).
