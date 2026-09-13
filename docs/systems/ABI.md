# H2O FFI ABI 草案 0

日期：2026-09-13。**不是**工業穩定 ABI。破壞性變更不開 pragma，只改這份草案的版本號。

執行期值是帶標籤的 `Val`（見 `compiler/rt.c`）。C 邊界只認草案裡列出的符號，不認任意 `dlsym`。

## `Val` 標籤（核）

| `k` | 含義 | C 這邊現在怎麼過 |
|---|---|---|
| `K_I` | 整數 `long i` | 本草案：`int`（截斷） |
| `K_T` | 文字 `char *s` | **還沒過界**。將來是 NUL 結尾位元組，生命週期＝這次呼叫 |
| `K_B` | 布林 | 還沒過界 |
| `K_U` | `()` | 還沒過界 |
| 其餘 | List／紀錄／建構子／閉包 | 不出現在 FFI 參數 |

H2O 的 `Int` 對應 `K_I`。不要把 `Val*` 直接當穩定佈局；欄位順序與填充可以變。

## 庫與符號

`prim.ffi_call(lib, name, arg)`：

- `lib`、`name` 必須是**字面** `Text`。變數當名字＝`h2o check` 紅。
- 今天只認 `lib == "c"`（行程裡的 libc，`dlopen(NULL)`）。
- 今天只認：

| 符號 | H2O | C |
|---|---|---|
| `abs` | `Int -> Int` | `int abs(int)` |
| `labs` | `Int -> Int` | `long labs(long)`（參數當 `int` 傳） |

不在表裡＝`h2o check` 紅（「沒有 ffi c.foo」）。`--no-check` 執行期也不呼叫未知符號。

包裝是普通 `def`，沒有新語法：

```
def c_abs(x: Int) -> Int:
    prim.ffi_call("c", "abs", x)
```

`prim.ffi_call("c", "abs", "no")` 是類型錯誤。

## 呼叫慣例（本切片）

1. 檢查器對上表之後，執行期把第三個 `Val` 的 `i` 轉成 `int`。
2. 呼叫 C。
3. 把 `int` 回傳值放進 `K_I`。

沒有 struct、沒有 callback、沒有多參數、沒有 H2O 閉包進 C。WASI 映像沒有 `dlfcn`：已知符號走內建 `abs`，行為與宿主相同（符號仍須在表裡）。

## 明確不是

穩定 ABI、Soname、Windows 呼叫慣例、Wasm 的 C-ABI 匯出、把編譯器用 FFI 重寫。那些要另開刀，並先改這份草案。
