# V1.0 之後

Core 1 已凍結。**不准再開語法。**  
下一步不是「更像 Haskell」，是讓現在這份合約在工程上站得住。

現況（2026-09-13）：語言形狀仍是 Core 1。執行器是 C bytecode VM。`h2o check` 是 HM 子集（見 [15-hm.md](15-hm.md)）。T5–T6：`A: Eq` 與兩份 `Functor` impl。無 HKT kind、無 overlapping solver、無列多型推導。`proto/` 已刪。和別的語言比見 [compare.md](compare.md)。

**能開發已過（V2.0.0）**，見 [17-v2.md](17-v2.md)。13-next 這三刀（V1.0.1／宿主／Typed·Systems）不涵蓋 GC、模組邊界、Prelude 說實話。Typed 1 與 Systems 1 的閘門在 [18-v2x.md](18-v2x.md)。

---

## 原則

1. 黃金測試是合約。行為一變，先改測試再改編譯器。
2. 一件事一種寫法仍然有效。新能力必須減少特例，不是加方言。
3. 換後端或加類型之前，黃金測試與 `./test` 是合約。行為一變，先改測試再改編譯器。

---

## 三刀（按順序，不准搶跑）

### 第一刀 — V1.0.1：讓 Core 1 說到做到

**已完成。** 黃金檔在 `tests/golden/`。`h2o fmt` 只清空白（見 `compiler/fmt.h2o`）。Prelude 介面在 `prelude/io.h2o`、`prelude/list.h2o`。

把報告裡已經寫下、實作卻還鬆的洞補上。沒過這一刀，不准談 V1.1。

| 做 | 驗收 |
|---|---|
| 效應列在類型裡強制 | 純函數裡寫 `State.get()` 是類型錯誤，不是執行期才爆 |
| `match` 窮盡 | 少一個建構子＝編譯失敗；訊息點出名 |
| 頂層定義可前後參照 | `main` 可以呼叫寫在後面的 `def` |
| 診斷黃金檔落盤 | `tests/golden/*.txt` 逐字鎖定；CI 跑 |
| `h2o fmt` 對 AST 重印或明確維持「只清空白」 | 全部 examples 兩遍不變；README 與行為一致 |
| Prelude 當真正模組 | `io`／`list` 有檔可讀，不只是內建表 |

**不做：** 新語法、新效應、換語言重寫。

### 第二刀 — V1.1：正式編譯器宿主

V1.0.1 的黃金測試當遷移合約。

1. **寫一頁評測**（Rust vs OCaml）：錯誤訊息可控、出碼路徑、誰養得起。評測沒寫完不准開工。
2. 移植：**詞法 → 解析 → 脫糖 → 類型 → 求值／IR**。宿主路徑後來改成 H2O 自己寫編譯器 + C VM，不另起 Rust／OCaml。
3. `h2o build` 產出 VM 映像（`--native` 同一套 ISA）。`proto/` 已不存在。
4. 前端能編譯自己的**詞法＋解析**可當加分，**不是閘門**。

**不做：** LLVM 研究、增量編譯產品化、套件註冊中心。

### 第三刀 — Typed 1 與 Systems 1（可分叉，但都在 V1.1 之後）

| 線 | 內容 | 驗收 |
|---|---|---|
| **Typed 1** | 見 [18-v2x.md](18-v2x.md) E0–E1：`once` 接到 `with`／別名／欄位；`Vect[n, a]` 用 `Z`／`S[_]` | `once` 漏關／用兩次紅；空 `vect_head` 是類型錯誤 |
| **Systems 1** | 見 [18-v2x.md](18-v2x.md) E2–E3：同一套 VM 交叉編成 WASI；`ffi_call` 有類型 | `wasmtime` 跑出與 `h2o run` 相同的 stdout；錯型 FFI 紅 |

兩線都自願加註解／加後端，不裂 Core 1。完整證明助理、內容定址、套件 registry 仍在更後面。

---

## 明確還不是下一步

- 用 H2O 重寫整個編譯器（自舉）
- 再做一套 `LANGUAGE` 開關
- 把 IO 加厚
- 子型別、隱式參數、宏
- 「先換 Rust 再補類型」——順序反了

---

## 此刻

V1.0.1、自舉弱固定點、日常不靠 Python、種子是 C、`proto/` 刪除，都已綠。  
**T0–T6 已綠。內建類型與 REPL 已綠。** 見 [15-hm.md](15-hm.md)、[16-repl.md](16-repl.md)。不准再開語法。

能開發＝[17-v2.md](17-v2.md) 的 D0→D4。**D0–D4 已綠。版本 V2.0.0。**  
Typed 1／Systems 1 的具體閘門：[18-v2x.md](18-v2x.md)。**E0–E4 已綠。版本 V2.5.0。**  
機器碼線：[19-v3.md](19-v3.md)。**V3.0.0 已綠。** 類型線：[20-core11.md](20-core11.md)。**V3.1.0 已綠**（列推導／HKT kind／overlapping 只報錯）。不准做套件註冊中心。
