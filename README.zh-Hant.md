# H2O

[English](README.md) | **中文**

不是 HTTP 伺服器 H2O，也不是 [H2O.ai](https://h2o.ai)。這是 **Haskell 的後繼**：含義走 Haskell，表面走 Python／Elixir／Go。授權：[MIT](LICENSE)。

**V3.1.x** 是編譯器版本。**Core 1** 是凍結的語言合約。兩個數字不是同一件事。

`--native` 是 **boxed `Val` + GC 上的 C 函數後端**，再 `cc`。`--vm` 是 bytecode 解釋器。`--target wasm` 是同一套 VM 交叉編成 WASI。`h2o vendor` 把 git 樹變成目錄。不是 GHC／pip／cargo。沒有套件註冊中心。

**Haskell 的後繼，不是另一門語言。**

| 能 | 不能 |
|---|---|
| 長期跑的本地 CLI／資料轉換 | 取代 GHC、OCaml、Koka |
| `h2o check`：HM 子集、關閉列、`F[_]` kind | 推效應列變數 `...E` |
| `{IO}` 當**已宣告**標籤（沒宣告的 `println` 會紅） | 從身體推出缺了哪一列 |
| `--native` C 函數（boxed `Val`）；`native-fast` 必須快過 `h2o run` | unbox／LLVM |
| `h2o fmt` 清行尾空白 | `gofmt` 級 AST 重印（下一刀） |
| 目錄套件 + `h2o vendor` | registry、semver 解算、lockfile 圖 |
| `once` 計次；`Vect[n,a]` 是 Peano | Rust 借用檢查、Idris 依賴類型 |

**Haskell 的後繼，不是另一門語言。**

H2O 留下 1987–1990 年委員會真正想保住的少數核心思想，清掉三十多年累積的歷史包袱、不一致預設與工程摩擦。新增的能力必須**減少**擴充數量，而不是再製造一批 pragma。

> 留下「能用類型說清楚、能用等式推理」的一切；  
> 丟掉「靠歷史慣例、部分函數、預設惰性與擴充開關才能寫日常程式」的一切。

## 一句話

純、嚴格為預設、ADT、類型類、效應出現在類型裡。  
`Text` 與列紀錄是日常；惰性、線性、依賴類型是能力，不是稅。  
**表面**：Python 降噪（縮排、`and`/`or`、`if cond:`）＋ Elixir 管線 `|>` ＋ Go 一件事一種寫法。Haskell 只留下含義，不留下 `$`／並置／`do`。

## 這還是 Haskell 嗎？

是。靈魂沒有換：

1. 純度與參照透明——效果必須出現在類型裡
2. 強靜態類型 + Hindley–Milner 風格推導 + 高階種類
3. 代數資料型別 + 窮盡模式比對
4. 類型類這個抽象（改階層，不改概念）
5. 不可變為預設、共享結構
6. 純核心與效果邊界分開
7. 參數多型帶來的自由定理

惰性**作為能力**留下。應改的是「預設全面惰性」，不是禁止惰性。

現存語言已付過學費：PureScript、Idris 2、Unison、Roc、Koka、OCaml 5。它們不是對手，是設計筆記。

## 文件

| | English | 中文 |
|---|---|---|
| 目錄 | [docs/en/index.md](docs/en/index.md) | [docs/zh-Hant/index.md](docs/zh-Hant/index.md) |
| 語言 | [docs/en/language.md](docs/en/language.md) | [docs/zh-Hant/language.md](docs/zh-Hant/language.md) |
| 現況 | [docs/en/status.md](docs/en/status.md) | [docs/zh-Hant/status.md](docs/zh-Hant/status.md) |

REPL 課文（`? 0`–`? 11`）是英文：[docs/learn](docs/learn)。示例：[core0_hello](examples/core0_hello.h2o)、[count](examples/count/)、[wc](examples/wc/)。

設計筆記與版本閘門（中文檔案）：[docs/00-principles.md](docs/00-principles.md) … [docs/20-core11.md](docs/20-core11.md)。

## 30 分鐘

```
./h2o run examples/core0_hello.h2o
./h2o run examples/count examples/count/sample.txt    # 印 count 3；once + Vect
./h2o build --native examples/count -o /tmp/count     # 發 C 函數再 cc，是機器碼
/tmp/count run examples/count examples/count/sample.txt
./h2o build --target wasm examples/count -o /tmp/count.wasm
wasmtime --dir=. /tmp/count.wasm run examples/count examples/count/sample.txt
# 套件＝目錄。沒有 registry。
# ./h2o vendor <git-url> [name]   → vendor/<name>/
./h2o repl          # ? 0 課表（英文）；? 11 once／Vect／Wasm
./test
```

`--native` 把每個 `def` 發成 C 函數再 `cc`（boxed `Val` + GC，不是 unbox）。`--vm` 是解釋器映像。`--target wasm` 仍是同一套 VM 交叉編成 WASI。`]` 沒有套件註冊中心。`h2o vendor <git-url> [name]` 把 git 樹拷進 `vendor/<name>/`；之後 `h2o run vendor/name`／`:load vendor/name/` 與本地目錄相同。

`h2o fmt` 只清行尾空白。`h2o build -o *.py`／`compiler/rt.py` 是 **unsupported debug**，不是第二套語意。

需要 `cc`（macOS 上是 Xcode Command Line Tools）。Wasm 要 [WASI SDK](compiler/wasi-sdk.sh) 與 `wasmtime`；沒有就跳過那些測試。

沒有 `bin/h2o` 時，`./h2o` 會用 `cc` 編 `compiler/seed.c`（可攜 C 快照，不靠 Python）。改完 `compiler/*.h2o` 後若要更新快照：

```
./h2o build --vm --no-check compiler/h2oc.h2o -o /tmp/h2o-seed
cp /tmp/h2o-seed.c compiler/seed.c
```
