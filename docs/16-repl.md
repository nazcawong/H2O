# REPL

日期：2026-09-12。Core 1 凍結，不開語法。模式切換只活在 REPL 裡，不是語言的一部分。

Julia 用提示切四個 mode。H2O 是整行讀入（沒有 keymap／Backspace 改提示），所以：

- 空提示打 `?`／`;`／`]` 進入該 mode（下一輪提示換成 `help?>`／`shell>`／`pkg>`）
- 同一行 `? map`、`; ls` 是一次性，不留下 mode
- 離開 mode：空行（Julia 是提示開頭按 Backspace）

```
h2o> 1 + 2
3
h2o> ?
H2O 入門（help?> 模式）
...
help?> 1
第 1 課  REPL 與第一個運算
...
help?> map
List[p], (p -> q) -> List[q]
help?>
h2o> ; echo hi
hi
h2o>
```

`?` 印入門目錄（`docs/learn/0.txt`）並進入 `help?>`。數字 `0`–`11`、`intro`／`tutorial`／`index`／`help` 印對應課文；`trait`／`impl`／`Eq`／`Functor` 進第 9 課；`pkg`／`module`／`gc` 進第 10 課；`once`／`Vect`／`wasm`／`native` 進第 11 課；`modes` 印四個提示的說明。其他查詢（`map`、`1 + 2`）仍是類型。`? 1`、`? 11`、`? map` 是一次性，不留下 mode。課文只寫現在能跑的語法。練習在空行回到 `h2o>` 之後做。

| Julia | H2O |
|---|---|
| `julia>` | `h2o>` 求值；縮排區塊空行結束 |
| `?` → `help?>` | 入門課文 `docs/learn/0.txt`–`11.txt`；非課碼查詢印**類型** |
| `;` → `shell>` | POSIX `sh -c` |
| `]` → `pkg>` | **沒有套件註冊中心**；`h2o vendor` 把 git 樹變成目錄；`:load path/` 載入 `path/mod.h2o` |
| Backspace 回 Julian | 空行回 `h2o>` |
| `ans`、tab、歷史搜尋 | 不做 |

`h2o>` 裡仍可 `:t e`、`:load f`、`:quit`。無參數的 `./h2o` 進 REPL。課文路徑相對工作目錄，請在倉庫根目錄開 REPL。
