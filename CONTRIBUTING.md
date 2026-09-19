# Contributing

H2O is gated. Do not open a second knife while one is red.

1. **No new syntax.** Core 1 is frozen. No pragmas, no GHC compatibility.
2. **Tests are the contract.** If behavior changes, update `tests/golden/` first, then the compiler.
3. **Compiler sources stay BH.** `compiler/*.h2o` must not use `|>`, `_` as a hole, or f-strings.
4. **C is the runtime kernel only.** Do not rewrite the compiler in C or Rust.
5. **No package registry.** `h2o vendor` copies a git tree into a directory.

Run `./test` before a PR. CI runs the same script on macOS and Ubuntu.

Need `cc` (Xcode Command Line Tools on macOS). Wasm tests need a WASI SDK and `wasmtime`; without them those items skip — they must not be faked green.
