# H2O 具體語法（Core）

Haskell 的思想留下；**表面語法不沿用**。可讀性標準見 [11-readability.md](11-readability.md)：Python 降語法噪聲，Elixir 管線，Go 一件事一種寫法，Haskell 只負責含義。

括號只用於呼叫、元組、改變優先序——不用來堆區塊、不用來包 `if`、不用來餵 `$`。`.` 不是方法鏈。

> 這份是可實作的語法決策。衝突時：少噪聲、一種寫法，勝過多一種糖。

完整對照見文末 [Haskell → H2O](#haskell--h2o)。一頁速查見 [syntax-cheatsheet.md](syntax-cheatsheet.md)。

---

## 1. 設計決策（先讀這個）

| 決策 | 選擇 | 為什麼 |
|---|---|---|
| 區塊 | 縮排（4 空格），`:` 開頭 | Python；消滅 `{}`、`;`、`do` |
| 布林 | `and` `or` `not` | 英語，不是 `&&` `\|\|` `!` |
| 條件 | `if user_is_active and data_is_ready:` | 條件上**禁止**括號 |
| 註解 | `#` | 不是 `--` |
| 命名 | 值／函數 `snake_case`；類型／建構子／效應 `PascalCase` | 類型一眼可辨 |
| 定義 | `def f(x: Int, y: Int) -> Int:` | 參數列括號消解 `->` 歧義；其餘像 Python |
| 呼叫 | **一律** `f(x, y)`、零參 `f()` | 一種寫法。並置 `f x y` 會讓 `f x + 1` 解錯 |
| 欄位／模組 | `.` **只做這兩件事** | `user.name`、`io.println`。沒有 UFCS、沒有方法鏈 |
| 多步資料 | **只用** `|>` | Elixir。`x \|> f()` = `f(x)`；`x \|> f(a)` = `f(x, a)` |
| 一步列表 | `[e for x in xs if p]` | Python comprehension；與管線分工見下 |
| 部分套用 | `_` | 不 curry、沒有 `(+1)` |
| 類型 | `List[A]`、`(A, B) -> {IO} C` | `[]` 類型參數；`{}` 效應／紀錄 |
| 類型類 | `trait` / `impl` | 不搶 Python 的 `class` |
| 運算子 | 小集合，禁止自訂中綴 | Go：少糖。不准再長 `$`、`>>=` |
| 格式 | `h2ofmt` 唯一 | 與 `gofmt` 相同：不討論風格 |
| 求值 | 最後一個運算式即回傳 | 仍像偽代碼 |

一步 comprehension **對** 多步 `|>` 不是兩種風格，是兩種問題。見 [11-readability.md](11-readability.md)。

---

## 2. 長相（先看懂全貌）

```
from io import println
from list import List

type User = {name: Text, active: Bool, age: Int}

def active_names(users: List[User]) -> Text:
    users
        |> filter(_.active)
        |> map(_.name)
        |> join(", ")

def main() -> {IO} ():
    users = [
        {name: "Ada", active: True, age: 36},
        {name: "Grace", active: False, age: 85},
        {name: "Edsger", active: True, age: 40},
    ]
    println(f"active: {active_names(users)}")
```

條件長得像英語：

```
if user_is_active and data_is_ready:
    start()
```

`x |> f()` 就是 `f(x)`。`x |> f(a)` 就是 `f(x, a)`。沒有 `$`，沒有方法鏈，沒有 `.` 函數合成。

---

## 3. 詞法

- 原始碼 UTF-8。縮排 **4 空格**。Tab 是錯誤。
- 註解：`#` 到行尾。文件字串用 Python 式 `"""..."""`。
- 識別字：`[A-Za-z_][A-Za-z0-9_]*`。模組路徑用 `.`：`io.println`。
- 數字：`42`、`1_000`、`3.14`、`0xFF`。
- 字串：`"hello"`、`f"hello {name}"`、`r"raw \n"`、`"""multi"""`。
- 字元不是文字。單個字元是 `Char`，寫 `'a'`。日常文字只有 `Text`。
- 延續：下一行若以 `|>`、`and`、`or`、`,` 開頭，且較前一行更縮進，則延續上一運算式。不必 `\`。`.` 不拿來斷行接方法——沒有方法鏈。

### 保留字

```
and     as      case    def     elif    else    except
fn      for     from    handle  if      impl    import
in      match   not     opaque  or      priv    pub
return  total   trait   try     type    with
```

`True`、`False`、`None` 是 Prelude 建構子，不可當變數名。`_` 在模式是萬用；在運算式是洞。

### 固定運算子（僅此）

```
.   欄位／模組路徑（不是方法、不是合成）
()  呼叫
[]  索引／切片／類型參數
{}  紀錄／效應列／集合

**  *  /  //  %
+  -
==  !=  <  >  <=  >=
and  or  not
in  not in
|>
...   列尾／展開
,     分隔
:     區塊起點、類型註解、紀錄欄位
=     繫結（不是運算式）
```

沒有：`$` `&` `>>=` `>>` `<<` `<*>` `<$>` `<|>` `/=` `` `infix` `` `::` `=>`（類型）、自訂運算子。

---

## 4. 括號規則（什麼時候一定要、什麼時候不准多寫）

**要寫**

| 場合 | 例子 |
|---|---|
| 函數定義的參數列 | `def add(x: Int, y: Int) -> Int:` |
| 函數呼叫 | `add(1, 2)`、`get()` |
| 元組 | `(x, y)`、`(Text, Int)` |
| 改變算術優先序 | `(a + b) * c` |
| 建構子帶參數 | `Some(x)`、`Cons(x, xs)` |

**不要寫**

| 場合 | 寫 | 不寫 |
|---|---|---|
| `if` / `elif` / `while` 條件 | `if x > 0:` | `if (x > 0):` |
| `match` 對象 | `match xs:` | `match (xs):` |
| `for` | `for x in xs:` | `for (x in xs):` |
| 區塊 | 縮排 | `{ ... }`、`do ...` |
| 零參數**值**（不是函數） | `True`、`None`、`Nil` | `None()` |
| 管線 | `xs \|> map(f) \|> sum()` | `sum(map(xs, f))` 外層再包一層、或 `xs.map(f)` |
| 比較 | `0 <= x < 10` | `(0 <= x) and (x < 10)`（仍合法，非必須） |
| 布林 | `a and not b` | `a && (!b)` |

單參數呼叫**仍要括號**：`println(name)`、`sqrt(x)`。Go：一種寫法。少括號靠縮排、`if cond:`、`and`/`or`、管線，不靠並置、不靠可選括號。

---

## 5. 類型語法

```
Int           Text         Bool        ()
List[A]       Maybe[Int]   Result[A, E]
{name: Text, age: Int}
{name: Text, ...R}                    # 列多型：還有別的欄位
<Click: {x: Int, y: Int}, Key: {code: Text}, ...R>   # 開放和型別
(A, B) -> C                           # 多參數函數
A -> C                                # 單參數可省略參數括號
(A, B) -> {IO, Except[E]} C           # 效應列在箭頭與結果之間
once File                             # Typed：線性
```

- 類型參數用 `[]`：`Map[Text, Int]`。
- 效應列用 `{}`，與紀錄同形但出現在箭頭位置：`-> {IO} Text`。
- 無效應就不要寫 `{}`：`-> Int`。
- 高階種類：`trait Functor[F[_]]`。
- 約束寫在參數上：`def sort[A: Ord](xs: List[A]) -> List[A]:`。多約束：`A: Ord + Show`。
- 函數類型不 curry：`(Int, Int) -> Int` 不是 `Int -> Int -> Int`。後者仍合法，意思是「回傳函數」。

---

## 6. 定義

### 函數

```
def add(x: Int, y: Int) -> Int:
    """兩整數之和。"""
    x + y

def greet(name: Text) -> {IO} ():
    println(f"hello, {name}")

pub def map[A, B](xs: List[A], f: A -> B) -> List[B]:
    match xs:
        case []:
            []
        case [x, *rest]:
            [f(x), *map(rest, f)]
```

- 參數類型可省略（推導）；回傳類型在公開 API 應寫。
- 區塊最後一個運算式是回傳值。不必 `return`。
- `return e` 允許提前離開，脫糖成 `if`。
- `pub` 匯出。預設私有。
- `total def ...` 是 Typed：必須窮盡且終止。

### 值

```
title: Text = "H2O"
origin = {x: 0, y: 0}
```

同一區塊可陰影（順序繫結，像 SSA），不可突變既有繫結：

```
n = 1
n = n + 1      # 新繫結，合法
```

沒有 `+=`、沒有迴圈累加變數。要累加用 `foldl`／遞迴／`State`。

### 區域函數

```
def hypot(x: Float, y: Float) -> Float:
    def sq(z):
        z * z
    sqrt(sq(x) + sq(y))
```

沒有 `let ... in`、沒有 `where`。

---

## 7. 運算式

### 呼叫、管線、洞

```
add(1, 2)
f()                    # 零參呼叫；常數是 None 不是 None()

users
    |> filter(_.active)
    |> map(_.name)
    |> join(", ")

add(1, _)              # 部分套用：(y: Int) -> Int
_ + 1                  # fn(x): x + 1
_.name                 # fn(p): p.name
```

規則（只有這一套）：

- `x |> f()` = `f(x)`。
- `x |> f(a, b)` = `f(x, a, b)`。
- 管線右側必須是呼叫。`x |> f` 不合法；寫 `x |> f()`。
- `.` 不是呼叫：`xs.filter(f)` 是錯誤（`List` 沒有欄位 `filter`）。寫 `filter(xs, f)` 或 `xs |> filter(f)`。
- `_` 在運算式中從左到右各代表一個參數。`(_ + _)` 是 `(x, y) -> x + y`。
- 沒有自動 curry：`add(1)` 若 `add` 要兩參數，是類型錯誤。寫 `add(1, _)`。

### lambda

```
fn(x): x + 1
fn(x, y): x + y
fn(x):
    y = x * 2
    y + 1
```

最後一個參數若是 `fn`，可寫成區塊，省一層括號：

```
xs |> map fn(x):
    x + 1

# 等價於
xs |> map(fn(x): x + 1)
```

### 條件（運算式；條件上沒有括號）

```
if user_is_active and data_is_ready:
    start()

sign = if x > 0:
    1
elif x == 0:
    0
else:
    -1
```

需要值的 `if` 必須有 `else`。比較可鏈：`0 <= x < 10`。

### match

```
def first[A](xs: List[A]) -> Maybe[A]:
    match xs:
        case []:
            None
        case [x, *_]:
            Some(x)
```

窮盡檢查。`case _:` 萬用。守衛：`case n if n < 0:`。或模式：`case 0 | 1:`。

### for 與 comprehension

```
for name in names:
    println(name)          # 效應版 traverse；型別 {IO} ()

squares = [x * x for x in xs if x > 0]
```

沒有可變的 `while`。重複用遞迴、`Stream`、或效應迴圈函式 `repeat`。

### try / with（語法糖，不是額外計算模型）

```
n = try:
    parse_int(s)
except ParseError as e:
    0
# 脫糖為 handle Except

with io.open(path) as f:
    read_all(f)
# 脫糖為線性／資源處理器；離開區塊即關閉。`.` 不是方法。
```

---

## 8. 模式

```
None
Some(x)
Ok(v)
Err(e)
[x, y, *rest]
[]
{name, age}                 # 紀錄欄位解構（同名繫結）
{name: n, age: 2, ...}      # 其餘欄位忽略
(x, y)
0
"yes"
_
n if n > 0
```

列表不用 `x :: xs`。`:` 已用於區塊與類型。寫 `[x, *xs]`。

---

## 9. 資料型別

### ADT

```
type Maybe[A]:
    None
    Some(A)

type Result[A, E]:
    Ok(A)
    Err(E)

type List[A]:
    Nil
    Cons(A, List[A])
```

零參建構子不加 `()`：`None`、`Nil`。帶參：`Some(1)`。

公開類型的建構子預設公開。不透明：

```
pub opaque type UserId:
    Mk(Int)
```

只有定義模組能用 `Mk`／解構。

別名與 newtype：

```
type Name = Text
type UserId = new Int          # 與 Int 不同型；無額外配置
```

### 紀錄（列）

```
p: {name: Text, age: Int} = {name: "Ada", age: 36}
p.name
p.age

grown = {p..., age: p.age + 1}

# 巢狀
moved = update p:
    age = 37
    profile.city = "Paris"
```

- `{name: "Ada"}` 是紀錄（識別字鍵）。
- `{"name": "Ada"}` 是 `Map[Text, _]`。
- `{1, 2, 3}` 是 `Set`。`{}` 是空紀錄。空集合寫 `Set.empty`。
- 沒有頂層欄位選擇器函數。`.` 只做投影與模組路徑，不做方法鏈。
- 列多型：

```
def name_of[R](p: {name: Text, ...R}) -> Text:
    p.name
```

### 開放和型別（列的對偶）

```
def handle_event[R](e: <Click: {x: Int, y: Int}, Key: {code: Text}, ...R>) -> Text:
    match e:
        case Click(c):
            f"click {c.x},{c.y}"
        case Key(k):
            k.code
        case _:
            "other"
```

不必先封死全部建構子。封閉 ADT 仍在，領域模型優先用封閉。

---

## 10. trait 與 impl

```
trait Eq[A]:
    def eq(x: A, y: A) -> Bool

    def neq(x: A, y: A) -> Bool:
        not eq(x, y)

impl Eq[Int]:
    def eq(x, y):
        prim.int_eq(x, y)

def contains[A: Eq](xs: List[A], x: A) -> Bool:
    any(xs, _ == x)
```

- `x == y` 脫糖為 `Eq.eq(x, y)`。
- `impl` 必須與類型或 `trait` 同模組。orphan 要顯式：`impl orphan Eq[TheirType]:`（審計點）。
- `Functor`／`Applicative`／`Monad` 仍存在，但是效應的**編碼**，不是效應的寫法。日常寫效應列。

```
trait Functor[F[_]]:
    def map[A, B](fa: F[A], f: A -> B) -> F[B]
```

---

## 11. 效應與處理器

```
effect State[S]:
    def get() -> S
    def put(s: S) -> ()

effect Except[E]:
    def throw(e: E) -> Never

def count_line(path: Text) -> {State[Int], Except[Text], IO} Int:
    n = State.get()
    txt = io.read_file(path)
    if txt == "":
        Except.throw("empty file")
    State.put(n + 1)
    n + 1
```

函數體裡，RHS 的效應若已出現在回傳列中，`=` 就是執行（bind）。純值同樣用 `=`。沒有 `<-`、沒有 `do`。

換處理器、不改業務：

```
def test_count() -> Result[Int, Text]:
    handle count_line("x.txt") with:
        IO = fake_io({"x.txt": "hello"})
        State = State.run(0)
        Except = Except.to_result

def main() -> {IO} ():
    handle count_line("x.txt") with:
        State = State.run(0)
        Except = Except.to_io
    |> println()
```

`IO` 只是一個處理器。測試可整列換掉。

結構化並發是效應，不是 `forkIO`：

```
def both() -> {IO, Conc} (Text, Text):
    conc.together(
        fn(): fetch("/a"),
        fn(): fetch("/b"),
    )
```

父作用域結束則子任務取消。

---

## 12. 模組

一個檔案一個模組。檔名小寫：`list.h2o`、`io.h2o`。

```
# user.h2o
from list import List
from maybe import Maybe
import io

pub type User = {name: Text, age: Int}

pub def adult(u: User) -> Bool:
    u.age >= 18
```

```
from user import User, adult
import user
user.adult(u)
```

- 介面與實作可拆成 `user.h2o`（簽名）+ `user.impl.h2o`（實作）。不是必須，大模組才拆。
- 建構子可見性見第 9 節。不靠 export list 慣例假裝私有。

---

## 13. Prelude（安全、嚴格、文字）

自動在範圍內（節錄）：

| 名稱 | 角色 |
|---|---|
| `Int` `Float` `Bool` `Text` `Bytes` `Char` | 基礎 |
| `List` `Maybe` `Result` `NonEmpty` `Stream` | 容器／和型別 |
| `True` `False` `None` `Some` `Ok` `Err` | 建構子 |
| `map` `filter` `foldl` `any` `join` | 資料在前 |
| `range` | 與 Python 相同：`range(n)`、`range(a, b)` 不含尾 |

**不在 Prelude**：`head`、`tail`、`xs[i]` 當總函數、`read`、`String`＝`[Char]`。

```
xs[i]          # Maybe[A]，越界是 None
xs[1:3]        # List[A]，越界裁切（像 Python）
head(xs)       # 僅 NonEmpty；List 用 Maybe
```

`Stream` 才惰性。`List` 嚴格。

---

## 14. Typed 層只加註解，不加方言

```
total def map[A, B](xs: List[A], f: A -> B) -> List[B]:
    match xs:
        case []: []
        case [x, *rest]: [f(x), *map(rest, f)]

def close(f: once File) -> {IO} ():
    ...
```

同樣的縮排與 `def`。沒有新的括號文化，沒有 pragma。

---

## 15. 優先序（高 → 低）

1. 原子：字面、名稱、`()` `[]` `{}` 字串
2. `.` 欄位／模組，呼叫 `()`，索引 `[]`
3. 單目 `not` `-`
4. `**`（右結合）
5. `* / // %`
6. `+ -`
7. `in` `not in`
8. `== != < > <= >=`（可鏈）
9. `and`
10. `or`
11. `|>`（左結合）
12. `fn` / `if` / `match` / `handle` / `try` / `with` / `update`

`=` 不是運算子，是語句。

---

## 16. 明確拒絕的 Haskell 表面

| Haskell | H2O | 理由 |
|---|---|---|
| `f x y` | `f(x, y)` | 消滅 `f x + 1` |
| `f $ g $ h x` | `x \|> h() \|> g() \|> f()` | `$` 是可讀性債；也不用方法鏈 |
| `g . f` | `x \|> f \|> g` | `.` 留給欄位 |
| `x :: Int` | `x: Int` | Python 註解 |
| `x:xs` | `[x, *xs]` | `:` 留給區塊 |
| `do` / `<-` | 縮排 + `=` | 效應體看起來像普通程式 |
| `where` / `let in` | 區域 `def` 與 `=` | 一種區塊規則 |
| `class` / `instance` | `trait` / `impl` | 不與 OOP 搶詞 |
| `Just` / `Nothing` / `Left` | `Some` / `None` / `Err` | 日常詞 |
| `putStrLn` | `println` | 日常詞 |
| `>>=` `<$>` `<*>` | 效應列或 `map` | 不把運算子當架構 |
| `{foo = bar}` 選擇器 | `p.foo`，無頂層選擇器 | 不污染命名空間 |
| `LANGUAGE` pragma | 層（Core／Typed／Systems） | 見分層報告 |

---

## 17. 迷你文法（Core 子集）

詞彙層另產 `INDENT` / `DEDENT` / `NEWLINE`（與 Python 相同的 off-side）。

```
module      = { import | defn }

import      = "from" name "import" names
            | "import" name

defn        = ["pub"] "def" name [gens] "(" params ")" [ "->" type ] ":" block
            | ["pub"] ["opaque"] "type" name [gens] ( "=" type | ":" adt )
            | ["pub"] "trait" name [gens] ":" trait_body
            | "impl" name gens ":" impl_body
            | ["pub"] name [ ":" type ] "=" expr

gens        = "[" gen { "," gen } "]"
gen         = name [ ":" bounds ]

params      = [ param { "," param } ]
param       = name [ ":" type ]

block       = NEWLINE INDENT { stmt } DEDENT
stmt        = defn | "return" expr NEWLINE
            | "for" name "in" expr ":" block
            | expr NEWLINE

expr        = pipe
pipe        = logic { "|>" app }     # 右側必須是呼叫：f() 或 f(a)
logic       = not { "and" not | "or" not }
not         = "not" not | cmp
cmp         = arith { ("=="|"!="|"<"|"..."|"in") arith }
arith       = term { ("+"|"-") term }
term        = app { ("*"|"/"|"//"|"%") app }
app         = primary { call | index | "." name }
call        = "(" args ")"
index       = "[" expr [ ":" expr ] "]"
primary     = name | literal | list | record | tuple
            | "fn" "(" params ")" [ ":" type ] ( ":" expr | ":" block )
            | "if" expr ":" block { "elif" expr ":" block } [ "else" ":" block ]
            | "match" expr ":" NEWLINE INDENT { "case" pat [ "if" expr ] ":" block } DEDENT
            | "handle" expr "with" ":" block
            | "update" expr ":" block
            | "(" expr ")"

pat         = name | literal | "_" | list_pat | rec_pat
            | name "(" pats ")" | "[" pats "]" | "{" rec_fields "}"
```

這不是完整 PEG；實作時以本節加上文規則為準，衝突時以第 1、4 節的決策為準。

---

## Haskell → H2O

```
-- Haskell
putStrLn $ "hi " ++ name
map (+1) . filter even $ xs
case xs of
  []     -> 0
  (x:ys) -> x + sum ys
do
  n <- readIO
  print (n + 1)
```

```
# H2O
println(f"hi {name}")
xs |> filter(even) |> map(_ + 1)
match xs:
    case []:
        0
    case [x, *ys]:
        x + sum(ys)
n = io.read_int()
println(n + 1)
```
