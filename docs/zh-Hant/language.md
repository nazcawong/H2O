# 語言

[English](../en/language.md) | **中文**

Core 1 已凍結。沒有 pragma，沒有第二套語法。完整合約：[core/REPORT.md](../core/REPORT.md)。

## 表面

4 空格縮排。`and`／`or`／`not`。沒有 `;`，沒有區塊 `{}`。呼叫 `f(x, y)`。多步資料只用 `|>`。`.` 只做欄位或模組，不是方法。

整數：`+ - * /`（向零）、`//`（向下）、`%`、`**`（右結合）。`abs(n)`、`isqrt(n)`。
浮點：`2.0`、`3.14`；`+ - * / **`；`sqrt(x)`、`float(n)`、`int(x)`。Int 與 Float 不能混。

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

| 做 | 寫 | 不寫 |
|---|---|---|
| 條件 | `if a and b:` | `if (a && b) {` |
| 呼叫 | `f(x)` | `f x` |
| 欄位 | `p.name` | `xs.map(f)` |
| 多步 | `xs \|> f() \|> g()` | `$`、方法鏈 |
| 一步列表 | `[e for x in xs if p]` | 與 `|>` 混用當風格 |

## 分層

| 層 | 用法 |
|---|---|
| **Core** | 預設。純、嚴格、ADT、HM + HKT、trait、效應列、列紀錄 |
| **Typed** | 自願：`once`、`total def`、`Vect[n, a]` |
| **Systems** | 執行期：`--native`、WASI、FFI |

往上走是加註解，不是打開不相容擴充。

## Core 1（檢查器會擋的）

- 純。效果在類型裡：`-> {IO, State[S], Except[E]} A`。沒宣告的效應是錯。未標註的 `main` 隱含 `{IO}`。
- 嚴格為預設。惰性結構是 `Stream`，不是 `List`。
- `match` 窮盡。`Text` 不是 `[Char]`。`xs[i]` 是 `Maybe`。
- 關閉紀錄 `{name: Text}` 不能多欄、不能少欄。`{name: Text, ...R}` 可以多。
- `trait Functor[F[_]]`：`Int` 不是 `F[_]`。同一型兩份 impl 報錯，不求解。
- `once T`：恰好用一次。不是借用檢查。

永不：GHC 相容、`$`、並置、方法鏈、自訂中綴、套件註冊中心、pragma 清單。

## 命令

`h2o run` · `h2o check` · `h2o fmt`（只清行尾空白，不是排版器）· `h2o build --native` · `h2o build --vm` · `h2o build --target wasm` · `h2o vendor` · `h2o repl`

REPL：`?` 課文、`;` shell、`]` 套件（只有目錄）。課文是英文（`docs/learn`）。真終端：↑↓ 歷史（`~/.h2o/history`）、←→ 編輯、Tab 補當前詞（`:load`、關鍵字、session 名字）。啟動印水滴包住 `H2O`。
