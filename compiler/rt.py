#!/usr/bin/env python3
import sys
import threading
sys.setrecursionlimit(100000)

_HNone = ("None",)

def Some(x):
    return ("Some", x)
def Ok(x):
    return ("Ok", x)
def Err(e):
    return ("Err", e)
def NonEmpty(x, xs):
    return ("NonEmpty", x, xs)

class _ExceptThrown(Exception):
    def __init__(self, value):
        self.value = value

class _StateH:
    def __init__(self, v):
        self.cell = [v]
    def get(self):
        return self.cell[0]
    def put(self, v):
        self.cell[0] = v

class _ExceptH:
    def __init__(self, mode):
        self.mode = mode
    def throw(self, e):
        raise _ExceptThrown(e)

class _IOH:
    def __init__(self, files=None, stdout=None):
        self.files = files
        self.stdout = stdout
    def read_file(self, path):
        if self.files is not None:
            if path not in self.files:
                raise _ExceptThrown("no such file " + str(path))
            return self.files[path]
        return open(path, encoding="utf-8").read()
    def write_file(self, path, s):
        if self.files is not None:
            self.files[path] = s
            return
        open(path, "w", encoding="utf-8").write(s)
    def println(self, x):
        text = x if isinstance(x, str) else str(x)
        if self.stdout is None:
            print(text)
        else:
            self.stdout.write(text + "\n")
    def open(self, path):
        return {"path": path, "open": True}

_STACK = []
def _cur():
    return _STACK[-1] if _STACK else {}
def _push(h):
    _STACK.append(h)
def _pop():
    if _STACK:
        _STACK.pop()

def fake_io(path, text):
    return _IOH({path: text})

def println(x):
    io = _cur().get("IO")
    if io is not None:
        return io.println(x)
    print(x if isinstance(x, str) else str(x))

def map(xs, f):
    return [f(x) for x in xs]
def filter(xs, f):
    return [x for x in xs if f(x)]
def join(xs, sep):
    return sep.join(xs)
def foldl(xs, acc, f):
    for x in xs:
        acc = f(acc, x)
    return acc
def any(xs, f):
    for x in xs:
        if f(x):
            return True
    return False
def range(*a):
    return list(__import__("builtins").range(*a))
def head(ne):
    return ne[1]
def nonempty(xs):
    return _HNone if not xs else ("Some", ("NonEmpty", xs[0], xs[1:]))
def chars(s):
    return [c for c in s]
def text_len(s):
    return len(s)
def slice_text(s, i, j):
    return s[i:j]
def starts_with(s, p):
    return s.startswith(p)
def cat(a, b):
    return a + b
def int_to_text(n):
    return str(n)
def list_len(xs):
    return len(xs)
def die(msg):
    print(msg, file=sys.stderr)
    raise SystemExit(1)
def argv():
    return sys.argv
def eprint(s):
    sys.stderr.write(s)
    sys.stderr.flush()
def exit(n):
    raise SystemExit(n)
def gc():
    return 0
def write_file(path, s):
    io = _cur().get("IO")
    if io is not None and hasattr(io, "write_file"):
        return io.write_file(path, s)
    open(path, "w", encoding="utf-8").write(s)
def read_file(path):
    io = _cur().get("IO")
    if io is not None:
        return io.read_file(path)
    return open(path, encoding="utf-8").read()

def State_get():
    h = _cur().get("State")
    if h is None:
        raise Exception("沒有 State 處理器")
    return h.get()
def State_put(v):
    h = _cur().get("State")
    if h is None:
        raise Exception("沒有 State 處理器")
    h.put(v)
def State_run(v):
    return _StateH(v)
def Except_throw(e):
    h = _cur().get("Except")
    if h is None:
        raise _ExceptThrown(e)
    return h.throw(e)

class _Ns:
    def __init__(self, **kw):
        self.__dict__.update(kw)

io = _Ns(read_file=read_file, write_file=write_file, println=println, open=lambda p: {"path": p, "open": True})
State = _Ns(get=State_get, put=State_put, run=State_run)
Except = _Ns(throw=Except_throw, to_result=_ExceptH("result"), to_io=_ExceptH("io"))
IO = _Ns()

class _Stream:
    def __init__(self, it):
        self.it = it
def stream_iterate(start, f):
    def gen():
        x = start
        while True:
            yield x
            x = f(x)
    return _Stream(gen())
def stream_take(s, n):
    out = []
    g = s.it if isinstance(s, _Stream) else iter(s)
    for i, x in zip(__import__("builtins").range(n), g):
        out.append(x)
    return out
def stream_map(s, f):
    def gen():
        g = s.it if isinstance(s, _Stream) else iter(s)
        for x in g:
            yield f(x)
    return _Stream(gen())
Stream = _Ns(iterate=stream_iterate, take=stream_take, map=stream_map)

def together(f1, f2):
    box = [None, None]
    err = []
    def run(i, fn):
        try:
            box[i] = fn()
        except Exception as ex:
            err.append(ex)
    t1 = threading.Thread(target=run, args=(0, f1))
    t2 = threading.Thread(target=run, args=(1, f2))
    t1.start(); t2.start(); t1.join(); t2.join()
    if err:
        raise err[0]
    return (box[0], box[1])
conc = _Ns(together=together)

def ffi_call(lib, name, *args):
    import ctypes
    so = ctypes.CDLL(None) if lib == "c" else ctypes.CDLL(lib)
    fn = getattr(so, name)
    fn.restype = ctypes.c_int
    coerced = [int(a) if isinstance(a, (int, bool)) else a for a in args]
    return int(fn(*coerced))
def vect(xs):
    return ("Vect", len(xs), list(xs))
def vect_len(v):
    return v[1]
def vect_xs(v):
    return v[2]
def vect_cons(x, v):
    return ("Vect", v[1] + 1, [x] + list(v[2]))
def vect_head(v):
    return v[2][0]
def chmod_x(path):
    import os
    os.chmod(path, 0o755)
prim = _Ns(int_eq=lambda x, y: x == y, bool_eq=lambda x, y: x == y, text_eq=lambda x, y: x == y, ffi_call=ffi_call)
