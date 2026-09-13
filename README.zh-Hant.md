# H2O

[English](README.md) | **中文**

**V3.1.0** — 能寫長期跑的本地 CLI；`--native` 發 C 函數再 `cc`，是機器碼（tagged `Val`）；`--vm` 是解釋器；`h2o vendor` 把 git 樹變成目錄。Core 1.1：關閉列、`F[_]` kind、overlapping 只報錯。不是 GHC／pip／cargo。沒有套件註冊中心。

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

## 語言分層

用三層穩定報告取代「GHC2024 + 二十個 pragma」：

| 層 | 內容 | 怎麼用 |
|---|---|---|
| **Core** | 純、嚴格預設、ADT、模式比對、HM + HKT、類型類、效應列、列紀錄、安全 Prelude | 預設。多數程式停在這裡 |
| **Typed** | 線性／仿射、索引／依賴子集、總性註解、可視量化 | 自願加註解，不是另一個方言 |
| **Systems** | 原生表示、未提升型別、FFI、線性緩衝、後端原語 | 執行期與互操作 |

往上走是加註解，不是打開不相容擴充。

## 若只能做四件

1. **效應系統入核心**——消滅最常見的架構痛
2. **列紀錄／開放和型別**——消滅最常見的日常痛
3. **嚴格預設 + 安全文字／Prelude**——消滅最常見的執行時痛
4. **線性資源 + 實用依賴類型**——讓「非法狀態不可表示」成為常規工具

## 文件

GitHub 首頁是英文 [README.md](README.md)。`docs/` 多數仍是中文。

| 文件 | 內容 |
|---|---|
| [docs/00-principles.md](docs/00-principles.md) | 取捨原則與成功標準 |
| [docs/01-keep.md](docs/01-keep.md) | 必須保留的靈魂 |
| [docs/02-discard.md](docs/02-discard.md) | 應捨棄或翻轉的預設 |
| [docs/03-redesign.md](docs/03-redesign.md) | 有價值、原樣搬會帶舊病 |
| [docs/04-must-add.md](docs/04-must-add.md) | 核心必須新增、不能再用擴充模擬 |
| [docs/05-engineering.md](docs/05-engineering.md) | 診斷、模組、元程式、編譯、標準庫 |
| [docs/06-optional.md](docs/06-optional.md) | 選配層：不要重蹈擴充爆炸 |
| [docs/07-do-not-add.md](docs/07-do-not-add.md) | 明確不要從別的語言搬進來的表面功能 |
| [docs/08-layers.md](docs/08-layers.md) | Core / Typed / Systems 報告邊界 |
| [ROADMAP.md](ROADMAP.md) | 零到 V1.0（歷史路線，已完成） |
| [docs/compare.md](docs/compare.md) | **現況**：和 GHC／OCaml／Koka／Python／Go／Elixir／Rust 比 |
| [docs/13-next.md](docs/13-next.md) | V1.0 之後（V1.0.1、自舉、去 proto 已完成） |
| [docs/17-v2.md](docs/17-v2.md) | **到 V2.0.0**：能開發的路線（D0–D4，已完成） |
| [docs/18-v2x.md](docs/18-v2x.md) | **到 V2.5**：Typed 1 然後 Systems 1（E0–E4，已完成） |
| [docs/19-v3.md](docs/19-v3.md) | **V3.0.0**：`--native` 發機器碼；套件仍是目錄 |
| [docs/20-core11.md](docs/20-core11.md) | **V3.1.0**：Core 1.1 列推導／HKT kind／overlapping 只報錯 |
| [docs/15-hm.md](docs/15-hm.md) | T0–T4：HM 子集（已完成） |
| [docs/16-repl.md](docs/16-repl.md) | REPL：`?` 入門教學（`docs/learn`）；`;` shell；`]` pkg |
| [docs/14-bootstrap.md](docs/14-bootstrap.md) | 自舉閘門（歷史 B0–B5；現況見文首） |
| [docs/09-roadmap.md](docs/09-roadmap.md) | 優先序與階段（對照表） |
| [docs/core/CORE-0.md](docs/core/CORE-0.md) | v0.1 Core-0 規格（實作已不在 proto） |
| [docs/10-syntax.md](docs/10-syntax.md) | **具體語法**：縮排、括號規則、文法 |
| [docs/11-readability.md](docs/11-readability.md) | 可讀性標準：Python / Elixir / Go / Haskell 各取一件 |
| [docs/syntax-cheatsheet.md](docs/syntax-cheatsheet.md) | 一頁速查 |
| [notes/related-languages.md](notes/related-languages.md) | 已付學費的設計筆記 |
| [examples/core0_hello.h2o](examples/core0_hello.h2o) | v0.1 單檔驗收 |
| [examples/main.h2o](examples/main.h2o) | v0.2 多檔驗收 |
| [examples/traits.h2o](examples/traits.h2o) | v0.3 trait／impl／HKT |
| [examples/wc/](examples/wc/) | V2.0.0 黃金工具：多檔 CLI（`pub` + 目錄套件） |
| [examples/count/](examples/count/) | **V2.5.0 黃金工具**：`once` 開檔 + `Vect` 定長 |

## 目錄

```
H2O/
├── README.md           English
├── README.zh-Hant.md   繁體中文
├── ROADMAP.md          零到 V1.0（歷史）
├── h2o                 POSIX 包裝：exec bin/h2o
├── test                POSIX 測試架（不靠 Python）
├── compiler/           H2O 編譯器 + C VM 核 + C 種子
├── prelude/
├── tests/
├── docs/
├── examples/
└── notes/
```

沒有 `proto/`。日常入口是 `bin/h2o`（C bytecode VM，標記－清除 GC）。現況對照：[docs/compare.md](docs/compare.md)。合約：[docs/core/REPORT.md](docs/core/REPORT.md)。能開發路線（已完成）：[docs/17-v2.md](docs/17-v2.md)。

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
./h2o repl          # ? 0 課表；? 11 once／Vect／Wasm
./test
```

`--native` 把每個 `def` 發成 C 函數再 `cc`（機器碼，仍是 tagged `Val` + GC）。`--vm` 是解釋器映像。`--target wasm` 仍是同一套 VM 交叉編成 WASI。`]` 沒有套件註冊中心。`h2o vendor <git-url> [name]` 把 git 樹拷進 `vendor/<name>/`；之後 `h2o run vendor/name`／`:load vendor/name/` 與本地目錄相同。

沒有 `bin/h2o` 時，`./h2o` 會用 `cc` 編 `compiler/seed.c`（可攜 C 快照，不靠 Python）。改完 `compiler/*.h2o` 後若要更新快照：

```
./h2o build --vm --no-check compiler/h2oc.h2o -o /tmp/h2o-seed
cp /tmp/h2o-seed.c compiler/seed.c
```
