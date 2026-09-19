/* H2O runtime kernel: tagged values, handlers, FFI, conc, bytecode VM. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <math.h>
#ifndef __wasi__
#include <setjmp.h>
#endif
#if !defined(__wasi__) && !defined(_WIN32)
#include <unistd.h>
#include <termios.h>
#include <dirent.h>
#include <errno.h>
#endif
#if defined(_WIN32) || defined(__wasi__)
#else
#include <dlfcn.h>
#include <pthread.h>
#endif

typedef struct Val Val;
typedef struct { int n; Val *xs; } List;
typedef struct { int n; char **ks; Val *vs; } Rec;
typedef struct { const char *tag; int n; Val *xs; } Ctor;
struct Val {
    int k; long i; char *s; List *l; Rec *r; Ctor *c;
    Val (*fn1)(Val);
    int fn_id; int ncap; Val *caps;
    double x;
};
enum { K_I=1, K_B=2, K_T=3, K_L=4, K_R=5, K_U=6, K_F=7, K_C=8, K_D=9 };
enum { GK_LIST=1, GK_REC, GK_CTOR, GK_STR, GK_VALS, GK_KEYS };
#define GC_N 8192
typedef struct GEnt {
    struct GEnt *next, *link;
    void *p;
    unsigned char kind, mark;
    size_t sz;
} GEnt;
static GEnt *gc_buck[GC_N], *gc_all;
static size_t gc_bytes, gc_nobj, gc_lim;
static int gc_busy;
static unsigned gc_hash(void *p){ return (unsigned)(((uintptr_t)p >> 3) & (GC_N-1)); }
static GEnt *gc_find(void *p){
    if(!p) return 0;
    for(GEnt *e=gc_buck[gc_hash(p)]; e; e=e->next) if(e->p==p) return e;
    return 0;
}
static void gc_mark_val(Val v);
static void gc_mark_ptr(void *p);
static void gc_collect(void);
static void gc_maybe(void){
    if(gc_busy) return;
    if(!gc_lim) gc_lim=512u*1024u;
    if(gc_bytes>gc_lim) gc_collect();
}
static void gc_reg(void *p, int kind, size_t n){
    if(!p) return;
    GEnt *e=malloc(sizeof *e);
    e->p=p; e->kind=(unsigned char)kind; e->mark=0; e->sz=n;
    unsigned h=gc_hash(p);
    e->next=gc_buck[h]; gc_buck[h]=e;
    e->link=gc_all; gc_all=e;
    gc_bytes+=n; gc_nobj++;
}
static void *gc_malloc(size_t n, int kind){
    void *p=calloc(1, n?n:1);
    gc_reg(p, kind, n);
    return p;
}
static void *gc_realloc(void *p, size_t n, int kind){
    if(!p) return gc_malloc(n, kind);
    GEnt *e=gc_find(p);
    void *q=realloc(p, n?n:1);
    if(!q) return 0;
    if(e){
        if(q!=p){
            unsigned h=gc_hash(e->p);
            GEnt **pp=&gc_buck[h];
            while(*pp && *pp!=e) pp=&(*pp)->next;
            if(*pp) *pp=e->next;
            gc_bytes-=e->sz;
            e->p=q; e->sz=n; gc_bytes+=n;
            h=gc_hash(q);
            e->next=gc_buck[h]; gc_buck[h]=e;
        } else {
            gc_bytes=gc_bytes-e->sz+n;
            e->sz=n;
        }
    } else gc_reg(q, kind, n);
    return q;
}
static char *gc_strdup(const char *s){
    size_t n=strlen(s?s:"")+1;
    char *p=gc_malloc(n, GK_STR);
    memcpy(p, s?s:"", n);
    return p;
}
static Val *gc_vals(int n){
    if(n<=0) return 0;
    return gc_malloc(sizeof(Val)*(size_t)n, GK_VALS);
}
static void gc_mark_ent(GEnt *e){
    if(!e || e->mark) return;
    e->mark=1;
    if(e->kind==GK_LIST){
        List *l=e->p;
        if(l->xs) gc_mark_ptr(l->xs);
        for(int i=0;i<l->n;i++) gc_mark_val(l->xs[i]);
    } else if(e->kind==GK_REC){
        Rec *r=e->p;
        if(r->vs) gc_mark_ptr(r->vs);
        if(r->ks) gc_mark_ptr(r->ks);
        for(int i=0;i<r->n;i++){
            if(r->ks[i]) gc_mark_ptr(r->ks[i]);
            gc_mark_val(r->vs[i]);
        }
    } else if(e->kind==GK_CTOR){
        Ctor *c=e->p;
        if(c->xs) gc_mark_ptr(c->xs);
        for(int i=0;i<c->n;i++) gc_mark_val(c->xs[i]);
    }
}
static void gc_mark_ptr(void *p){
    if(p) gc_mark_ent(gc_find(p));
}
static void gc_mark_val(Val v){
    if(v.k==K_T) gc_mark_ptr(v.s);
    else if(v.k==K_L) gc_mark_ptr(v.l);
    else if(v.k==K_R) gc_mark_ptr(v.r);
    else if(v.k==K_C) gc_mark_ptr(v.c);
    else if(v.k==K_F && v.caps){
        gc_mark_ptr(v.caps);
        for(int i=0;i<v.ncap;i++) gc_mark_val(v.caps[i]);
    }
}

static Val V_U(void){ Val v; memset(&v,0,sizeof v); v.k=K_U; return v; }
static Val V_I(long x){ Val v=V_U(); v.k=K_I; v.i=x; return v; }
static Val V_D(double x){ Val v=V_U(); v.k=K_D; v.x=x; return v; }
static Val V_B(int x){ Val v=V_U(); v.k=K_B; v.i=x; return v; }
static Val V_T(const char *s){ Val v=V_U(); v.k=K_T; v.s=gc_strdup(s?s:""); return v; }
static Val V_L(List *l){ Val v=V_U(); v.k=K_L; v.l=l; return v; }
static Val V_R(Rec *r){ Val v=V_U(); v.k=K_R; v.r=r; return v; }
static Val V_F(Val (*f)(Val)){ Val v=V_U(); v.k=K_F; v.fn1=f; return v; }
static Val V_CTOR(const char *tag, int n, Val *xs){
    Val v=V_U(); v.k=K_C; v.c=gc_malloc(sizeof(Ctor), GK_CTOR);
    v.c->tag=tag?tag:""; v.c->n=n; v.c->xs=xs; return v;
}

#define H2O_HIST_MAX 500
#define H2O_LINE_MAX 4096
static char *h2o_hist[H2O_HIST_MAX];
static int h2o_hist_n, h2o_hist_loaded;
static char h2o_prompt[128];
static char **h2o_sess;
static int h2o_sess_n;
#if !defined(__wasi__) && !defined(_WIN32)
static struct termios h2o_told;
static int h2o_raw;
static void h2o_tty_restore(void){
    if(h2o_raw){ tcsetattr(0, TCSANOW, &h2o_told); h2o_raw=0; }
}
static int h2o_tty_raw(void){
    if(tcgetattr(0, &h2o_told)) return -1;
    struct termios t=h2o_told;
    t.c_lflag &= (tcflag_t)~(ICANON|ECHO|ISIG);
    t.c_cc[VMIN]=1; t.c_cc[VTIME]=0;
    if(tcsetattr(0, TCSANOW, &t)) return -1;
    h2o_raw=1; return 0;
}
#endif
static void h2o_hist_path(char *d, size_t dn, char *f, size_t fn){
    const char *h=getenv("HOME");
    if(!h || !h[0]){ d[0]=0; f[0]=0; return; }
    snprintf(d, dn, "%s/.h2o", h);
    snprintf(f, fn, "%s/.h2o/history", h);
}
static void h2o_hist_load(void){
    if(h2o_hist_loaded) return;
    h2o_hist_loaded=1;
    char dir[512], path[512];
    h2o_hist_path(dir, sizeof dir, path, sizeof path);
    if(!path[0]) return;
    FILE *fp=fopen(path, "r");
    if(!fp) return;
    char line[H2O_LINE_MAX];
    while(h2o_hist_n<H2O_HIST_MAX && fgets(line, sizeof line, fp)){
        size_t n=strlen(line);
        while(n && (line[n-1]=='\n' || line[n-1]=='\r')) line[--n]=0;
        if(!n) continue;
        h2o_hist[h2o_hist_n]=malloc(n+1);
        if(!h2o_hist[h2o_hist_n]) break;
        memcpy(h2o_hist[h2o_hist_n], line, n+1);
        h2o_hist_n++;
    }
    fclose(fp);
}
static void h2o_hist_save(void){
    char dir[512], path[512];
    h2o_hist_path(dir, sizeof dir, path, sizeof path);
    if(!path[0]) return;
    if(dir[0]) mkdir(dir, 0700);
    FILE *fp=fopen(path, "w");
    if(!fp) return;
    for(int i=0;i<h2o_hist_n;i++) if(h2o_hist[i]) fprintf(fp, "%s\n", h2o_hist[i]);
    fclose(fp);
#if !defined(__wasi__) && !defined(_WIN32)
    chmod(path, 0600);
#endif
}
static void h2o_hist_add(const char *s){
    if(!s || !s[0]) return;
    if(h2o_hist_n>0 && h2o_hist[h2o_hist_n-1] && strcmp(h2o_hist[h2o_hist_n-1], s)==0) return;
    char *c=malloc(strlen(s)+1);
    if(!c) return;
    strcpy(c, s);
    if(h2o_hist_n==H2O_HIST_MAX){
        free(h2o_hist[0]);
        memmove(h2o_hist, h2o_hist+1, (H2O_HIST_MAX-1)*sizeof(char*));
        h2o_hist_n--;
    }
    h2o_hist[h2o_hist_n++]=c;
    h2o_hist_save();
}
static void h2o_line_draw(int oldc, int oldn, const char *buf, int n, int cur){
    int i;
    for(i=0;i<oldc;i++) fputc('\b', stdout);
    if(n) fwrite(buf, 1, (size_t)n, stdout);
    int extra=oldn-n;
    if(extra<0) extra=0;
    for(i=0;i<extra;i++) fputc(' ', stdout);
    for(i=0;i<extra;i++) fputc('\b', stdout);
    for(i=0;i<n-cur;i++) fputc('\b', stdout);
    fflush(stdout);
}
static int h2o_word_start(const char *buf, int cur){
    int i=cur;
    while(i>0 && buf[i-1]!=' ' && buf[i-1]!='\t') i--;
    return i;
}
static int h2o_pref_ok(const char *s, const char *p, int pn){
    return (int)strlen(s)>=pn && strncmp(s, p, (size_t)pn)==0;
}
static void h2o_cmp_add(char **xs, int *n, const char *s, const char *p, int pn){
    if(*n>=256) return;
    if(!h2o_pref_ok(s, p, pn)) return;
    for(int i=0;i<*n;i++) if(strcmp(xs[i], s)==0) return;
    xs[*n]=malloc(strlen(s)+1);
    if(!xs[*n]) return;
    strcpy(xs[*n], s);
    (*n)++;
}
static void h2o_cmp_paths(char **xs, int *n, const char *pref){
#if !defined(__wasi__) && !defined(_WIN32)
    char dir[1024], base[256];
    const char *sl=strrchr(pref, '/');
    if(sl){
        size_t dn=(size_t)(sl-pref);
        if(dn>=sizeof dir) return;
        memcpy(dir, pref, dn); dir[dn]=0;
        if(!dir[0]) strcpy(dir, "/");
        snprintf(base, sizeof base, "%s", sl+1);
    } else {
        strcpy(dir, ".");
        snprintf(base, sizeof base, "%s", pref);
    }
    DIR *d=opendir(dir);
    if(!d) return;
    struct dirent *e;
    while((e=readdir(d))){
        if(e->d_name[0]=='.' && (!base[0] || base[0]!='.')) continue;
        if(!h2o_pref_ok(e->d_name, base, (int)strlen(base))) continue;
        char show[1200];
        if(sl){
            if(dir[0]=='/' && !dir[1]) snprintf(show, sizeof show, "/%s", e->d_name);
            else snprintf(show, sizeof show, "%s/%s", dir, e->d_name);
        } else snprintf(show, sizeof show, "%s", e->d_name);
        char stpath[1200];
        if(sl) snprintf(stpath, sizeof stpath, "%s/%s", dir, e->d_name);
        else snprintf(stpath, sizeof stpath, "%s", e->d_name);
        struct stat st;
        if(stat(stpath, &st)==0 && S_ISDIR(st.st_mode)){
            size_t L=strlen(show);
            if(L+2<sizeof show){ show[L]='/'; show[L+1]=0; }
        }
        h2o_cmp_add(xs, n, show, pref, (int)strlen(pref));
    }
    closedir(d);
#else
    (void)xs; (void)n; (void)pref;
#endif
}
static const char *h2o_kw[]={
    "def","if","elif","else","match","case","type","trait","impl","fn",
    "pub","import","from","not","and","or","total","with","handle",
    "map","filter","foldl","join","println","cat","range","abs","isqrt","float","int","sqrt",
    ":t",":load",":quit",":q",":exit",
    0
};
static void h2o_complete(char *buf, int *n, int *cur){
    int ws, pn, i, m=0;
    char *xs[256];
    char pref[H2O_LINE_MAX];
    if(strncmp(buf, ":load ", 6)==0 || strncmp(buf, ":load\t", 6)==0){
        const char *p=buf+6;
        while(*p==' ') p++;
        h2o_cmp_paths(xs, &m, p);
        ws=(int)(p-buf);
        pn=*n-ws;
        if(pn<0) pn=0;
    } else {
        ws=h2o_word_start(buf, *cur);
        pn=*cur-ws;
        if(pn<0) pn=0;
        if(pn>=H2O_LINE_MAX) pn=H2O_LINE_MAX-1;
        memcpy(pref, buf+ws, (size_t)pn); pref[pn]=0;
        if(ws==0 && pref[0]==':'){
            for(i=0;h2o_kw[i];i++) if(h2o_kw[i][0]==':') h2o_cmp_add(xs, &m, h2o_kw[i], pref, pn);
        } else {
            for(i=0;h2o_kw[i];i++) if(h2o_kw[i][0]!=':') h2o_cmp_add(xs, &m, h2o_kw[i], pref, pn);
            for(i=0;i<h2o_sess_n;i++) h2o_cmp_add(xs, &m, h2o_sess[i], pref, pn);
        }
    }
    if(m==1){
        const char *s=xs[0];
        int sl=(int)strlen(s);
        int tail=*n-*cur;
        if(ws+sl+tail+1<H2O_LINE_MAX){
            memmove(buf+ws+sl, buf+*cur, (size_t)tail);
            memcpy(buf+ws, s, (size_t)sl);
            *n=ws+sl+tail;
            *cur=ws+sl;
            buf[*n]=0;
        }
    } else if(m>1){
        fputc('\n', stdout);
        for(i=0;i<m;i++){
            fputs(xs[i], stdout);
            fputc(i==m-1?'\n':' ', stdout);
        }
        fputs(h2o_prompt, stdout);
        fwrite(buf, 1, (size_t)*n, stdout);
        for(i=0;i<*n-*cur;i++) fputc('\b', stdout);
        fflush(stdout);
    }
    for(i=0;i<m;i++) free(xs[i]);
}
#if !defined(__wasi__) && !defined(_WIN32)
static int h2o_tty_line(char *buf, int cap){
    int n=0, cur=0, oldn=0, oldc=0, hist_i, saved_set=0;
    char saved[H2O_LINE_MAX];
    unsigned char c;
    buf[0]=0;
    hist_i=h2o_hist_n;
    h2o_hist_load();
    hist_i=h2o_hist_n;
    if(h2o_tty_raw()) return -1;
    for(;;){
        ssize_t r=read(0, &c, 1);
        if(r<=0){ h2o_tty_restore(); return 0; }
        if(c=='\r' || c=='\n'){
            fputc('\n', stdout); fflush(stdout);
            buf[n]=0;
            h2o_tty_restore();
            h2o_hist_add(buf);
            return 1;
        }
        if(c==3){ /* Ctrl-C: clear line, stay in REPL */
            h2o_line_draw(cur, n, "", 0, 0);
            n=0; cur=0; buf[0]=0; oldn=0; oldc=0;
            continue;
        }
        if(c==4){ /* Ctrl-D */
            if(n==0){ h2o_tty_restore(); return 0; }
            if(cur<n){ memmove(buf+cur, buf+cur+1, (size_t)(n-cur)); n--; buf[n]=0;
                h2o_line_draw(oldc, oldn, buf, n, cur); oldn=n; oldc=cur; }
            continue;
        }
        if(c==1){ cur=0; h2o_line_draw(oldc, oldn, buf, n, cur); oldc=cur; continue; } /* Ctrl-A */
        if(c==5){ cur=n; h2o_line_draw(oldc, oldn, buf, n, cur); oldc=cur; continue; } /* Ctrl-E */
        if(c==21){ /* Ctrl-U */
            memmove(buf, buf+cur, (size_t)(n-cur)+1);
            n-=cur; cur=0;
            h2o_line_draw(oldc, oldn, buf, n, cur); oldn=n; oldc=cur;
            continue;
        }
        if(c==127 || c==8){
            if(cur>0){
                memmove(buf+cur-1, buf+cur, (size_t)(n-cur)+1);
                cur--; n--;
                h2o_line_draw(oldc, oldn, buf, n, cur); oldn=n; oldc=cur;
            }
            continue;
        }
        if(c==9){ /* Tab */
            buf[n]=0;
            h2o_complete(buf, &n, &cur);
            h2o_line_draw(oldc, oldn, buf, n, cur); oldn=n; oldc=cur;
            continue;
        }
        if(c==27){
            unsigned char s[4];
            if(read(0, s, 1)!=1) continue;
            unsigned char k=s[0];
            if(k=='[' || k=='O'){
                if(read(0, s, 1)!=1) continue;
                k=s[0];
                if(k=='3'){ /* delete ~ */
                    unsigned char t;
                    read(0, &t, 1);
                    if(cur<n){
                        memmove(buf+cur, buf+cur+1, (size_t)(n-cur));
                        n--; buf[n]=0;
                        h2o_line_draw(oldc, oldn, buf, n, cur); oldn=n; oldc=cur;
                    }
                    continue;
                }
                if(k=='A'){ /* up */
                    if(!saved_set){ memcpy(saved, buf, (size_t)n); saved[n]=0; saved_set=1; }
                    if(hist_i>0){
                        hist_i--;
                        strncpy(buf, h2o_hist[hist_i]?h2o_hist[hist_i]:"", (size_t)cap-1);
                        buf[cap-1]=0; n=(int)strlen(buf); if(n>=cap) n=cap-1; cur=n;
                        h2o_line_draw(oldc, oldn, buf, n, cur); oldn=n; oldc=cur;
                    }
                    continue;
                }
                if(k=='B'){ /* down */
                    if(hist_i<h2o_hist_n) hist_i++;
                    if(hist_i>=h2o_hist_n){
                        if(saved_set){ strncpy(buf, saved, (size_t)cap-1); buf[cap-1]=0; n=(int)strlen(buf); }
                        else { n=0; buf[0]=0; }
                        cur=n;
                    } else {
                        strncpy(buf, h2o_hist[hist_i]?h2o_hist[hist_i]:"", (size_t)cap-1);
                        buf[cap-1]=0; n=(int)strlen(buf); cur=n;
                    }
                    h2o_line_draw(oldc, oldn, buf, n, cur); oldn=n; oldc=cur;
                    continue;
                }
                if(k=='C'){ if(cur<n) cur++; h2o_line_draw(oldc, oldn, buf, n, cur); oldc=cur; continue; }
                if(k=='D'){ if(cur>0) cur--; h2o_line_draw(oldc, oldn, buf, n, cur); oldc=cur; continue; }
                if(k=='H'){ cur=0; h2o_line_draw(oldc, oldn, buf, n, cur); oldc=cur; continue; }
                if(k=='F'){ cur=n; h2o_line_draw(oldc, oldn, buf, n, cur); oldc=cur; continue; }
            }
            continue;
        }
        if(c<32) continue;
        if(n+1>=cap) continue;
        memmove(buf+cur+1, buf+cur, (size_t)(n-cur));
        buf[cur]=(char)c; n++; cur++; buf[n]=0;
        h2o_line_draw(oldc, oldn, buf, n, cur); oldn=n; oldc=cur;
    }
}
#endif
static int h2o_read_line(char *buf, int cap){
    buf[0]=0;
#if !defined(__wasi__) && !defined(_WIN32)
    if(isatty(0)){
        int ok=h2o_tty_line(buf, cap);
        if(ok>=0) return ok;
    }
#endif
    if(!fgets(buf, cap, stdin)) return 0;
    size_t n=strlen(buf);
    if(n && buf[n-1]=='\n') buf[--n]=0;
    if(n && buf[n-1]=='\r') buf[--n]=0;
    return 1;
}
static void h2o_set_names(Val v){
    int i;
    for(i=0;i<h2o_sess_n;i++) free(h2o_sess[i]);
    free(h2o_sess); h2o_sess=0; h2o_sess_n=0;
    if(v.k!=K_L || !v.l) return;
    h2o_sess=calloc((size_t)v.l->n+1, sizeof(char*));
    if(!h2o_sess) return;
    for(i=0;i<v.l->n;i++){
        if(v.l->xs[i].k==K_T && v.l->xs[i].s){
            h2o_sess[h2o_sess_n]=malloc(strlen(v.l->xs[i].s)+1);
            if(h2o_sess[h2o_sess_n]){ strcpy(h2o_sess[h2o_sess_n], v.l->xs[i].s); h2o_sess_n++; }
        }
    }
}
static Val V_CLO(int fn_id, int ncap, Val *caps){
    Val v=V_U(); v.k=K_F; v.fn_id=fn_id; v.ncap=ncap; v.caps=caps; return v;
}
static List *lst_new(void){ return gc_malloc(sizeof(List), GK_LIST); }
static List *lst_add(List *l, Val x){
    l->xs=gc_realloc(l->xs,sizeof(Val)*(size_t)(l->n+1), GK_VALS); l->xs[l->n++]=x; return l;
}
static Rec *rec_new(void){ return gc_malloc(sizeof(Rec), GK_REC); }
static Rec *rec_add(Rec *r, const char *k, Val v){
    r->ks=gc_realloc(r->ks,sizeof(char*)*(size_t)(r->n+1), GK_KEYS);
    r->vs=gc_realloc(r->vs,sizeof(Val)*(size_t)(r->n+1), GK_VALS);
    r->ks[r->n]=gc_strdup(k); r->vs[r->n]=v; r->n++; return r;
}
static Val rec_get(Val o, const char *k){
    if(o.k!=K_R||!o.r) return V_U();
    for(int i=0;i<o.r->n;i++) if(strcmp(o.r->ks[i],k)==0) return o.r->vs[i];
    return V_U();
}
static int truth(Val v){
    if(v.k==K_B||v.k==K_I) return v.i!=0;
    if(v.k==K_D) return v.x!=0;
    if(v.k==K_T) return v.s && v.s[0];
    if(v.k==K_L) return v.l && v.l->n;
    if(v.k==K_U) return 0;
    return 1;
}
static int eqv(Val a, Val b){
    if(a.k!=b.k) return 0;
    if(a.k==K_D) return a.x==b.x;
    if(a.k==K_I||a.k==K_B) return a.i==b.i;
    if(a.k==K_T) return strcmp(a.s?a.s:"", b.s?b.s:"")==0;
    if(a.k==K_L){
        if(!a.l||!b.l||a.l->n!=b.l->n) return 0;
        for(int i=0;i<a.l->n;i++) if(!eqv(a.l->xs[i],b.l->xs[i])) return 0;
        return 1;
    }
    if(a.k==K_C){
        if(!a.c||!b.c||strcmp(a.c->tag?a.c->tag:"", b.c->tag?b.c->tag:"")!=0) return 0;
        if(a.c->n!=b.c->n) return 0;
        for(int i=0;i<a.c->n;i++) if(!eqv(a.c->xs[i],b.c->xs[i])) return 0;
        return 1;
    }
    if(a.k==K_R){
        if(!a.r||!b.r||a.r->n!=b.r->n) return 0;
        for(int i=0;i<a.r->n;i++){
            Val av=a.r->vs[i], bv=rec_get(b, a.r->ks[i]);
            if(!eqv(av,bv)) return 0;
        }
        return 1;
    }
    return 0;
}
static int cap_on;
static char *capb;
static size_t capn, capm;
static void cap_put(const char *s){
    if(!s) return;
    size_t k=strlen(s);
    if(capn+k+1>capm){ capm=capn+k+64; capb=realloc(capb, capm); }
    memcpy(capb+capn, s, k);
    capn+=k;
    capb[capn]=0;
}
static Val h_show(Val x);
static Val h_println(Val x){
    Val sh=h_show(x);
    const char *s=sh.k==K_T&&sh.s? sh.s : "()";
    if(cap_on){ cap_put(s); cap_put("\n"); }
    else printf("%s\n", s);
    return V_U();
}
static void show_into(char *o, size_t cap, Val x, int d){
    if(cap<4) return;
    if(d<=0){ strncat(o, "...", cap-strlen(o)-1); return; }
    if(x.k==K_T){
        size_t n=strlen(o), m=x.s?strlen(x.s):0;
        if(n+m+3<cap){ strcat(o, "\""); if(x.s) strcat(o, x.s); strcat(o, "\""); }
    } else if(x.k==K_I){ char b[32]; sprintf(b,"%ld", x.i); strncat(o, b, cap-strlen(o)-1); }
    else if(x.k==K_D){ char b[64]; snprintf(b,sizeof b,"%.15g", x.x); strncat(o, b, cap-strlen(o)-1); }
    else if(x.k==K_B) strncat(o, x.i?"True":"False", cap-strlen(o)-1);
    else if(x.k==K_U) strncat(o, "()", cap-strlen(o)-1);
    else if(x.k==K_L && x.l){
        strncat(o, "[", cap-strlen(o)-1);
        for(int i=0;i<x.l->n;i++){
            if(i) strncat(o, ", ", cap-strlen(o)-1);
            show_into(o, cap, x.l->xs[i], d-1);
        }
        strncat(o, "]", cap-strlen(o)-1);
    } else if(x.k==K_C && x.c){
        strncat(o, x.c->tag?x.c->tag:"", cap-strlen(o)-1);
        if(x.c->n>0){
            strncat(o, "(", cap-strlen(o)-1);
            for(int i=0;i<x.c->n;i++){
                if(i) strncat(o, ", ", cap-strlen(o)-1);
                show_into(o, cap, x.c->xs[i], d-1);
            }
            strncat(o, ")", cap-strlen(o)-1);
        }
    } else strncat(o, "()", cap-strlen(o)-1);
}
static Val h_show(Val x){
    if(x.k==K_T) return x;
    if(x.k==K_I){ char b[32]; sprintf(b,"%ld",x.i); return V_T(b); }
    if(x.k==K_D){ char b[64]; snprintf(b,sizeof b,"%.15g", x.x); return V_T(b); }
    if(x.k==K_B) return V_T(x.i?"True":"False");
    char buf[1024]; buf[0]=0;
    show_into(buf, sizeof buf, x, 6);
    return V_T(buf);
}
static Val h_join(Val xs, Val sep){
    if(xs.k!=K_L||!xs.l) return V_T("");
    const char *sp = (sep.k==K_T && sep.s)? sep.s : "";
    size_t n=1;
    for(int i=0;i<xs.l->n;i++){
        Val t=xs.l->xs[i];
        n += (t.k==K_T && t.s)? strlen(t.s):0;
        if(i+1<xs.l->n) n+=strlen(sp);
    }
    char *b=calloc(n,1);
    for(int i=0;i<xs.l->n;i++){
        Val t=xs.l->xs[i];
        if(t.k==K_T && t.s) strcat(b,t.s);
        if(i+1<xs.l->n) strcat(b,sp);
    }
    Val r=V_T(b); free(b); return r;
}
static Val h_filter(Val xs, Val f){
    List *o=lst_new();
    if(xs.k!=K_L||!xs.l||!f.fn1) return V_L(o);
    for(int i=0;i<xs.l->n;i++){
        Val x=xs.l->xs[i];
        if(truth(f.fn1(x))) lst_add(o,x);
    }
    return V_L(o);
}
static Val h_map(Val xs, Val f){
    List *o=lst_new();
    if(xs.k!=K_L||!xs.l||!f.fn1) return V_L(o);
    for(int i=0;i<xs.l->n;i++) lst_add(o, f.fn1(xs.l->xs[i]));
    return V_L(o);
}
static Val h_cat(Val a, Val b){
    const char *as=a.k==K_T&&a.s?a.s:"";
    const char *bs=b.k==K_T&&b.s?b.s:"";
    char *p=malloc(strlen(as)+strlen(bs)+1);
    sprintf(p,"%s%s",as,bs);
    Val r=V_T(p); free(p); return r;
}
static Val h_ffi_abs(Val x){
    long n = x.k==K_I ? x.i : 0;
    return V_I(n<0?-n:n);
}
static int ffi_known(const char *lib, const char *nm){
    return lib && nm && strcmp(lib,"c")==0 && (strcmp(nm,"abs")==0 || strcmp(nm,"labs")==0);
}
#if defined(_WIN32) || defined(__wasi__)
static Val h_ffi_call(Val lib, Val name, Val arg){
    const char *lb = lib.k==K_T && lib.s ? lib.s : "";
    const char *nm = name.k==K_T && name.s ? name.s : "";
    if(!ffi_known(lb, nm)) return V_I(0);
    return h_ffi_abs(arg);
}
#else
static Val h_ffi_call(Val lib, Val name, Val arg){
    const char *lb = lib.k==K_T && lib.s ? lib.s : "";
    const char *nm = name.k==K_T && name.s ? name.s : "";
    if(!ffi_known(lb, nm)) return V_I(0);
    void *h = dlopen(NULL, RTLD_LAZY);
    if(!h) return h_ffi_abs(arg);
    int (*fn)(int) = (int(*)(int))dlsym(h, nm);
    if(!fn) return h_ffi_abs(arg);
    int a = arg.k==K_I ? (int)arg.i : 0;
    return V_I(fn(a));
}
#endif

typedef struct { Val box; int done; } ConcBox;
static void *conc_run(void *p){
    ConcBox *b = p;
    /* filled by generated code via fn1 */
    (void)b;
    return NULL;
}

enum {
    OP_HALT=0, OP_ICONST, OP_SCONST, OP_BCONST, OP_UCONST,
    OP_LOAD, OP_STORE, OP_POP, OP_CALLG, OP_RET,
    OP_ADD, OP_SUB, OP_EQ, OP_NE, OP_LT, OP_AND, OP_OR, OP_NOT,
    OP_JMP, OP_JMPZ, OP_LIST, OP_REC, OP_GET, OP_PRINT, OP_CAT,
    OP_CLO, OP_LOADC, OP_CALL, OP_DUP, OP_LEN,
    OP_IDX, OP_TAIL, OP_CTOR, OP_ISTAG, OP_GETI, OP_MAYBEIDX,
    OP_BUILTIN, OP_MUL, OP_DIV, OP_LE, OP_GE, OP_GT,
    OP_LNEW, OP_LADD, OP_LEXTEND, OP_ISLIST, OP_NONE,
    OP_PUSH_H, OP_POP_H, OP_THROW, OP_OKERR,
    OP_GLOAD, OP_GSTORE,
    OP_MOD, OP_IDIV, OP_POW, OP_DCONST, OP_NEG
};

enum {
    BI_JOIN=0, BI_RANGE, BI_CHARS, BI_TEXT_LEN, BI_SLICE, BI_STARTS,
    BI_INT_TO_TEXT, BI_LIST_LEN, BI_DIE, BI_ARGV, BI_READ, BI_WRITE, BI_PRINT,
    BI_FOLDL, BI_ANY, BI_MAP, BI_FILTER,
    BI_FFI, BI_FAKE_IO, BI_OPEN, BI_STATE_GET, BI_STATE_PUT, BI_STATE_RUN,
    BI_EX_RESULT, BI_STREAM_ITER, BI_STREAM_TAKE, BI_STREAM_MAP, BI_TOGETHER,
    BI_VM_EXEC, BI_CC_LINK, BI_CLOSE, BI_REC_PATCH,
    BI_VECT, BI_VECT_LEN, BI_VECT_XS,
    BI_CHMOD_X, BI_SET_DIAG, BI_CAP_START, BI_CAP_TAKE, BI_HEX_ESC,
    BI_READ_LINE, BI_CATCH_DIE, BI_WRITE_OUT, BI_SHELL,
    BI_WRITE_ERR, BI_EXIT, BI_GC,
    BI_VECT_CONS, BI_VECT_HEAD,
    BI_REPL_TTY, BI_REPL_NAMES, BI_REPL_PROMPT,
    BI_ABS, BI_ISQRT, BI_FLOAT, BI_TOINT, BI_SQRT
};

#define VM_STACK 8192
#define VM_FRAMES 131072
#define VM_LOCALS 1048576
typedef struct {
    const char *name;
    int entry;
    int arity;
    int nlocals;
} VMFn;
typedef struct {
    const int *code; int ncode;
    const char **strs; int nstr;
    const VMFn *fns; int nfns;
    int main_id;
} VMProg;
typedef struct {
    int ip; int bp; int nlocals;
    Val *caps; int ncap;
} Frame;

static int g_argc;
static char **g_argv;
static void h2o_set_argv(int c, char **v){ g_argc=c; g_argv=v; }

static VMProg GP;
static Val vst[VM_STACK];
static int vsp;
static Val vlocs[VM_LOCALS];
static int vloc_top;
static Frame vfr[VM_FRAMES];
static int vfp;
static int vip, vbp;
static Val *vcaps;
static int vncap;
typedef struct { Val rec; int fp; } HEnt;
static HEnt hstack[64];
static int hsp;
static int vm_thrown;
static Val vm_exc;
static Val vglob[256];
static void gc_collect(void){
    gc_busy=1;
    for(int i=0;i<vsp;i++) gc_mark_val(vst[i]);
    for(int i=0;i<vloc_top;i++) gc_mark_val(vlocs[i]);
    for(int i=0;i<256;i++) gc_mark_val(vglob[i]);
    for(int i=0;i<hsp;i++) gc_mark_val(hstack[i].rec);
    gc_mark_val(vm_exc);
    if(vcaps){
        gc_mark_ptr(vcaps);
        for(int i=0;i<vncap;i++) gc_mark_val(vcaps[i]);
    }
    for(int i=0;i<vfp;i++){
        if(vfr[i].caps){
            gc_mark_ptr(vfr[i].caps);
            for(int j=0;j<vfr[i].ncap;j++) gc_mark_val(vfr[i].caps[j]);
        }
    }
    GEnt *e=gc_all, *keep=0;
    memset(gc_buck, 0, sizeof gc_buck);
    gc_bytes=0; gc_nobj=0;
    while(e){
        GEnt *n=e->link;
        if(e->mark){
            e->mark=0;
            e->link=keep; keep=e;
            unsigned h=gc_hash(e->p);
            e->next=gc_buck[h]; gc_buck[h]=e;
            gc_bytes+=e->sz; gc_nobj++;
        } else {
            free(e->p);
            free(e);
        }
        e=n;
    }
    gc_all=keep;
    gc_lim=gc_bytes*3;
    if(gc_lim<512u*1024u) gc_lim=512u*1024u;
    gc_busy=0;
}
static int die_json;
static int catch_on;
static char catch_msg[2048];
static int catch_vfp, catch_vsp, catch_vloc, catch_vbp, catch_vip, catch_vncap;
static Val *catch_vcaps;
#ifndef __wasi__
static jmp_buf catch_jb;
#endif

static Val hget(const char *lab){
    for(int i=hsp-1;i>=0;i--){
        Val v=rec_get(hstack[i].rec, lab);
        if(v.k!=K_U) return v;
    }
    return V_U();
}
static int vm_exec_vals(Val code_v, Val strs_v, Val fns_v, Val mid_v);
static int vm_interp(int stop_fp);
static int vm_run(VMProg p);

static void vm_enter(int fn_id, Val *args, int nargs, Val *caps, int ncap){
    VMFn fn=GP.fns[fn_id];
    int nbp=vloc_top;
    vloc_top+=fn.nlocals;
    for(int i=0;i<fn.nlocals;i++) vlocs[nbp+i]=V_U();
    int n = nargs<fn.arity? nargs : fn.arity;
    for(int i=0;i<n;i++) vlocs[nbp+i]=args[i];
    vfr[vfp++] = (Frame){ .ip=vip, .bp=vbp, .nlocals=fn.nlocals, .caps=vcaps, .ncap=vncap };
    vbp=nbp; vip=fn.entry; vcaps=caps; vncap=ncap;
}

static Val vm_apply(int fn_id, Val *args, int n, Val *caps, int nc){
    int stop=vfp;
    vm_enter(fn_id, args, n, caps, nc);
    vm_interp(stop);
    return vsp? vst[--vsp] : V_U();
}

typedef Val (*H2oNat)(Val *args, int nargs, Val *caps, int ncap);
static H2oNat *h2o_nat_table;
static int h2o_nat_n;
static void h2o_set_nat(H2oNat *t, int n){ h2o_nat_table=t; h2o_nat_n=n; }
static Val h2o_do_builtin(int id, Val *a, int arity);
static Val clo_apply(Val f, Val *args, int n){
    if(f.k!=K_F) return V_U();
    if(f.fn1 && n==1) return f.fn1(args[0]);
    if(h2o_nat_table && f.fn_id>=0 && f.fn_id<h2o_nat_n && h2o_nat_table[f.fn_id])
        return h2o_nat_table[f.fn_id](args, n, f.caps, f.ncap);
    return vm_apply(f.fn_id, args, n, f.caps, f.ncap);
}
static Val h2o_add(Val a, Val b){
    if(a.k==K_T||b.k==K_T) return h_cat(a.k==K_T?a:h_show(a), b.k==K_T?b:h_show(b));
    if(a.k==K_D && b.k==K_D) return V_D(a.x+b.x);
    return V_I(a.i+b.i);
}
static Val h2o_sub(Val a, Val b){
    if(a.k==K_D && b.k==K_D) return V_D(a.x-b.x);
    return V_I(a.i-b.i);
}
static Val h2o_mul(Val a, Val b){
    if(a.k==K_D && b.k==K_D) return V_D(a.x*b.x);
    return V_I(a.i*b.i);
}
static Val h2o_div(Val a, Val b){
    if(a.k==K_D && b.k==K_D) return V_D(b.x!=0? a.x/b.x : 0);
    return V_I(b.i? a.i/b.i : 0);
}
static void h2o_fail(const char *m){
    if(catch_on){
        snprintf(catch_msg, sizeof catch_msg, "%s", m?m:"die");
#ifndef __wasi__
        longjmp(catch_jb, 1);
#endif
    }
    fprintf(stderr, "%s\n", m?m:"die");
    exit(1);
}
static long h2o_floordiv(long a, long b){
    if(!b) return 0;
    long q=a/b, r=a%b;
    if(r && ((a<0)!=(b<0))) q--;
    return q;
}
static long h2o_pow_i(long a, long e){
    if(e<0) h2o_fail("次方不能是負的");
    long r=1;
    while(e>0){
        if(e&1) r*=a;
        e>>=1;
        if(e) a*=a;
    }
    return r;
}
static long h2o_isqrt_i(long n){
    if(n<0) h2o_fail("isqrt 不能是負的");
    if(n<2) return n;
    long lo=1, hi=n, ans=1;
    while(lo<=hi){
        long mid=lo+(hi-lo)/2;
        if(mid<=n/mid){ ans=mid; lo=mid+1; }
        else hi=mid-1;
    }
    return ans;
}
static Val h2o_mod(Val a, Val b){ return V_I(b.i? a.i%b.i : 0); }
static Val h2o_idiv(Val a, Val b){ return V_I(h2o_floordiv(a.i, b.i)); }
static Val h2o_pow(Val a, Val b){
    if(a.k==K_D && b.k==K_D) return V_D(pow(a.x, b.x));
    return V_I(h2o_pow_i(a.i, b.i));
}
static Val h2o_eq(Val a, Val b){ return V_B(eqv(a,b)); }
static Val h2o_ne(Val a, Val b){ return V_B(!eqv(a,b)); }
static Val h2o_and(Val a, Val b){ return V_B(truth(a)&&truth(b)); }
static Val h2o_or(Val a, Val b){ return V_B(truth(a)||truth(b)); }
static Val h2o_not(Val a){ return V_B(!truth(a)); }
static Val h2o_neg(Val a){ return a.k==K_D? V_D(-a.x) : V_I(-a.i); }
static Val h2o_lt(Val a, Val b){
    if(a.k==K_D && b.k==K_D) return V_B(a.x<b.x);
    return V_B(a.i<b.i);
}
static Val h2o_le(Val a, Val b){
    if(a.k==K_D && b.k==K_D) return V_B(a.x<=b.x);
    return V_B(a.i<=b.i);
}
static Val h2o_gt(Val a, Val b){
    if(a.k==K_D && b.k==K_D) return V_B(a.x>b.x);
    return V_B(a.i>b.i);
}
static Val h2o_ge(Val a, Val b){
    if(a.k==K_D && b.k==K_D) return V_B(a.x>=b.x);
    return V_B(a.i>=b.i);
}
static Val h2o_field(Val o, const char *k){
    if(o.k==K_C && o.c && o.c->n==1 && o.c->xs[0].k==K_R)
        return rec_get(o.c->xs[0], k);
    return rec_get(o, k);
}
static Val h2o_maybeidx(Val xs, Val i){
    if(xs.k==K_L && xs.l && i.i>=0 && i.i<xs.l->n){
        Val *p=gc_vals(1); *p=xs.l->xs[i.i];
        return V_CTOR("Some", 1, p);
    }
    return V_CTOR("None", 0, 0);
}
static Val h2o_geti(Val a, int i){
    if(a.k==K_C && a.c && i>=0 && i<a.c->n) return a.c->xs[i];
    if(a.k==K_L && a.l && i>=0 && i<a.l->n) return a.l->xs[i];
    return V_U();
}
static int h2o_istag(Val a, const char *tag){
    return a.k==K_C && a.c && strcmp(a.c->tag?a.c->tag:"", tag)==0;
}
static Val h2o_clo(int id, int ncap, Val *caps){ return V_CLO(id, ncap, caps); }

static Val bi_join(Val xs, Val sep){ return h_join(xs, sep); }
static Val bi_range(Val a, Val b, int two){
    long s=two? a.i : 0, e=two? b.i : a.i;
    List *l=lst_new();
    for(long i=s;i<e;i++) lst_add(l, V_I(i));
    return V_L(l);
}
static Val bi_chars(Val s){
    List *l=lst_new();
    const char *p=s.k==K_T&&s.s? s.s : "";
    char buf[8];
    for(;*p;p++){ buf[0]=*p; buf[1]=0; lst_add(l, V_T(buf)); }
    return V_L(l);
}
static Val bi_slice(Val s, Val i, Val j){
    const char *p=s.k==K_T&&s.s? s.s : "";
    long n=(long)strlen(p), a=i.i, b=j.i;
    if(a<0) a=0; if(b<0) b=0; if(a>n) a=n; if(b>n) b=n; if(b<a) b=a;
    char *o=malloc((size_t)(b-a+1));
    memcpy(o, p+a, (size_t)(b-a)); o[b-a]=0;
    Val r=V_T(o); free(o); return r;
}

static int vm_nest;
static Val h2o_do_builtin(int id, Val *a, int arity){
    Val r=V_U();
            switch(id){
            case BI_JOIN: r=bi_join(a[0], a[1]); break;
            case BI_RANGE:
                r=arity>=2? bi_range(a[0], a[1], 1) : bi_range(a[0], V_I(0), 0); break;
            case BI_CHARS: r=bi_chars(a[0]); break;
            case BI_TEXT_LEN: r=V_I(a[0].k==K_T&&a[0].s? (long)strlen(a[0].s):0); break;
            case BI_SLICE: r=bi_slice(a[0], a[1], a[2]); break;
            case BI_STARTS: {
                const char *s=a[0].k==K_T&&a[0].s? a[0].s:"";
                const char *p=a[1].k==K_T&&a[1].s? a[1].s:"";
                r=V_B(strncmp(s,p,strlen(p))==0);
            } break;
            case BI_INT_TO_TEXT: { char b[32]; sprintf(b,"%ld", a[0].i); r=V_T(b); } break;
            case BI_LIST_LEN: r=V_I(a[0].k==K_L&&a[0].l? a[0].l->n:0); break;
            case BI_DIE: {
                const char *m=a[0].k==K_T&&a[0].s? a[0].s : "die";
                if(catch_on){
                    snprintf(catch_msg, sizeof catch_msg, "%s", m);
#ifndef __wasi__
                    longjmp(catch_jb, 1);
#endif
                }
                if(die_json){
                    fputs("[{\"message\": \"", stdout);
                    for(const char *p=m; *p; p++){
                        if(*p=='\\' || *p=='"') putchar('\\');
                        if(*p=='\n'){ fputs("\\n", stdout); continue; }
                        putchar(*p);
                    }
                    fputs("\", \"severity\": \"error\"}]\n", stdout);
                    exit(1);
                }
                fprintf(stderr, "%s\n", m); exit(1);
            } break;
            case BI_SET_DIAG: die_json=1; r=V_U(); break;
            case BI_CAP_START:
                cap_on=1; capn=0;
                if(capb) capb[0]=0;
                r=V_U();
                break;
            case BI_CAP_TAKE:
                cap_on=0;
                r=V_T(capb?capb:"");
                break;
            case BI_HEX_ESC: {
                const char *s=a[0].k==K_T&&a[0].s? a[0].s:"";
                size_t n=strlen(s);
                char *o=malloc(n*3+1);
                char *p=o;
                for(size_t i=0;i<n;i++){
                    sprintf(p, "\\%02x", (unsigned char)s[i]);
                    p+=3;
                }
                *p=0;
                r=V_T(o); free(o);
            } break;
            case BI_READ_LINE: {
                fflush(stdout);
                char buf[H2O_LINE_MAX];
                if(!h2o_read_line(buf, sizeof buf)) r=V_CTOR("None", 0, 0);
                else {
                    Val *xs=gc_vals(1);
                    xs[0]=V_T(buf);
                    r=V_CTOR("Some", 1, xs);
                }
            } break;
            case BI_REPL_TTY:
#if !defined(__wasi__) && !defined(_WIN32)
                r=V_B(isatty(0)?1:0);
#else
                r=V_B(0);
#endif
                break;
            case BI_REPL_NAMES:
                h2o_set_names(a[0]); r=V_U(); break;
            case BI_REPL_PROMPT: {
                const char *s=a[0].k==K_T&&a[0].s? a[0].s:"";
                snprintf(h2o_prompt, sizeof h2o_prompt, "%s", s);
                r=V_U();
            } break;
            case BI_ABS: {
                long x=a[0].k==K_I? a[0].i:0;
                r=V_I(x<0? -x:x);
            } break;
            case BI_ISQRT:
                r=V_I(h2o_isqrt_i(a[0].k==K_I? a[0].i:0)); break;
            case BI_FLOAT:
                r=V_D(a[0].k==K_D? a[0].x : (double)(a[0].k==K_I? a[0].i:0)); break;
            case BI_TOINT:
                r=V_I(a[0].k==K_D? (long)a[0].x : (a[0].k==K_I? a[0].i:0)); break;
            case BI_SQRT: {
                double x=a[0].k==K_D? a[0].x : 0;
                if(x<0) h2o_fail("sqrt 不能是負的");
                r=V_D(sqrt(x));
            } break;
            case BI_WRITE_OUT: {
                const char *s=a[0].k==K_T&&a[0].s? a[0].s : "";
                fputs(s, stdout);
                fflush(stdout);
                r=V_U();
            } break;
            case BI_SHELL: {
                const char *s=a[0].k==K_T&&a[0].s? a[0].s : "";
#ifdef __wasi__
                (void)s;
                r=V_I(-1);
#else
                r=V_I(system(s));
#endif
            } break;
            case BI_WRITE_ERR: {
                const char *s=a[0].k==K_T&&a[0].s? a[0].s : "";
                fputs(s, stderr);
                fflush(stderr);
                r=V_U();
            } break;
            case BI_EXIT: {
                int c=a[0].k==K_I? (int)a[0].i : 1;
                exit(c);
            } break;
            case BI_GC:
                gc_collect();
                r=V_I((long)gc_nobj);
                break;
            case BI_CATCH_DIE: {
                Val f=a[0];
#ifdef __wasi__
                Val inner=clo_apply(f, 0, 0);
                Val *xs=gc_vals(1);
                xs[0]=inner;
                r=V_CTOR("Ok", 1, xs);
#else
                catch_vfp=vfp; catch_vsp=vsp; catch_vloc=vloc_top; catch_vbp=vbp;
                catch_vip=vip; catch_vcaps=vcaps; catch_vncap=vncap;
                if(setjmp(catch_jb)==0){
                    catch_on=1;
                    Val inner=clo_apply(f, 0, 0);
                    catch_on=0;
                    Val *xs=gc_vals(1);
                    xs[0]=inner;
                    r=V_CTOR("Ok", 1, xs);
                } else {
                    catch_on=0;
                    vfp=catch_vfp; vsp=catch_vsp; vloc_top=catch_vloc; vbp=catch_vbp;
                    vip=catch_vip; vcaps=catch_vcaps; vncap=catch_vncap;
                    Val *xs=gc_vals(1);
                    xs[0]=V_T(catch_msg);
                    r=V_CTOR("Err", 1, xs);
                }
#endif
            } break;
            case BI_ARGV: {
                List *l=lst_new();
                for(int i=0;i<g_argc;i++) lst_add(l, V_T(g_argv[i]));
                r=V_L(l);
            } break;
            case BI_READ: {
                const char *path=a[0].k==K_T&&a[0].s? a[0].s : "";
                Val io=hget("IO");
                if(io.k==K_R){
                    Val files=rec_get(io, "files");
                    Val t=rec_get(files, path);
                    if(t.k==K_T){ r=t; break; }
                }
                FILE *f=fopen(path,"rb");
                if(!f){ r=V_T(""); break; }
                fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
                if(n<0) n=0;
                char *buf=malloc((size_t)n+1);
                n=(long)fread(buf,1,(size_t)n,f); buf[n]=0; fclose(f);
                r=V_T(buf); free(buf);
            } break;
            case BI_WRITE: {
                const char *path=a[0].k==K_T&&a[0].s? a[0].s : "";
                const char *s=a[1].k==K_T&&a[1].s? a[1].s : "";
                FILE *f=fopen(path,"wb");
                if(f){ fputs(s,f); fclose(f); }
                r=V_U();
            } break;
            case BI_PRINT: r=h_println(a[0]); break;
            case BI_FOLDL: {
                Val xs=a[0], acc=a[1], f=a[2];
                if(xs.k==K_L && xs.l)
                    for(int i=0;i<xs.l->n;i++){
                        Val av[2]={acc, xs.l->xs[i]};
                        acc=clo_apply(f, av, 2);
                    }
                r=acc;
            } break;
            case BI_ANY: {
                Val xs=a[0], f=a[1]; r=V_B(0);
                if(xs.k==K_L && xs.l)
                    for(int i=0;i<xs.l->n;i++){
                        Val av[1]={xs.l->xs[i]};
                        if(truth(clo_apply(f, av, 1))){ r=V_B(1); break; }
                    }
            } break;
            case BI_MAP: {
                Val xs=a[0], f=a[1]; List *o=lst_new();
                if(xs.k==K_L && xs.l)
                    for(int i=0;i<xs.l->n;i++){
                        Val av[1]={xs.l->xs[i]};
                        lst_add(o, clo_apply(f, av, 1));
                    }
                r=V_L(o);
            } break;
            case BI_FILTER: {
                Val xs=a[0], f=a[1]; List *o=lst_new();
                if(xs.k==K_L && xs.l)
                    for(int i=0;i<xs.l->n;i++){
                        Val x=xs.l->xs[i]; Val av[1]={x};
                        if(truth(clo_apply(f, av, 1))) lst_add(o, x);
                    }
                r=V_L(o);
            } break;
            case BI_FFI: r=h_ffi_call(a[0], a[1], arity>2? a[2]:V_I(0)); break;
            case BI_FAKE_IO: {
                Rec *files=rec_new(); rec_add(files, a[0].s?a[0].s:"", a[1]);
                Rec *io=rec_new(); rec_add(io, "files", V_R(files));
                r=V_R(io);
            } break;
            case BI_OPEN: {
                Rec *o=rec_new(); rec_add(o,"path", a[0]); rec_add(o,"open", V_B(1));
                r=V_R(o);
            } break;
            case BI_STATE_GET: {
                Val st=hget("State");
                Val cell=rec_get(st, "cell");
                r=(cell.k==K_L && cell.l && cell.l->n)? cell.l->xs[0] : V_U();
            } break;
            case BI_STATE_PUT: {
                Val st=hget("State");
                Val cell=rec_get(st, "cell");
                if(cell.k==K_L && cell.l && cell.l->n) cell.l->xs[0]=a[0];
                r=V_U();
            } break;
            case BI_STATE_RUN: {
                List *cell=lst_new(); lst_add(cell, a[0]);
                Rec *o=rec_new(); rec_add(o, "cell", V_L(cell));
                r=V_R(o);
            } break;
            case BI_EX_RESULT: {
                Rec *o=rec_new(); rec_add(o, "mode", V_T("result"));
                r=V_R(o);
            } break;
            case BI_STREAM_ITER: {
                Rec *o=rec_new(); rec_add(o,"start",a[0]); rec_add(o,"f",a[1]);
                r=V_R(o);
            } break;
            case BI_STREAM_TAKE: {
                Val st=a[0]; long n=a[1].i; Val x=rec_get(st,"start"); Val f=rec_get(st,"f");
                List *o=lst_new();
                for(long i=0;i<n;i++){
                    lst_add(o, x);
                    Val av[1]={x}; x=clo_apply(f, av, 1);
                }
                r=V_L(o);
            } break;
            case BI_STREAM_MAP: r=a[0]; break;
            case BI_TOGETHER: {
                Val a0=clo_apply(a[0], 0, 0);
                Val b0=clo_apply(a[1], 0, 0);
                Val *xs=gc_vals(2); xs[0]=a0; xs[1]=b0;
                r=V_CTOR("Tuple", 2, xs);
            } break;
            case BI_VM_EXEC:
                vm_exec_vals(a[0], a[1], a[2], a[3]);
                r=vsp? vst[--vsp] : V_U();
                break;
            case BI_CC_LINK: {
                const char *c=a[0].k==K_T&&a[0].s? a[0].s : "a.c";
                const char *o=a[1].k==K_T&&a[1].s? a[1].s : "a.out";
#ifdef __wasi__
                (void)c; (void)o;
                r=V_I(-1);
#else
                char cmd[4096];
                snprintf(cmd, sizeof cmd, "cc -std=c11 -O1 -o \"%s\" \"%s\"", o, c);
                int rc=system(cmd);
                if(rc!=0){
                    snprintf(cmd, sizeof cmd, "cc -std=c11 -O1 -o \"%s\" \"%s\" -ldl", o, c);
                    rc=system(cmd);
                }
                r=V_I(rc);
#endif
            } break;
            case BI_CLOSE: {
                if(a[0].k==K_R && a[0].r)
                    for(int i=0;i<a[0].r->n;i++)
                        if(strcmp(a[0].r->ks[i],"open")==0) a[0].r->vs[i]=V_B(0);
                r=V_U();
            } break;
            case BI_REC_PATCH: {
                Val o=a[0], k=a[1], v=a[2];
                const char *key=k.k==K_T&&k.s? k.s : "";
                Rec *nr=rec_new();
                int found=0;
                if(o.k==K_R && o.r){
                    for(int i=0;i<o.r->n;i++){
                        if(strcmp(o.r->ks[i], key)==0){ rec_add(nr, key, v); found=1; }
                        else rec_add(nr, o.r->ks[i], o.r->vs[i]);
                    }
                }
                if(!found) rec_add(nr, key, v);
                r=V_R(nr);
            } break;
            case BI_VECT: {
                Val xs=a[0];
                long n=xs.k==K_L&&xs.l? xs.l->n : 0;
                Val *p=gc_vals(2);
                p[0]=V_I(n); p[1]=xs;
                r=V_CTOR("Vect", 2, p);
            } break;
            case BI_VECT_LEN:
                r=(a[0].k==K_C && a[0].c && a[0].c->n>=1)? a[0].c->xs[0] : V_I(0);
                break;
            case BI_VECT_XS:
                r=(a[0].k==K_C && a[0].c && a[0].c->n>=2)? a[0].c->xs[1] : V_L(lst_new());
                break;
            case BI_VECT_CONS: {
                Val x=a[0], v=a[1];
                Val xs=(v.k==K_C && v.c && v.c->n>=2)? v.c->xs[1] : V_L(lst_new());
                long n=(v.k==K_C && v.c && v.c->n>=1 && v.c->xs[0].k==K_I)? v.c->xs[0].i : 0;
                List *o=lst_new();
                lst_add(o, x);
                if(xs.k==K_L && xs.l){
                    int i;
                    for(i=0;i<xs.l->n;i++) lst_add(o, xs.l->xs[i]);
                }
                Val *p=gc_vals(2);
                p[0]=V_I(n+1); p[1]=V_L(o);
                r=V_CTOR("Vect", 2, p);
            } break;
            case BI_VECT_HEAD: {
                Val xs=(a[0].k==K_C && a[0].c && a[0].c->n>=2)? a[0].c->xs[1] : V_L(lst_new());
                if(xs.k==K_L && xs.l && xs.l->n>=1) r=xs.l->xs[0];
                else r=V_U();
            } break;
            case BI_CHMOD_X: {
                const char *p=a[0].k==K_T&&a[0].s? a[0].s : "";
#ifdef __wasi__
                (void)p;
#else
                chmod(p, 0755);
#endif
                r=V_U();
            } break;
            default: break;
            }

    return r;
}

static int vm_interp(int stop_fp){
    while(vfp>stop_fp && vip>=0 && vip<GP.ncode){
        int op=GP.code[vip++];
        switch(op){
        case OP_HALT: return 0;
        case OP_ICONST: vst[vsp++]=V_I(GP.code[vip++]); break;
        case OP_SCONST: vst[vsp++]=V_T(GP.strs[GP.code[vip++]]); break;
        case OP_BCONST: vst[vsp++]=V_B(GP.code[vip++]); break;
        case OP_UCONST: vst[vsp++]=V_U(); break;
        case OP_NONE: vst[vsp++]=V_CTOR("None", 0, 0); break;
        case OP_LOAD: vst[vsp++]=vlocs[vbp+GP.code[vip++]]; break;
        case OP_STORE: vlocs[vbp+GP.code[vip++]]=vst[--vsp]; break;
        case OP_LOADC: {
            int i=GP.code[vip++];
            vst[vsp++]=(vcaps && i>=0 && i<vncap)? vcaps[i] : V_U();
        } break;
        case OP_POP: if(vsp) vsp--; break;
        case OP_DUP: vst[vsp]=vst[vsp-1]; vsp++; break;
        case OP_ADD: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_add(a,b); } break;
        case OP_SUB: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_sub(a,b); } break;
        case OP_MUL: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_mul(a,b); } break;
        case OP_DIV: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_div(a,b); } break;
        case OP_MOD: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_mod(a,b); } break;
        case OP_IDIV: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_idiv(a,b); } break;
        case OP_POW: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_pow(a,b); } break;
        case OP_DCONST: vst[vsp++]=V_D(atof(GP.strs[GP.code[vip++]])); break;
        case OP_NEG: { Val a=vst[--vsp]; vst[vsp++]=h2o_neg(a); } break;
        case OP_EQ: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=V_B(eqv(a,b)); } break;
        case OP_NE: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=V_B(!eqv(a,b)); } break;
        case OP_LT: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_lt(a,b); } break;
        case OP_LE: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_le(a,b); } break;
        case OP_GE: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_ge(a,b); } break;
        case OP_GT: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h2o_gt(a,b); } break;
        case OP_AND: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=V_B(truth(a)&&truth(b)); } break;
        case OP_OR: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=V_B(truth(a)||truth(b)); } break;
        case OP_NOT: { Val a=vst[--vsp]; vst[vsp++]=V_B(!truth(a)); } break;
        case OP_JMP: vip = GP.code[vip]; break;
        case OP_JMPZ: { int t=GP.code[vip++]; Val a=vst[--vsp]; if(!truth(a)) vip=t; } break;
        case OP_LIST: {
            int n=GP.code[vip++]; List *l=lst_new();
            for(int i=0;i<n;i++) lst_add(l, vst[vsp-n+i]);
            vsp-=n; vst[vsp++]=V_L(l);
        } break;
        case OP_LNEW: vst[vsp++]=V_L(lst_new()); break;
        case OP_LADD: {
            Val x=vst[--vsp], xs=vst[--vsp];
            if(xs.k!=K_L||!xs.l) xs=V_L(lst_new());
            lst_add(xs.l, x); vst[vsp++]=xs;
        } break;
        case OP_LEXTEND: {
            Val more=vst[--vsp], xs=vst[--vsp];
            if(xs.k!=K_L||!xs.l) xs=V_L(lst_new());
            if(more.k==K_L && more.l)
                for(int i=0;i<more.l->n;i++) lst_add(xs.l, more.l->xs[i]);
            vst[vsp++]=xs;
        } break;
        case OP_PRINT: h_println(vst[--vsp]); vst[vsp++]=V_U(); break;
        case OP_CAT: { Val b=vst[--vsp], a=vst[--vsp]; vst[vsp++]=h_cat(a.k==K_T?a:h_show(a), b.k==K_T?b:h_show(b)); } break;
        case OP_GET: {
            int si=GP.code[vip++]; Val o=vst[--vsp];
            if(o.k==K_C && o.c && o.c->n==1 && o.c->xs[0].k==K_R)
                vst[vsp++]=rec_get(o.c->xs[0], GP.strs[si]);
            else
                vst[vsp++]=rec_get(o, GP.strs[si]);
        } break;
        case OP_REC: {
            int n=GP.code[vip++]; Rec *r=rec_new();
            for(int i=0;i<n;i++){
                int ki=GP.code[vip++];
                rec_add(r, GP.strs[ki], vst[vsp-n+i]);
            }
            vsp-=n; vst[vsp++]=V_R(r);
        } break;
        case OP_CLO: {
            int id=GP.code[vip++]; int ncap=GP.code[vip++];
            Val *caps=0;
            if(ncap){
                caps=gc_vals(ncap);
                for(int i=0;i<ncap;i++) caps[i]=vst[vsp-ncap+i];
                vsp-=ncap;
            }
            vst[vsp++]=V_CLO(id, ncap, caps);
        } break;
        case OP_LEN: {
            Val a=vst[--vsp];
            if(a.k==K_L && a.l) vst[vsp++]=V_I(a.l->n);
            else if(a.k==K_T && a.s) vst[vsp++]=V_I((long)strlen(a.s));
            else vst[vsp++]=V_I(0);
        } break;
        case OP_IDX: {
            Val i=vst[--vsp], xs=vst[--vsp];
            if(xs.k==K_L && xs.l && i.i>=0 && i.i<xs.l->n) vst[vsp++]=xs.l->xs[i.i];
            else vst[vsp++]=V_U();
        } break;
        case OP_MAYBEIDX: {
            Val i=vst[--vsp], xs=vst[--vsp];
            if(xs.k==K_L && xs.l && i.i>=0 && i.i<xs.l->n){
                Val *p=gc_vals(1); *p=xs.l->xs[i.i];
                vst[vsp++]=V_CTOR("Some", 1, p);
            } else vst[vsp++]=V_CTOR("None", 0, 0);
        } break;
        case OP_TAIL: {
            int n=GP.code[vip++]; Val xs=vst[--vsp];
            List *l=lst_new();
            if(xs.k==K_L && xs.l)
                for(int i=n;i<xs.l->n;i++) lst_add(l, xs.l->xs[i]);
            vst[vsp++]=V_L(l);
        } break;
        case OP_ISLIST: {
            Val a=vsp? vst[--vsp] : V_U();
            vst[vsp++]=V_B(a.k==K_L);
        } break;
        case OP_CTOR: {
            int si=GP.code[vip++]; int n=GP.code[vip++];
            Val *xs=n? gc_vals(n) : 0;
            for(int i=0;i<n;i++) xs[i]=vst[vsp-n+i];
            vsp-=n; vst[vsp++]=V_CTOR(GP.strs[si], n, xs);
        } break;
        case OP_ISTAG: {
            int si=GP.code[vip++];
            Val a=vsp? vst[--vsp] : V_U();
            int ok=a.k==K_C && a.c && strcmp(a.c->tag?a.c->tag:"", GP.strs[si])==0;
            vst[vsp++]=V_B(ok);
        } break;
        case OP_GETI: {
            int i=GP.code[vip++]; Val a=vst[--vsp];
            if(a.k==K_C && a.c && i>=0 && i<a.c->n) vst[vsp++]=a.c->xs[i];
            else if(a.k==K_L && a.l && i>=0 && i<a.l->n) vst[vsp++]=a.l->xs[i];
            else vst[vsp++]=V_U();
        } break;
        case OP_CALLG: {
            int id=GP.code[vip++]; int arity=GP.code[vip++];
            Val args[16];
            for(int i=0;i<arity;i++) args[i]=vst[vsp-arity+i];
            vsp-=arity;
            vm_enter(id, args, arity, 0, 0);
        } break;
        case OP_CALL: {
            int arity=GP.code[vip++];
            Val f=vst[--vsp];
            Val args[16];
            for(int i=0;i<arity;i++) args[i]=vst[vsp-arity+i];
            vsp-=arity;
            if(f.k==K_F) vm_enter(f.fn_id, args, arity, f.caps, f.ncap);
            else vst[vsp++]=V_U();
        } break;
        case OP_RET: {
            Val rv = vsp? vst[--vsp] : V_U();
            if(vfp<=stop_fp+1 && vfp<=1){ vst[vsp++]=rv; return 0; }
            Frame f=vfr[--vfp];
            vloc_top = vbp;
            vip=f.ip; vbp=f.bp; vcaps=f.caps; vncap=f.ncap;
            vst[vsp++]=rv;
            if(vfp<=stop_fp) return 0;
        } break;
        case OP_BUILTIN: {
            int id=GP.code[vip++]; int arity=GP.code[vip++];
            Val a[8];
            for(int i=0;i<arity;i++) a[i]=vst[vsp-arity+i];
            vsp-=arity;
            vst[vsp++]=h2o_do_builtin(id, a, arity);
        } break;
        case OP_PUSH_H: {
            if(hsp<64) hstack[hsp++]=(HEnt){ vst[--vsp], vfp };
            else vsp--;
        } break;
        case OP_POP_H: if(hsp) hsp--; break;
        case OP_THROW: {
            vm_thrown=1; vm_exc=vst[--vsp];
            Val ex=hget("Except");
            if(ex.k==K_R && rec_get(ex,"mode").k==K_T){
                int fp = hsp? hstack[hsp-1].fp : 1;
                while(vfp > fp && vfp > 1){
                    Frame f=vfr[--vfp];
                    vloc_top = vbp;
                    vip=f.ip; vbp=f.bp; vcaps=f.caps; vncap=f.ncap;
                }
            } else {
                fprintf(stderr, "unhandled Except\n"); exit(1);
            }
        } break;
        case OP_OKERR: {
            Val v=vsp? vst[--vsp] : V_U();
            Val ex=hget("Except");
            int as_res = ex.k==K_R && rec_get(ex,"mode").k==K_T;
            if(vm_thrown){
                vm_thrown=0;
                if(as_res){
                    Val *xs=gc_vals(1); *xs=vm_exc;
                    vst[vsp++]=V_CTOR("Err", 1, xs);
                } else vst[vsp++]=vm_exc;
            } else if(as_res){
                Val *xs=gc_vals(1); *xs=v;
                vst[vsp++]=V_CTOR("Ok", 1, xs);
            } else vst[vsp++]=v;
        } break;
        case OP_GLOAD: {
            int i=GP.code[vip++];
            vst[vsp++]=(i>=0&&i<256)? vglob[i] : V_U();
        } break;
        case OP_GSTORE: {
            int i=GP.code[vip++];
            Val x=vst[--vsp];
            if(i>=0&&i<256) vglob[i]=x;
        } break;
        default: return 1;
        }
    }
    return 0;
}

static int val_n(Val xs){ return xs.k==K_L && xs.l ? xs.l->n : 0; }
static Val val_at(Val xs, int i){ return xs.l->xs[i]; }

static int vm_exec_vals(Val code_v, Val strs_v, Val fns_v, Val mid_v){
    VMProg oldGP=GP;
    int old_vsp=vsp, old_loc=vloc_top, old_fp=vfp, old_ip=vip, old_bp=vbp, old_ncap=vncap, old_hsp=hsp;
    Val *old_caps=vcaps;
    int old_thrown=vm_thrown; Val old_exc=vm_exc;
    Val oldglob[256]; memcpy(oldglob, vglob, sizeof vglob);
    HEnt oldhs[64]; memcpy(oldhs, hstack, sizeof hstack);
    int nst=vsp; Val *sst=nst? malloc(sizeof(Val)*(size_t)nst):0;
    if(nst) memcpy(sst, vst, sizeof(Val)*(size_t)nst);
    int nfr=vfp; Frame *sfr=nfr? malloc(sizeof(Frame)*(size_t)nfr):0;
    if(nfr) memcpy(sfr, vfr, sizeof(Frame)*(size_t)nfr);
    int nlc=vloc_top; Val *slc=nlc? malloc(sizeof(Val)*(size_t)nlc):0;
    if(nlc) memcpy(slc, vlocs, sizeof(Val)*(size_t)nlc);

    int ncode=val_n(code_v);
    int *code=malloc(sizeof(int)*(size_t)(ncode?ncode:1));
    for(int i=0;i<ncode;i++) code[i]=(int)val_at(code_v,i).i;
    int nstr=val_n(strs_v);
    const char **strs=malloc(sizeof(char*)*(size_t)(nstr?nstr:1));
    for(int i=0;i<nstr;i++){
        Val t=val_at(strs_v,i);
        strs[i]=strdup(t.k==K_T&&t.s? t.s:"");
    }
    int nfns=val_n(fns_v);
    VMFn *fns=malloc(sizeof(VMFn)*(size_t)(nfns?nfns:1));
    for(int i=0;i<nfns;i++){
        Val rec=val_at(fns_v,i);
        Val nm=rec_get(rec,"name");
        fns[i].name=strdup(nm.k==K_T&&nm.s? nm.s:"");
        fns[i].entry=(int)rec_get(rec,"entry").i;
        fns[i].arity=(int)rec_get(rec,"arity").i;
        fns[i].nlocals=(int)rec_get(rec,"nlocals").i;
    }
    VMProg child={code, ncode, strs, nstr, fns, nfns, (int)mid_v.i};
    int rc=vm_run(child);

    GP=oldGP; vcaps=old_caps; vncap=old_ncap; hsp=old_hsp;
    vfp=old_fp; vip=old_ip; vbp=old_bp; vloc_top=old_loc;
    vm_thrown=old_thrown; vm_exc=old_exc;
    memcpy(vglob, oldglob, sizeof vglob);
    memcpy(hstack, oldhs, sizeof hstack);
    if(nfr){ memcpy(vfr, sfr, sizeof(Frame)*(size_t)nfr); free(sfr); }
    if(nlc){ memcpy(vlocs, slc, sizeof(Val)*(size_t)nlc); free(slc); }
    vsp=old_vsp;
    if(nst){ memcpy(vst, sst, sizeof(Val)*(size_t)nst); free(sst); }
    vst[vsp++]=V_I(rc);
    return rc;
}

static int vm_run(VMProg p){
    GP=p; vsp=0; vloc_top=0; vfp=0; vip=0; vbp=0; vcaps=0; vncap=0; hsp=0; vm_thrown=0;
    for(int i=0;i<256;i++) vglob[i]=V_U();
    vm_nest++;
    int main_id=p.main_id;
    if(main_id<0){
        for(int i=0;i<p.nfns;i++) if(p.fns[i].name && strcmp(p.fns[i].name,"main")==0) main_id=i;
    }
    if(main_id<0||p.nfns<=0){ vm_nest--; return 2; }
    vfr[vfp++] = (Frame){ .ip=p.ncode, .bp=0, .nlocals=0, .caps=0, .ncap=0 };
    vm_enter(main_id, 0, 0, 0, 0);
    int rc=vm_interp(1);
    vm_nest--;
    if(vm_nest==0){
        vsp=0; vloc_top=0; vfp=0; hsp=0; vcaps=0; vncap=0;
        for(int i=0;i<256;i++) vglob[i]=V_U();
        gc_collect();
    }
    return rc;
}
