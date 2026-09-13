# 現在的 H2O 和其他語言對比

短文（給外人）：[English](en/status.md) · [中文](zh-Hant/status.md)。本文是長版檔案。

日期：2026-09-13。**V3.1.0。** 這是**實作現況**，不是設計宣言。設計取捨見 [`notes/related-languages.md`](../notes/related-languages.md) 與 [`00-principles.md`](00-principles.md)。Core 1 合約見 [`core/REPORT.md`](core/REPORT.md)。

**EN:** Implementation status, not a design manifesto. Design notes: [`notes/related-languages.md`](../notes/related-languages.md), [`00-principles.md`](00-principles.md). Core 1 contract: [`core/REPORT.md`](core/REPORT.md).

今天的定位：能寫**長期跑的本地 CLI／資料轉換**（多檔、`pub`、目錄套件、GC）。`once` 是移動切片，不是借用檢查。`Vect[n, a]` 的 n 是 Peano（`Z`／`S[_]`）。`--native` 發 C 函數再 `cc`（機器碼，**仍是 tagged `Val` + GC**，不是 unbox）。`--vm` 是解釋器。`--target wasm` 是同一套 VM 交叉編譯。Core 1.1：關閉列、`F[_]` kind、overlapping 只報錯。不是 GHC 替代品，不是 pip，不是 cargo。

**EN:** Position today: long-running local CLI / data transforms (multi-file, `pub`, directory packages, GC). `once` is a move slice, not borrow checking. `Vect[n, a]` uses Peano `Z` / `S[_]`. `--native` emits C functions then `cc` (machine code, still tagged `Val` + GC, not unboxed). `--vm` is the interpreter. `--target wasm` is the same VM cross-compiled. Core 1.1: closed rows, `F[_]` kinds, overlapping instances only error. Not a GHC replacement, not pip, not cargo.

對照軸只有兩個：**語言形狀（合約寫了什麼）**，以及 **今天 `./h2o` 實際做了什麼**。兩者不一致的地方，以實作為準。

---

## 今天這台機器是什麼

- **入口：** POSIX `./h2o` → `bin/h2o`。沒有映像時用 `cc` 編 `compiler/seed.c`。日常 `run`／`check`／`fmt`／`tokens`／`diagnostics`／`build` **不啟動 Python**。
- **編譯器：** `compiler/*.h2o`（BH 子集：沒有 `|>`／`_`／f-string），自己編自己。映像再編映像，產生的 `.c` 陣列位元組相同（弱固定點；Mach-O 時間戳會不同）。
- **執行期：** `compiler/rt.c`。帶標籤的 `Val`、堆疊 bytecode VM、同一套 ISA。`--native` 把每個 `def` 發成 C 函數再 `cc`（機器碼，仍是 `Val`+GC）。`--vm` 是解釋器映像。`--target wasm` 把同一套 VM 用 WASI SDK 編成模組。不是 LLVM。
- **可選除錯通道：** `h2o build -o *.py` 仍發獨立 Python（`compiler/rt.py`）。這不是原型編譯器。`proto/` **已刪**。
- **檢查器：** Hindley–Milner 子集。`map`／`filter`／`foldl`／`join` 等內建有真正的類型方案（`map(1, 2)` 是類型錯誤）。`A: Eq` 與 `Trait.method` 會查 impl；`Functor.map` 依第一參數的型構選 `_Functor_List_map`／`_Functor_Maybe_map`。`Vect[n, a]` 的 n 是 `Z`／`S[_]`（`vect` 只要列表字面；空 `vect_head` 是類型錯誤）。關閉紀錄缺／多欄位是類型錯誤；`{name: Text, ...R}` 接受多餘欄位。`impl Functor[Int]`／`List[Int]` 是 kind 錯誤。同一型兩份 impl 是錯誤（不求解）。**沒有效應列變數 `...E`、沒有 overlapping solver。**
- **REPL：** `./h2o` 或 `./h2o repl`。`h2o>` 提示、縮排續行、`:t`／`:load`／`:quit`。`:load path/` 載入 `path/mod.h2o`。`]` **沒有**套件註冊中心。錯誤不殺掉行程。沒有 `ans` 活綁定、沒有 tab 補全。

有最小標記－清除 GC（List／紀錄／建構子／文字／閉包）。沒有 STG、沒有壓縮／分代，沒有最佳化管道。

---

## 總表

| | H2O **合約** | H2O **今天** | 對照語言今天的實況 |
|---|---|---|---|
| 求值 | 嚴格預設；惰性是 `Stream` | 嚴格；`Stream.*` 是 C builtin | GHC 預設惰性；OCaml／Koka／Go／Rust 嚴格 |
| 純度 | 效果出現在類型裡 | 執行期有 handler；檢查只看函數有沒有宣告 `{IO}`／`State`／`Except` | GHC `IO`；Koka／OCaml 5 有效應；Python／Go／Elixir 效果不在類型裡 |
| 類型 | HM + HKT + trait | HM 子集；關閉列／`F[_]` kind／overlapping 只報錯 | GHC／OCaml／Koka／Rust 都有完整推導或檢查 |
| 表面 | Python 縮排 + `|>` + 一種寫法 | 已實作：縮排、`and`/`or`/`not`、`|>`、`.` 不是方法 | Python 縮排；Elixir `|>`；Go `gofmt`；Haskell `$`／並置 |
| 效應 | 列 + `handle` | 動態 handler 能跑 `examples/effects.h2o`；列多型沒推導 | Koka 是這件事的完整實作；OCaml 5 有效應處理器 |
| 並發 | `{Conc}` 結構化取消 | `conc.together` 兩個任務（C `pthread`） | Go goroutine；GHC 輕量執行緒；BEAM actor；OCaml 5 domain |
| 編譯器宿主 | （曾評測發 Python／Rust／OCaml） | **H2O 寫前端，C 只當 VM 核** | GHC／OCaml／Rust／Go 都自舉；CPython 是 C |
| 後端 | `h2o build` 出原生可執行檔 | `--native` 發 C 函數再 `cc`（機器碼，仍 tagged `Val`）；`--vm` 是解釋器映像 | GHC NCG/LLVM；OCaml 原生+bytecode；Rust LLVM；Go 自家後端 |
| 自舉 | 不是 V1.0 閘門 | **已過弱固定點**（C 陣列相同） | GHC／OCaml／Go／Rust 工業級自舉 |
| 種子 | — | `compiler/seed.c` + `cc` | 各語言有自己的 bootstrap 故事 |
| GC | 未寫進 Core 1 | 標記－清除（最小） | 對照語言都有（Rust 用所有權代替） |
| 相容 | 永不 GHC | 沒有 | GHC Haskell 相容是 GHC 的工作 |

---

## GHC Haskell

Haskell 的後繼指的是**含義**：純、ADT、類型類、參數多型。表面與預設全部反過來。

| | GHC | H2O 今天 |
|---|---|---|
| 預設求值 | 惰性 | 嚴格 |
| 文字 | `String = [Char]` 仍在 Prelude | `Text` 是日常；沒有把字元串當文字 |
| 擴充 | GHC2024 + pragma | Core 1 凍結，不開開關 |
| 呼叫 | 並置、`$`、自訂中綴 | `f(x, y)`，一種寫法 |
| 效應 | `IO` + 函式庫（MTL／transformers／effectful） | 語法有效應列；檢查是標籤，不是列推導 |
| 編譯器 | Haskell 寫，STG → Cmm → NCG/LLVM | H2O 寫，直接發 VM 位元組 |
| 類型 | 工業級 HM、類型族、Kind | HM 子集，無類型族 |
| 優化 | 需求嚴格、worker/wrapper、rewrite rules | 沒有 |

**現在比得過 GHC 的：** 日常 CLI 能跑、編譯器自舉在弱意義上成立、表面比 Haskell 安靜、嚴格預設讓空間行為可猜。

**現在比不過的：** 類型、優化、生態、惰性作為一等能力（`Stream` 只是 builtin）、診斷深度、函式庫。H2O 不是 GHC 替代品，合約自己也這麼寫。

讀程式時：GHC 先看到符號與型別；H2O 先看到縮排與 `|>`。這是刻意的。類型是否說得清楚，今天 GHC 贏。

---

## OCaml

OCaml 是「嚴格、工業、能自舉、有效應（5）」這條路上已經做成的語言。H2O 的執行模型比較接近 OCaml 而不是 GHC：嚴格、ADT、`match`、bytecode + 可選原生。

| | OCaml 5 | H2O 今天 |
|---|---|---|
| 表面 | 有括號／`;;`／兩套語法歷史 | Python 縮排，一種寫法 |
| 模組 | 真正的模組／函子 | `pub` 是邊界；套件＝本地目錄；仍不是函子 |
| 類型 | 完整 HM，推論強 | 子集檢查 |
| 效應 | 一等效應處理器 | 動態 handler + 標籤檢查 |
| 後端 | 原生 + bytecode，工業級 | `--native` 是 C 函數機器碼（仍 `Val`）；`--vm` 是 bytecode |
| GC / 執行期 | 分代 GC、領域 | 標記－清除（最小） |

H2O 若要往「能寫系統工具」靠，缺的是 OCaml 已經有的那些：真正的類型、原生後端。GC 與模組邊界在 V2.0.0 已有最小切片。表面 H2O 比較像偽代碼；表達力與可信度今天是 OCaml 贏。

宿主評測（[`host-eval.md`](host-eval.md)）曾經把 OCaml 當 stage-1 候選。實際沒走那條：編譯器直接用 H2O 寫，C 只留 VM。這讓自舉路徑短，但把 OCaml 會順便帶來的類型與後端品質都推遲了。

---

## Koka

Koka 是效應列的主要參考。Core 1 的 `{IO, State[S], Except[E]}` 與 `handle` 就是衝著它去的。

| | Koka | H2O 今天 |
|---|---|---|
| 效應列 | 推導、列多型、處理器是語言核心 | 語法在；檢查只問「有沒有寫這個標籤」 |
| 推論 | 效應與值類型一起推 | 沒有推導 |
| `handle` | 有語義、有 elaboration | 執行期 builtin（`OP_PUSH_H`／`OP_THROW`） |
| 表面 | 自己的語法，偏函式 | Python 縮排 + `|>` |

`examples/effects.h2o` 能跑：同一支 `count_line` 接假 IO 得 `Result`。這證明**動態**故事通了。類型端還沒做到「列是型別的一部分、會推、會報缺了哪一列」。跟 Koka 比，H2O 現在是「效應有語法的嚴格腳本」，不是「效應語言」。

---

## Python

表面大量重疊，含義相反。

| | Python | H2O 今天 |
|---|---|---|
| 縮排、`and`/`or`/`not`、`if cond:` | 是 | 是（4 空格；tab 是錯） |
| 可變、隱式 IO | 是 | 預設不可變；`println` 要 `{IO}`（`main` 隱含） |
| `.` 方法鏈 | 是 | 非 `NVar` 接收者會死「`.` 不是方法，請用 `|>`」 |
| 執行 | CPython 位元組碼 + GC | C tagged VM，標記－清除 GC |
| 類型 | 可選、不擋執行 | `h2o check` 是 HM 子集 |
| 編譯器 | C | H2O + 幾十 KB 的 C 核 |

`proto/` 曾經是 Python 樹走訪。那條路關了。剩下的 Python 只有 `h2o build -o out.py` 這條除錯發碼。日常入口的 Mach-O **不**連 libpython。

H2O 看起來像 Python，是為了降語法噪聲，不是為了當 Python。可變、猴子補丁、隱式 IO、方法生態，全部不搬。

---

## Go

取「一件事一種寫法」和官方格式。不取「把類型表達力砍到能養編譯器」。

| | Go | H2O 今天 |
|---|---|---|
| 格式 | `gofmt`，不討論 | `h2o fmt` 只清空白，不是 AST 重印 |
| 一種寫法 | 語言文化 | Core 規則（沒有第二套呼叫） |
| 編譯器 | Go 寫，出真機器碼 | H2O 寫；`--native` 發 C 函數再 `cc`；`--vm` 出映像 |
| 並發 | goroutine + channel | 兩個任務的 `together` |
| 類型 | 標註為主，推論有限，現在有泛型 | 合約比 Go 大；實作比 Go 小 |
| GC | 有 | 標記－清除（最小） |

`h2o fmt` 今天還不是 `gofmt`：它不做版面決策，只 rstrip。Go 贏在工具完成度與原生後端。H2O 贏在（合約上的）ADT／效應／`Maybe` 索引；實作上這些還沒變成 Go 那種「編譯器真的擋」。

---

## Elixir

取 `|>` 當多步資料的唯一寫法。不取可選括號、宏、actor 預設。

| | Elixir | H2O 今天 |
|---|---|---|
| `|>` | 是，可跟巨集一起長 | 是，脫糖成普通呼叫 |
| 執行 | BEAM，不可變 + 訊息 | 嚴格 VM，無 actor |
| 類型 | 漸進（Typespec / 漸強的集合檢查） | 靜態意圖 + 子集檢查 |
| 宏 | 語言文化 | Core 1 不做 |

管線長得像 Elixir；程式含義不是 Elixir。沒有 OTP、沒有熱升級、沒有 mailbox。

---

## Rust

Typed 層的 `once` 是衝著資源生命週期去的，不是衝著 Rust 語法。

| | Rust | H2O 今天 |
|---|---|---|
| 資源 | 借用檢查器，編譯期 | `once` 移動（別名／欄位算用掉）；`with` 內仿射，離開即關。不是借用檢查 |
| 後端 | LLVM，真機器碼 | `--native` 是 C 函數機器碼（仍 `Val`）；不是 LLVM |
| 記憶體 | 所有權，無 GC | 標記－清除；`once` 是移動切片，不是借用檢查 |
| 效應 | 沒有一等效應列；`Result` + 顯式 | 效應列在合約裡；檢查是標籤 |
| 表面 | 系統語言 | 偽代碼外表 |

`examples/linear.h2o` 能跑「opened」。這不是借用檢查。跟 Rust 比，H2O 今天沒有資格談系統層記憶體安全；`once` 是靜態計次，漏了別名、漏了結構欄位、漏了 IR。

FFI：`prim.ffi_call("c", "abs", -7)` 能印 `7`。檢查器有符號表（錯型／未知名是 `h2o check` 紅）。ABI 草案：[systems/ABI.md](systems/ABI.md)。不是穩定 ABI。

---

## 編譯器這一行怎麼放

| | 編譯器用什麼寫 | 日常執行 | 自舉 | 種子 |
|---|---|---|---|---|
| **H2O** | H2O（BH） | `--vm` bytecode；`--native` C 函數 | 弱固定點（`.c` 陣列相同） | `seed.c` + `cc` |
| GHC | Haskell | RTS + 原生 | 強、工業 | 有階段 |
| OCaml | OCaml | 原生或 bytecode | 強、工業 | `boot/` |
| Go | Go | 原生 | 強、工業 | 有 |
| Rust | Rust | LLVM 原生 | 強、工業 | stage0 |
| CPython | C | 位元組碼 + GC | 不自舉 | tarball |
| Elixir | Elixir / Erlang | BEAM | 在 BEAM 上 | OTP |
| Koka | 自身／歷史上 Haskell 宿主 | C 後端 | 有效應的完整編譯器 | — |

H2O 的特別之處不是「比誰都快」或「類型比誰都強」，而是：**語言含義走 Haskell，表面走 Python，編譯器已經用自己寫、跑在有 GC 的 VM 上，日常不再經過 Python，能養本地多檔工具。** 這個位置成立。把它說成工業編譯器或 Koka 級效應系統，不成立。

---

## 不要從這張表讀出的結論

1. **不是 GHC 替代品。** 沒有惰性生態、沒有 Cabal／Hackage。HM 是子集，不是工業編譯器。
2. **`--native` 發 C 函數再 `cc`，是機器碼，不是 LLVM。** 每個 H2O `def` 一支 C 函數（仍 tagged `Val` + GC）。`--vm` 是解釋器。
3. **`--target wasm` 不是把 H2O 譯成 Wasm 指令。** 同一套 VM 交叉編成 WASI；要 WASI SDK。
4. **`]` 不是套件庫。** 本地目錄；`h2o vendor` 把 git 樹變成 `vendor/<name>/`。沒有 index、沒有 semver 解算。
5. **`proto/` 不存在。** 文件裡若還寫 Python 樹走訪，那是舊句，以本文與 `compiler/` 為準。
6. **Python 發碼還在。** `compiler/rt.py` 是除錯通道；刪 `proto/` 沒有刪這條路。
7. **Core 1.1 已補列／kind／overlapping 報錯。** 效應列變數 `...E`、Koka 級 solver、完整 kind 推滿還沒做。V2.0.0 兌現的是工程：GC、模組邊界、能跑的 CLI。

驗收入口：

```
./h2o run examples/core0_hello.h2o
./h2o run examples/count examples/count/sample.txt
./h2o build --native examples/count -o /tmp/count
/tmp/count run examples/count examples/count/sample.txt
./test
```
