# Core-0：下一個要做完的東西

**v0.1 已通過驗收。** 這份是當時的切片規格。實作已不在 `proto/`（目錄已刪）；今天的入口是倉庫根目錄的 `./h2o`（`bin/h2o` VM 映像），源在 `compiler/*.h2o`。現況對照見 [`docs/compare.md`](../compare.md)。

成功標準：

```
h2o run examples/core0_hello.h2o
```

印出 `active: Ada, Edsger`，錯誤訊息讀得懂，沒有 pragma。

Core-0 不是完整 Core。效應列、trait、列多型都不在這裡。先把「縮排 + 類型 + 資料」做對，再接效應。

## 做

| 項目 | 範圍 |
|---|---|
| 詞法 | UTF-8、`#` 註解、4 空格、`INDENT`/`DEDENT` |
| 定義 | `def`、值繫結、`type` ADT、紀錄字面 |
| 類型 | `Int` `Bool` `Text` `List[A]` `Maybe[A]`，HM 推導，註解可省略 |
| 控制 | `if`/`elif`/`else`（條件無括號）、`match`/`case`、窮盡檢查（ADT） |
| 運算 | 呼叫 `f(x, y)`、欄位 `p.name`、管線 `x \|> f()` / `x \|> f(a)`、洞 `_`、`and`/`or`/`not`、比較鏈、`+ - *` |
| 資料 | 列表、封閉紀錄、`{p..., age: n}`、`Some`/`None` |
| 內建 | `println`、`filter`、`map`、`join`、`==` |
| 入口 | `def main():`；`println` 是暫時的主機原語，**還不是**效應列 |
| 格式 | 先手寫符合 `h2ofmt` 預期；工具本身可第二步 |
| 診斷 | 列號、程式設計師詞彙（「這支函數少一個參數」），禁止 dump 內部 AST 當錯誤 |

## 不做（看見就停）

- `{IO}` 效應列、`handle`、`State`、`Except`
- `trait` / `impl`
- 列多型 `...R`、開放和型別
- `from`/`import` 多檔模組（Core-0 單檔 + 內建 Prelude）
- `fn` 多行以外的糖：`for`、comprehension、`try`、`with`
- 線性、總性、依賴類型
- 自訂運算子、並置呼叫、方法鏈
- 相容 Haskell

`main` 裡能印東西，是主機權限，不是語言的效應系統。階段 1 再把 `println` 收進 `{IO}`。

## 建議實作切法（按週，不要平行開編譯器架構）

1. **詞法 + 縮排** — 對 `hello` 印 token 流，INDENT/DEDENT 測過。
2. **解析** — 得到 AST；`examples/core0_hello.h2o` 能 parse。
3. **脫糖** — `|>`、`_`、`{p..., x: e}` 變成普通呼叫與紀錄。
4. **類型** — HM；`List`/`Maybe`/紀錄；`match` 窮盡。
5. **求值** — 嚴格、樹走訪即可；`println` 接到 stdout。
6. **診斷** — 刻意寫錯的測資（少參數、`if (x):`、`xs.filter`）必須失敗得清楚。

語言用什麼寫原型：**先 Python**（縮排解析最快，對齊表面語言）。過關後再考慮用別的語言重寫正式編譯器。原型允許醜，不允許偷偷加入「不做」清單裡的糖。

## 驗收檔

`examples/core0_hello.h2o`（單檔、無 import）：

```
type User = {name: Text, active: Bool}

def active_names(users: List[User]) -> Text:
    users
        |> filter(_.active)
        |> map(_.name)
        |> join(", ")

def main():
    users = [
        {name: "Ada", active: True},
        {name: "Grace", active: False},
        {name: "Edsger", active: True},
    ]
    if users != [] and active_names(users) != "":
        println(f"active: {active_names(users)}")
```

過關：跑起來輸出那一行；改成 `users.filter(_.active)` 必須報「`.` 不是方法，請用 `|>`」。

## 過關之後才做的下一刀

Core-0 能跑 → **階段 1 第一刀：一階效應列**（`{IO}`、`{Except[E]}`、一個 `handle`）。那才是 H2O 有別於「帶類型的 Python」的起點。

在那之前，不開 trait、不開並發、不開套件管理。
