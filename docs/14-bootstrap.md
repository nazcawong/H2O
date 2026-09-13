# 從現在到自舉

自舉的意思只有一個：**用 H2O 寫的編譯器，能編譯自己，產出的產物再編譯自己，行為與黃金測試一致。**

**狀態（2026-09-12）：已過弱固定點。** `proto/` 已刪。種子是 `compiler/seed.c`（`cc`，不靠 Python）。日常入口是 `bin/h2o`（C bytecode VM）。`./test` 含 `self-s1` 與 `self-fixed-point`（`s1.c` 與 `s2.c` 位元組相同）。下文是當時的閘門計畫，後端後來改成同一套 ISA 的 VM，而不是「發獨立 Python」。

原則：每一階段有可執行的閘門。閘門沒綠，不准開下一階段。不准為了自舉而加語法。

---

## 當時缺什麼（歷史；下列缺口已關）

| 缺口 | 為什麼當時擋住自舉 |
|---|---|
| 執行器是 Python 樹走訪 | H2O 程式不能獨立成為編譯器產物 |
| `h2o build` 依賴 `proto/` | 產物不是編譯器，是包裝器 |
| 沒有穩定 IR | 沒有「編譯」這一步，只有「解釋」 |
| 編譯器未用 H2O 寫 | 沒有被編譯的對象 |
| 沒有二次編譯比對 | 無法證明固定點 |

語言本身（Core 1）已經夠寫編譯器前端：`Text`、`List`、ADT、`match`、模組、`{IO}`、`trait`。不必先做依賴類型或 FFI。

---

## 總圖

```
stage 0   Python 原型（歷史，V1.0.1；`proto/` 已刪）
    │     黃金測試 = 合約
    ▼
B0        宿主評測（一頁，寫完才能寫碼）
    ▼
B1        宿主編譯器（Rust 或 OCaml）
          詞法→解析→脫糖→類型→IR→後端
          黃金測試全綠；build 不依賴已刪的 proto/
    ▼
B2        H2O 寫詞法＋解析
          stage1 編譯 compiler/parse.h2o
          對同一批 .h2o，token/AST 與 stage1 一致
    ▼
B3        H2O 寫脫糖＋類型
          黃金診斷逐字一致
    ▼
B4        H2O 寫 IR ＋ 後端（或 bytecode VM）
          `h2o build compiler.h2o -o stage2`
    ▼
B5        自舉固定點
          stage2 編譯自己 → stage3
          stage2 與 stage3 對黃金測試行為相同
```

B5 才叫自舉。B2 只是「編譯器用 H2O 寫了一部分」。

---

## B0 — 宿主評測（下一件要寫的文件，不是下一堆程式）

一頁，三欄：Rust / OCaml / 繼續 Python。

必須回答：

1. 錯誤訊息能否完全由我們寫（不能靠編譯器內部詞）
2. 後端最短路徑：bytecode VM，還是原生（C 或 LLVM）
3. 誰養得起（工具、編譯時間、跨平台）
4. 與現有 `tests/golden/` 如何對接

評測結論要寫死：**一種宿主、一種後端。** 沒寫完不准開 B1。

建議預設（可被評測推翻）：**OCaml 或 Rust + 小型 bytecode VM**。VM 比 LLVM 更接近「H2O 稍後能自己發出的東西」，自舉才走得通。

---

## B1 — 宿主編譯器（真正的 stage 1）

把當時 Python 原型的管道搬過去，行為以黃金測試為準。

| 做 | 驗收 |
|---|---|
| 詞法／縮排／`|>` 續行 | `h2o tokens` 與 stage 0 對同一檔一致 |
| 解析＋脫糖 | examples 全 parse |
| 類型（含效應列、窮盡、前後參照） | `tests/golden/*.txt` 逐字 |
| IR（A-normal 或小型 CPS 即可） | 每個 Core 1 程式有 IR dump |
| 後端：bytecode **或** 原生，擇一 | `h2o build examples/core0_hello.h2o -o x && ./x` 印 `active: Ada, Edsger`，產物不依賴已刪的 proto |
| 凍結 Python 原型 | 之後只修合約漏洞，不加功能 |

**不做：** 自舉、FFI、宏、新語法。

B1 結束時：有一台不靠 Python 套件樹的 H2O 編譯器。這是自舉的**底座**，還不是自舉。

---

## B2 — 詞法與解析用 H2O 寫

目錄建議：

```
compiler/
  lex.h2o
  parse.h2o
  ast.h2o
```

stage1 編譯這些檔，產出的解析器對 `examples/*.h2o` 的 AST 與 stage1 內建解析器一致（黃金 AST 或結構雜湊）。

閘門：`compiler/parse.h2o` 能 parse 自己。

這一階段編譯器**執行**仍是 stage1。只是前端換成 H2O 產物。

---

## B3 — 脫糖與類型用 H2O 寫

```
compiler/
  desugar.h2o
  infer.h2o
  report.h2o    # 人話錯誤，對齊 golden
```

閘門：`tests/golden/` 在「H2O 類型檢查器」下仍然逐字相同。  
效應列、`match` 窮盡、orphan、`once` 用兩次，全部要過。

---

## B4 — IR 與後端用 H2O 寫

到這裡 H2O 必須能表達：

- 讀寫檔（`{IO}`，已有）
- 整數／文字／列表／ADT（已有）
- 模組（已有）
- 發出 **bytecode 檔** 或 **C 原始碼**（選 B0 定的那一種）

閘門：

```
stage1-h2o build compiler.h2o -o stage2
./stage2 run examples/core0_hello.h2o
```

`stage2` 是 H2O 編譯器編譯出來的編譯器。還差固定點。

---

## B5 — 固定點（自舉完成）

```
./stage2 build compiler.h2o -o stage3
./stage3 run examples/core0_hello.h2o
./stage3 build compiler.h2o -o stage4
```

通過條件（滿足一項即可，評測裡寫死）：

- **強：** `stage3` 與 `stage4` 位元組相同；或
- **弱：** 兩者對全部 golden + examples 行為相同（輸出與錯誤字串一致）

弱條件允許後端非確定性（時間戳）；強條件要求可重現 build。

過關之後：Python 原型降為 **stage 0 歷史**，不再加功能。日常開發用 stage2。（2026-09-12：`proto/` 已刪，日常就是 VM 映像。）

---

## 編譯器用得到、Core 1 已有的

不必為自舉加語言：

- `Text`、`List`、`Maybe`、`Result`
- ADT + 窮盡 `match`
- 模組、`pub`
- `{IO}`、`handle`
- `trait` 可選；編譯器內部不必用 type class

自舉**不需要**先做：依賴類型、FFI、WASM、線性檔案、套件註冊、內容定址。

若 B4 選「發 C」，才需要 Systems 的一點點：能寫出文字檔（IO 已夠），不必 FFI。

---

## 明確不要的捷徑

| 捷徑 | 為什麼不算 |
|---|---|
| H2O 檔 `import` Python 的 lexer | 還在 stage 0 |
| 把 `.py` 自動譯成 `.h2o` | 沒有設計，只有翻譯腔 |
| 先 LLVM 再自舉 | 後端 H2O 寫不出來，B4 卡死 |
| 自舉同時加語法 | 合約漂走，黃金測試失效 |
| 未過 B1 就開 `compiler/*.h2o` | 沒有獨立編譯器可餵這些檔 |

---

## 與 V1.0.1 / V1.1 的銜接

| 現在 | 自舉路上的名字 |
|---|---|
| V1.0.1 已完成 | stage 0 合約穩定 |
| 13-next 的「V1.1 宿主」 | **B0 + B1** |
| 自舉本身 | **B2–B5**，在 B1 之後 |

所以「下一步到自舉」的**下一步**仍是 B0：一頁宿主評測。不是明天開始寫 `compiler/lex.h2o`。

---

## 狀態（2026-09-12）

**BH 子集已自舉，跑在 C VM 上。** `proto/` 已刪。歷史評測見 [`host-eval.md`](host-eval.md)（當時結論是發 Python；實際後端是 bytecode VM）。現況對照見 [`compare.md`](compare.md)。

| 階段 | 產物 | 閘門 |
|---|---|---|
| B0 | `docs/host-eval.md` | 當時：發 Python。現在：C VM |
| B1 | `h2o build` → VM 映像（可選 `-o *.py`） | `examples/core0_hello.h2o` 印 `active: Ada, Edsger` |
| B2–B4 | `compiler/h2oc.h2o` | 詞法＋解析＋檢查＋發碼，BH 子集 |
| B5 | 映像編譯自己 → 再編譯自己 | **弱固定點**：兩次發出的 `.c` 陣列相同 |

驗收：

```
./h2o build --vm --no-check compiler/h2oc.h2o -o /tmp/h2o-s1
/tmp/h2o-s1 build --vm --no-check compiler/h2oc.h2o -o /tmp/h2o-s2
cmp /tmp/h2o-s1.c /tmp/h2o-s2.c
./test
```

`h2o build --no-check` 只給 `compiler/h2oc.h2o`：檢查器對這份互相遞迴的編譯器不是完整 HM，日常 examples 仍先檢查再發碼。

`h2oc` **輸入**已涵蓋 Core 1 日常驗收：`|>`、`_`、f-string、`impl`、`update`、多檔 `from`／`import`。  
`h2oc` **自己**仍用 BH 寫（沒有 `|>`／`_`／f-string），所以固定點不漂。

能 `./h2o run`（同一套 VM）：`core0_hello`、`hello`、`main`+`user`、`traits`、`records`、`bh_hello`、`effects`、`stream`、`conc`、`linear`。
