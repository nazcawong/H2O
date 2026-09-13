# H2O 語法速查

```
# 4 空格縮排。and / or / not。沒有 ; 沒有區塊 {}。
from list import List
import io

title: Text = "H2O"

def add(x: Int, y: Int) -> Int:
    x + y

add(1, 2)

if user_is_active and data_is_ready:
    start()

users
    |> filter(_.active)
    |> map(_.name)
    |> join(", ")

names = [u.name for u in users if u.active]

match xs:
    case []: 0
    case [n, *rest]: n + sum(rest)

type Maybe[A]:
    None
    Some(A)

p = {name: "Ada", age: 36}
p.name                         # . 只做欄位／模組
{p..., age: 37}

trait Eq[A]:
    def eq(x: A, y: A) -> Bool

def load(path: Text) -> {IO, Except[Text]} Text:
    txt = io.read_file(path)
    if txt == "":
        Except.throw("empty")
    txt
```

## 一種寫法

| 做 | 寫 | 不寫 |
|---|---|---|
| 條件 | `if a and b:` | `if (a && b) {` |
| 呼叫 | `f(x)` | `f x` |
| 欄位 | `p.name` | `xs.map(f)` |
| 多步 | `xs \|> f() \|> g()` | `$`、`.` 合成 |
| 一步列表 | `[e for x in xs if p]` | 與管線混用當風格 |

`h2ofmt` 是唯一格式。詳見 [11-readability.md](11-readability.md)。
