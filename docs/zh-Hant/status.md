# 現況

[English](../en/status.md) | **中文**

**V3.1.0。** 這是今天 `./h2o` 實際做的，不是設計宣言。Core 1 報告比編譯器大的地方，以實作為「今天」。

H2O 能寫**長期跑的本地 CLI／資料轉換**。不是 GHC 替代品，不是 pip，不是 cargo。

## 這台機器

- 入口：POSIX `./h2o` → `bin/h2o`。沒有映像時用 `cc` 編 `compiler/seed.c`。日常命令**不**啟動 Python。
- 編譯器：`compiler/*.h2o`（編譯器自己用的子集：沒有 `|>`／`_`／f-string）。弱固定點：bytecode 陣列相同。
- 執行期：`compiler/rt.c` 裡 tagged `Val` + 標記－清除 GC。
- `--native`：boxed `Val` + GC 上的 C 函數後端，再 `cc`。不是 unbox，不是 LLVM。
- `--vm`：解釋器映像。
- `--target wasm`：**同一套 VM** 交叉編成 WASI，不是 Wasm 指令。
- 套件：本地目錄。`h2o vendor` 把 git 樹拷進 `vendor/<name>/`。沒有 registry。
- 檢查器：HM 子集、關閉列、`F[_]` kind、overlapping 只報錯。沒有效應列變數 `...E`、沒有 overlapping solver、沒有 unbox。

## 和其他語言

| | H2O 今天 | 對方贏在 |
|---|---|---|
| **GHC** | 嚴格預設、表面安靜、`Text` 不是 `[Char]`、CLI 能跑 | 類型、優化、函式庫、惰性作為一等能力 |
| **OCaml** | 偽代碼外表、一種寫法 | 完整 HM、真正模組、工業原生＋GC |
| **Koka** | 效應有語法、`handle` 能換測試 IO | 效應列會推、會 elaboration |
| **Python** | 不可變預設、`{IO}` 在類型裡、`.` 不是方法 | 函式庫。H2O 只借了降噪 |
| **Go** | 合約上有 ADT／效應／`Maybe` | 工具、原生後端、並發。`h2o fmt` 只清空白 |
| **Elixir** | `|>` 是唯一多步寫法 | BEAM／OTP。這裡沒有 actor |
| **Rust** | `once` 計次 | 借用檢查、LLVM、無 GC |

含義走 Haskell，表面走 Python，編譯器用自己寫。這個位置成立。說成工業編譯器或 Koka 級效應語言，不成立。

## 語言裡不做

套件註冊中心、semver 解算、GHC 相容、`$`、方法鏈、pragma、把編譯器改用 C／Rust 寫。
