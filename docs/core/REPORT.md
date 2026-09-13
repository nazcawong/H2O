# H2O Core 1 — 語言報告

版本：**Core 1**（V1.0）。這份報告與 `h2o run`／`check`／`fmt`／`build` 的行為同一份合約。破壞性變更走 Core 2，不開 pragma。

## 計算模型

- 純。沒有隱式副作用。效果出現在類型裡：`-> {IO, State[S], Except[E]} A`。
- **嚴格為預設**。惰性結構是 `Stream`，不是 `List`。
- 不可變為預設。`=` 是繫結。

## 表面

Python 式縮排（4 空格）、`and`／`or`／`not`、`if cond:` 無括號。呼叫 `f(x, y)`。多步資料只用 `|>`。`.` 只做欄位與模組，不是方法。

命令：`h2o run`、`h2o check`、`h2o fmt`、`h2o build -o`。  
`h2o fmt` 只去掉行尾空白與檔尾空行，並保證結尾換行（不是 AST 重印）。  
`h2o check` 只做類型檢查，不執行。

## 類型

- Hindley–Milner 推導、參數多型、高階種類（`F[_]`）。
- ADT 模式比對，**窮盡檢查**（少建構子＝編譯失敗）；開放和型別 `<Click: T, ...R>` 需要 `case _`。
- 列紀錄 `{name: Text, ...R}`；`update`；`{p..., field: v}`。
- `trait`／`impl`。orphan 必須寫 `impl orphan`。
- 安全 Prelude：`Text` 不是 `[Char]`；`xs[i]` 是 `Maybe`；沒有 List 的總 `head`。

## 效應

- `{IO}` `{State[S]}` `{Except[E]}` `{Conc}`。
- 沒宣告的效應不能用：純函數裡 `State.get()` 是類型錯誤。
- 未標註的 `main` 隱含 `{IO}`（程式入口）。
- `handle expr with:` 在邊界解釋。`IO` 只是處理器之一。
- `conc.together`：兩個任務；父作用域結束則一起結束。
- `Stream.iterate`／`take`／`map`。

## Typed（可關）

- `once T`：恰好用一次。關掉註解，Core 含義不變。
- `total def`：標註此定義應窮盡；不強迫全語言總性。
- 完整依賴類型不在 Core 1。

## 不做

GHC 相容、自訂中綴、`$`、方法鏈、FFI／WASM、自舉作為發布閘門、pragma 清單。

## 驗收入口

```
./h2o run examples/core0_hello.h2o
./h2o run examples/main.h2o
./h2o run examples/traits.h2o
./h2o run examples/records.h2o
./h2o run examples/effects.h2o
./h2o run examples/stream.h2o
./h2o run examples/conc.h2o
./h2o run examples/linear.h2o
./h2o run examples/tools/stats.h2o
```
