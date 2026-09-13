# B0 宿主評測（自舉路徑）

日期：2026-09-07。結論寫死，B1 按此做。

**現況（2026-09-12）：** 沒有走「stage 1 發獨立 Python、再自舉」這條。編譯器是 `compiler/*.h2o`，日常後端是 `compiler/rt.c` bytecode VM；種子是 `compiler/seed.c`。`proto/` 已刪。`h2o build -o *.py` 仍是可選除錯發碼（`compiler/rt.py`），不是原型。下文保留為歷史評測。

## 選項

| | Python 發碼 | Rust 原生 | OCaml 原生 |
|---|---|---|---|
| 錯誤訊息 | 完全自管 | 完全自管 | 完全自管 |
| 最短後端 | 發 `.py` 文字 | LLVM/cranelift，長 | 發 C 或 bytecode，中 |
| H2O 能否自己發 | 能（`io.write_file`） | 不能發機器碼，除非再包 LLVM | 發 C 可以，VM 也可以 |
| 自舉 B4 | **走得通** | B4 卡死 | 走得通（發 C） |
| 養 | 已有 stage 0 | 重寫全部 | 重寫全部 |

## 結論

**自舉後端 = 發出獨立 Python 原始碼**（早期語言發 C 的同一招）。

- stage 1：（歷史）當時的 Python 原型把 H2O AST 譯成 **不 import 原型** 的 `.py`
- stage 2：`compiler/h2oc.h2o`（H2O 寫的編譯器）由 stage 1 譯出
- stage 3：stage 2 再譯 `h2oc.h2o`
- 固定點：stage 2 與 stage 3 對同一批測資行為相同

Rust／OCaml 原生後端放在**自舉成功之後**當 retarget，不當自舉前提。

## 黃金測試

B1 起 `h2o build` 的產物必須通過 `examples/core0_hello.h2o` 與 `tests/golden/`（由 stage 0 類型檢查；發碼路徑至少跑通 examples）。
