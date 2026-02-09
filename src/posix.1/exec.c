#include "errno.h"
#include "unistd.h"
#include "spawn.h"
#include "cmd.h"
#include "app.h"
#include "sys_table.h"
#include "__stdlib.h"
#include "__stdio.h"
#include "sys/wait.h"
#include "sys/fcntl.h"
#include "sys/stat.h"
#include "signal.h"

typedef struct FDESC_s {
    FIL* fp;
    unsigned int flags;
    char* path;
} FDESC;
static void close_fd_for_ctx(cmd_ctx_t* ctx, FDESC* fd);

pid_t __fork(void) {
    // unsupported, use posix_spawn
    errno = ENOSYS;
    return -1;
}

pid_t __getpid(void) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    return (pid_t)ctx->pid;
}

pid_t __getppid(void) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    return (pid_t)ctx->ppid;
}

pid_t __setsid(void) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    if (ctx->pid == ctx->pgid) {
        errno = EPERM;
        return -1;
    }
    ctx->sid = ctx->pid;
    ctx->pgid = ctx->pid;
    ctx->ctty = 0;
    return (pid_t)ctx->pid;
}

pid_t __getsid(pid_t pid) {
    if (!pid) {
        cmd_ctx_t *ctx = get_cmd_ctx();
        errno = 0;
        return ctx->sid;
    }
    if (pid > 0 && pid < pids->size) {
        cmd_ctx_t *c = pids->p[pid];
        if (!c) {
            errno = ESRCH;
            return -1;
        }
        return c->sid;
    }
    errno = ESRCH;
    return -1;
}

gid_t __getgid(void) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    return ctx->gid;
}

int __setgid(gid_t gid) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    ctx->gid = gid;
    ctx->egid = gid;
    return 0;
}

gid_t __getegid(void) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    return ctx->egid;
}

uid_t __getuid(void) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    return ctx->uid;
}
uid_t __geteuid(void) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    return ctx->euid;
}

int __setuid(uid_t uid) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    ctx->uid = uid;
    ctx->euid = uid;
    return 0;
}
int __seteuid(uid_t uid) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    ctx->euid = uid;
    return 0;
}
int __setegid(gid_t gid) {
    cmd_ctx_t *ctx = get_cmd_ctx();
    ctx->egid = gid;
    return 0;
}

pid_t __getpgid(pid_t pid) {
    if (pid == 0) {
        cmd_ctx_t *ctx = get_cmd_ctx();
        errno = 0;
        return ctx->pgid;
    }

    if (pid > 0 && pid < pids->size) {
        cmd_ctx_t *ctx = pids->p[pid];
        if (!ctx) {
            errno = ESRCH;
            return -1;
        }
        return ctx->pgid;
    }

    errno = ESRCH;
    return -1;
}

int __setpgid(pid_t pid, pid_t pgid) {
    cmd_ctx_t *self = get_cmd_ctx();

    // pid == 0 → текущий процесс
    if (pid == 0)
        pid = self->pid;

    // pgid == 0 → pgid = pid
    if (pgid == 0)
        pgid = pid;

    // Проверка диапазона PID
    if (pid <= 0 || pid >= pids->size) {
        errno = ESRCH;
        return -1;
    }

    cmd_ctx_t *proc = pids->p[pid];
    if (!proc) {
        errno = ESRCH;
        return -1;
    }

    // Только процессы в одной сессии могут менять PGID
    if (proc->sid != self->sid) {
        errno = EPERM;
        return -1;
    }

    // Нельзя менять PGID, если процесс уже лидер группы (PGID == PID)
    if (proc->pgid == proc->pid) {
        errno = EPERM;
        return -1;
    }

    // Установка
    proc->pgid = pgid;
    return 0;
}

int __tcgetpgrp(int fd) {
    cmd_ctx_t *ctx = get_cmd_ctx();

    if (ctx->ctty != fd) {
        errno = ENOTTY;
        return -1;
    }

    return ctx->pgid;
}

int __tcsetpgrp(int fd, pid_t pgrp) {
    cmd_ctx_t *ctx = get_cmd_ctx();

    // Проверка: терминал должен быть управляющим для процесса
    if (ctx->ctty != fd) {
        errno = ENOTTY;
        return -1;
    }

    // Поиск группы pgrp → нужно найти лидера группы
    if (pgrp <= 0 || pgrp >= pids->size) {
        errno = ESRCH;
        return -1;
    }

    cmd_ctx_t *leader = pids->p[pgrp];
    if (!leader) {
        errno = ESRCH;
        return -1;
    }

    // Группа должна быть в той же сессии
    if (leader->sid != ctx->sid) {
        errno = EPERM;
        return -1;
    }

    // Установить foreground PGID
    // В твоей модели foreground = pgid текущего процесса
    ctx->pgid = pgrp;

    return 0;
}

static bool has_any_child(cmd_ctx_t *self) {
    for (size_t i = 1; i < pids->size; ++i) {
        cmd_ctx_t *c = pids->p[i];
        if (c && c->ppid == self->pid)
            return true;
    }
    return false;
}

void deliver_signals(cmd_ctx_t *ctx)
{
    uint32_t pending = ctx->sig_pending & ~ctx->sig_blocked;
    if (!pending)
        return;

    for (int sig = 1; sig < MAX_SIG; ++sig) {
        if (pending & (1u << sig)) {
            ctx->sig_pending &= ~(1u << sig);

            sighandler_t h = ctx->sig_handler[sig];
            if (h == SIG_IGN)
                continue;

            if (h == SIG_DFL) {
                ctx->ret_code = sig;
                ctx->stage = ZOMBIE;
                if (ctx->parent_task)
                    xTaskNotifyGive(ctx->parent_task);
                vTaskDelete(NULL);
                __unreachable();
            }

            h(sig);
        }
    }
}

pid_t __waitpid(pid_t pid, int *pstatus, int options)
{
    cmd_ctx_t *self = get_cmd_ctx();
    
    deliver_signals(self);
    
    int mode = 0;
    pid_t target_pgid = 0;

    /*
        mode:
        1 = конкретный ребёнок (pid > 0)
        2 = дети из self->pgid (pid == 0)
        3 = любой ребёнок (pid == -1)
        4 = дети из группы target_pgid (pid < -1)
    */

    if (pid > 0) {
        mode = 1;
    } else if (pid == 0) {
        mode = 2;
        target_pgid = self->pgid;
    } else if (pid == -1) {
        mode = 3;
    } else { // pid < -1
        mode = 4;
        target_pgid = -pid;
    }

search_zombie:
    // 1. Ищем подходящего зомби
    for (size_t i = 1; i < pids->size; i++) {
        cmd_ctx_t *c = pids->p[i];
        if (!c) continue;
        if (c->ppid != self->pid) continue;

        switch (mode) {
            case 1: if (c->pid != pid) continue; break;
            case 2: if (c->pgid != target_pgid) continue; break;
            case 3: /* любой */ break;
            case 4: if (c->pgid != target_pgid) continue; break;
        }

        if (c->stage == ZOMBIE) {
            // нашли
            if (pstatus)
                *pstatus = (c->ret_code & 0xFF) << 8; // POSIX EXITSTATUS

            pid_t rpid = c->pid;
            remove_ctx(c);
            return rpid;
        }
    }

    // 2. Зомби нет → проверим, есть ли вообще подходящие дети
    {
        int has_child = 0;

        for (size_t i = 1; i < pids->size; i++) {
            cmd_ctx_t *c = pids->p[i];
            if (!c) continue;
            if (c->ppid != self->pid) continue;

            switch (mode) {
                case 1: if (c->pid != pid) continue; break;
                case 2: if (c->pgid != target_pgid) continue; break;
                case 3: break;
                case 4: if (c->pgid != target_pgid) continue; break;
            }

            has_child = 1;
            break;
        }

        if (!has_child) {
            errno = ECHILD;
            return -1;
        }
    }

    // 3. Если WNOHANG → возвращаем 0
    if (options & WNOHANG)
        return 0;

    deliver_signals(self);
    // 4. Ждём уведомления и повторяем поиск
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    deliver_signals(self);
    goto search_zombie;
}

// TODO: -> .h
void init_pfiles(cmd_ctx_t* ctx);
inline static bool is_closed_desc(const FDESC* fd) {
    if (fd && !fd->fp) return true;
    return fd && (intptr_t)fd->fp > STDERR_FILENO && fd->fp->obj.fs == 0;
}

void* alloc_file(void);
void dealloc_file(void* p);
void* alloc_dir(void);
void dealloc_dir(void* p);

static cmd_ctx_t* prep_ctx(
    cmd_ctx_t* parent,
    const char *path,
    char *const argv[],
    char *const envp[]
) {
    cmd_ctx_t* child = __new_ctx();
    if (!child) return NULL;

    child->sig_pending = 0;
    child->sig_blocked = 0;
    for(int i=0;i<MAX_SIG;i++)
        child->sig_handler[i] = SIG_DFL;
    child->sig_default = DEFAULT_MASK;

    /* --- copy argv --- */
    if (argv) {
        int argc = 0;
        while (argv[argc]) argc++;

        child->argc = argc;
        child->argv = pvPortCalloc(argc + 1, sizeof(char*));

        for (int i = 0; i < argc; i++)
            child->argv[i] = copy_str(argv[i]);
    }

    /* --- set orig_cmd --- */
    child->orig_cmd = path;

    /* --- copy IO descriptors (do NOT steal parent’s descriptors!) --- */
    if (parent) {
        child->std_in  = parent->std_in;
        child->std_out = parent->std_out;
        child->std_err = parent->std_err;
    }

    /* --- build environment --- */
    if (envp) {
        /* use provided envp */
        int n = 0;
        while (envp[n]) n++;

        child->vars_num = n;
        child->vars = pvPortCalloc(n, sizeof(vars_t));

        for (int i = 0; i < n; i++) {
            char *entry = envp[i];
            char *eq = strchr(entry, '=');

            if (!eq) continue;

            size_t keylen = eq - entry;
            char *key = pvPortMalloc(keylen + 1);
            memcpy(key, entry, keylen);
            key[keylen] = 0;

            child->vars[i].key   = key;
            child->vars[i].value = copy_str(eq + 1);
        }
    } else if (parent) {
        /* inherit parent's environment */
        child->vars_num = parent->vars_num;
        child->vars = pvPortCalloc(child->vars_num, sizeof(vars_t));

        for (size_t i = 0; i < child->vars_num; i++) {
            child->vars[i].key   = copy_str(parent->vars[i].key);
            child->vars[i].value = copy_str(parent->vars[i].value);
        }
    }

    /* --- child internal fields --- */
    child->pboot_ctx   = NULL;
    child->parent_task = parent ? parent->task : 0;
    child->ppid        = parent ? parent->pid : 1; // 1 - like init proc
    child->sid         = parent ? parent->sid : -1; // TODO: new session for no parent case
    child->stage       = FOUND;
    child->ret_code    = 0;
    child->pid = -1;
    for (size_t i = 1; i < pids->size; ++i) {
        if (!pids->p[i]) {
            child->pid = i;
            pids->p[i] = child;
            break;
        }
    }
    if (child->pid == -1) {
        child->pid = (long)array_push_back(pids, child);
    }
    child->pgid = parent ? parent->pgid : child->pid;
    child->gid = child->pgid;
    child->egid = child->pgid;

    /* ===== Наследование POSIX-дескрипторов ===== */
    if (parent) {
        // гарантируем, что у родителя pfiles/pdirs инициализированы
        init_pfiles(parent);
        // гарантируем, что у ребёнка pfiles/pdirs инициализированы
        child->pfiles = new_array_v(alloc_file, dealloc_file, NULL);
        child->pdirs = new_array_v(alloc_dir, dealloc_dir, NULL);
        // копируем структуру таблицы pfiles
        for (size_t i = 0; i < parent->pfiles->size; ++i) {
            FDESC* pfd = (FDESC*)array_get_at(parent->pfiles, i);
            FDESC* cfd = 0;
            if (!pfd) {
                // дырка в таблице — сохраняем дырку
                array_push_back(child->pfiles, 0);
                continue;
            }
            if (i > STDERR_FILENO && is_closed_desc(pfd)) {
                array_push_back(child->pfiles, 0);
                continue;
            }
            // FD_CLOEXEC → в posix_spawn ребёнок их не видит
            if (pfd->flags & FD_CLOEXEC) {
                array_push_back(child->pfiles, 0);
                continue;
            }
            cfd = (FDESC*)pvPortCalloc(1, sizeof(FDESC));
            if (!cfd) {
                // OOM: можно здесь сделать простой откат:
                // cleanup_pfiles(child);
                // child->pfiles = 0; child->pdirs = 0;
                // и вернуть NULL выше
                // но для краткости сейчас пропускаем детальный rollback
                continue;
            }
            cfd->fp    = pfd->fp;
            cfd->flags = pfd->flags;
            cfd->path  = pfd->path;  // разделяем строку пути (только чтение)
            if (cfd->fp) {
                cfd->fp->pending_descriptors++;
            }
            array_push_back(child->pfiles, cfd);
        }
        // Для pdirs POSIX ничего не обещает, поэтому безопаснее их не наследовать.
    }
    return child;
}

static void vProcessTask(void *pv) {
    cmd_ctx_t* ctx = (cmd_ctx_t*)pv;
    #if DEBUG_APP_LOAD
    goutf("vProcessTask: %s [%p]\n", ctx->orig_cmd, ctx);
    #endif
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    ctx->task = th;
    vTaskSetThreadLocalStoragePointer(th, 0, ctx);
    if (ctx->sid <= 0) {
        __setsid(); // ensure new session for no-parent case
    }
    deliver_signals(ctx);
    exec_sync(ctx);
    ctx->stage = ZOMBIE;
    if (ctx->parent_task) {
        xTaskNotifyGive(ctx->parent_task);
    } else if (ctx->ppid && ctx->ppid < pids->size && pids->p[ctx->ppid]) {
        xTaskNotifyGive(((cmd_ctx_t*)pids->p[ctx->ppid])->task);
    }
    #if DEBUG_APP_LOAD
    goutf("vProcessTask: [%p] <<<\n", ctx);
    #endif
    vTaskDelete(NULL);
    __unreachable();
}

static int apply_file_actions(cmd_ctx_t* child,
                              const posix_spawn_file_actions_t *actions)
{
    if (!actions) return 0;

    cmd_ctx_t* saved = get_cmd_ctx();
    // Подменяем на child
    set_cmd_ctx(child);

    int err = 0;    
    for (size_t i = 0; i < actions->count; ++i) {
        posix_spawn_file_action_t *a = &actions->items[i];
        switch (a->type) {
       case ACTION_OPEN: {
            int fd = __openat(AT_FDCWD, a->path, a->oflag, a->mode);
            if (fd < 0) {
                err = errno ? errno : EIO;
                goto out;
            }
            // POSIX: open в file_actions обязан открыть ровно в a->fd
            if (fd != a->fd) {
                // перенесём в нужный дескриптор
                if (__dup2(fd, a->fd) < 0) {
                    err = errno ? errno : EIO;
                    goto out;
                }
                __close(fd);
            }
            break;
        }

        case ACTION_CLOSE: {
            if (__close(a->fd) < 0) {
                err = errno ? errno : EBADF;
                goto out;
            }
            break;
        }

        case ACTION_DUP2: {
            if (__dup2(a->fd, a->newfd) < 0) {
                err = errno ? errno : EBADF;
                goto out;
            }
            break;
        }

        default:
            err = ENOTSUP;
            goto out;
        }
    }

out:
    // Восстанавливаем исходный контекст
    set_cmd_ctx(saved);
    return err;
}

static int apply_proc_attr(cmd_ctx_t* child,
                           const posix_spawnattr_t *attr)
{
    if (!attr) return 0;

    // --- RESETIDS ---
    if (attr->flags & POSIX_SPAWN_RESETIDS) {
        child->uid  = child->euid;
        child->gid  = child->egid;
    }

    // --- SETPGROUP ---
    if (attr->flags & POSIX_SPAWN_SETPGROUP) {
        pid_t pg = attr->pgroup;
        if (pg == 0)
            pg = child->pid;
        child->pgid = pg;
    }

    // --- SETSID ---
    if (attr->flags & POSIX_SPAWN_SETSID) {
        if (child->pid == child->pgid)
            return EPERM;
        child->sid = child->pid;
        child->pgid = child->pid;
        child->ctty = 0;
    }

    // --- Signals (ignored, but keep POSIX shape) ---
    if (attr->flags & POSIX_SPAWN_SETSIGMASK) {
        /* ignore — no signal model yet */
    }
    if (attr->flags & POSIX_SPAWN_SETSIGDEF) {
        /* ignore — no signal model yet */
    }

    // --- Scheduling (ignored) ---
    if (attr->flags & POSIX_SPAWN_SETSCHEDPARAM) {
        /* ignore */
    }
    if (attr->flags & POSIX_SPAWN_SETSCHEDULER) {
        /* ignore */
    }

    return 0;
}

int __posix_spawn(
    pid_t *pid_out,
    const char *path,
    const posix_spawn_file_actions_t *actions,
    const posix_spawnattr_t *attr,
    char *const argv[],
    char *const envp[]
) {
    if (!argv || !argv[0]) {
        return EFAULT;
    }
    if (!path) {
        return ENOENT;
    }
    const char* rp = __realpath(path, 0);
    if (!rp) {
        return errno;
    }

    cmd_ctx_t* parent = get_cmd_ctx();
    cmd_ctx_t* child  = prep_ctx(parent, rp, argv, envp);
    if (!child) {
        return ENOMEM;
    }
    if (!load_app(child)) {
        remove_ctx(child);
        return EFAULT;
    }
    int err = apply_file_actions(child, actions);
    if (err) {
        remove_ctx(child);
        return err;
    }
    err = apply_proc_attr(child, attr);
    if (err) {
        remove_ctx(child);
        return err;
    }
    xTaskCreate(vProcessTask, child->argv[0], 1024/*x 4 = 4096*/, child, configMAX_PRIORITIES - 1, 0);

    if (pid_out)
        *pid_out = (pid_t)child->pid;
    return 0;
}

static void close_fd_for_ctx(cmd_ctx_t* ctx, FDESC* fd) {
    FIL* fp = fd->fp;
    if ((intptr_t)fp <= STDERR_FILENO) {  // just ignore close request for std descriptors
        return;
    }
    if (fp->pending_descriptors) {
        --fp->pending_descriptors;
        fd->fp = 0; // it was a pointer to other FIL in other descriptor, so cleanup it, to do not use by FDESC in 2 places
        fd->flags = 0;
        fd->path = 0;
        return;
    }
    f_close(fp);
}

int __execve(const char *pathname, char *const argv[], char *const envp[])
{
    if (!pathname) {
        errno = EFAULT;
        return -1;
    }
    cmd_ctx_t *ctx = get_cmd_ctx();
// W/As
// history (move to cmd proc)
{
    char* tmp = get_ctx_var(ctx, "TEMP");
    if(!tmp) tmp = "";
    char * cmd_history_file = concat(tmp, ".cmd_history");
    FIL* pfh = (FIL*)pvPortMalloc(sizeof(FIL));
    f_open(pfh, cmd_history_file, FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND);
    UINT br;
    f_write(pfh, pathname, strlen(pathname), &br);
    f_write(pfh, "\n", 1, &br);
    f_close(pfh);
    vPortFree(pfh);
    vPortFree(cmd_history_file);
}
set_usb_detached_handler(0);
set_scancode_handler(0);
set_cp866_handler(0);

    /* ----------------- cleanup old environment (if overwritten) ----------------- */
    if (envp) {
        vars_t* ovars = ctx->vars;
        size_t ovars_num = ctx->vars_num;
        ctx->vars = NULL;
        ctx->vars_num = 0;
        for (int i = 0; envp[i]; ++i) {
            char *kv = envp[i];
            char *eq = strchr(kv, '=');
            if (!eq) continue;
            size_t klen = eq - kv;
            char *key = pvPortMalloc(klen + 1);
            memcpy(key, kv, klen);
            key[klen] = '\0';
            set_ctx_var(ctx, key, eq + 1);
            vPortFree(key);
        }
        if (ovars) {
            for (size_t i = 0; i < ovars_num; ++i) {
                if (ovars[i].key)
                    vPortFree(ovars[i].key);
                if (ovars[i].value)
                    vPortFree(ovars[i].value);
            }
            vPortFree(ovars);
        }
    }

    /* ----------------- save for cleanup ----------------- */
    char** oargv = ctx->argv;
    size_t oargc = ctx->argc;
    /* ----------------- fill new argv ----------------- */
    if (argv) {
        int argc = 0;
        while (argv[argc]) argc++;

        ctx->argc = argc;
        ctx->argv = pvPortCalloc(argc + 1, sizeof(char*));
        for (int i = 0; i < argc; ++i)
            ctx->argv[i] = copy_str(argv[i]);
    } else { // W/A allow to be out of spec 
        ctx->argc = 1;
        ctx->argv = pvPortCalloc(2, sizeof(char*));
        ctx->argv[0] = copy_str(pathname);
    }
    /* ----------------- set new orig_cmd ----------------- */
    if (ctx->orig_cmd) {
        vPortFree(ctx->orig_cmd);
        ctx->orig_cmd = NULL;
    }
    ctx->orig_cmd = copy_str(pathname);

    /* ----------------- cleanup outdated state ----------------- */
    if (oargv) {
        for (int i = 0; i < oargc; ++i)
            vPortFree(oargv[i]);
        vPortFree(oargv);
    }
    /* --- close-on-exec --- */
    if (ctx->pfiles) {
        for (size_t i = 0; i < ctx->pfiles->size; ++i) {
            FDESC* fd = (FDESC*)array_get_at(ctx->pfiles, i);
            if (!fd) continue;
            if (is_closed_desc(fd)) continue;
            if (fd->flags & FD_CLOEXEC) {
                // локальный helper, аналог __close, но по ctx
                close_fd_for_ctx(ctx, fd);
            }
        }
    }
    /* ----------------- laod and jump into program ----------------- */
    if( !load_app(ctx) ) {
        errno = EFAULT;
        return -1;
    }
    exec_sync(ctx);
// should not be there, but if
    ctx->stage = ZOMBIE;
    if (ctx->parent_task) {
        xTaskNotifyGive(ctx->parent_task);
    }
    #if DEBUG_APP_LOAD
    goutf("__execve: [%p] <<<\n", ctx);
    #endif
    vTaskDelete( NULL );
    __unreachable();
}

char* __getenv (const char *v) {
    return get_ctx_var(get_cmd_ctx(), v);
}

int __posix_spawnp(
    pid_t *pid_out,
    const char *path,
    const posix_spawn_file_actions_t *actions,
    const posix_spawnattr_t *attr,
    char *const argv[],
    char *const envp[]
) {
    if (!argv || !argv[0]) {
        return EFAULT;
    }
    if (!path) {
        return ENOENT;
    }
    if (path[0] == '/') {
        return __posix_spawn(pid_out, path, actions, attr, argv, envp);
    }
    const char *p = __getenv("PATH");
    if (!p) p = "/bin:/usr/bin"; // POSIX fallback
    while (p && *p) {
        char* end = p;
        while (*end != 0 && *end != ':') end++; // lookup for the end of string
        size_t len = end - p;
        char* res = concat2(p, len, path);
        // goutf("try path %s\n", res);
        struct stat obuf;
        if (__stat(res, &obuf) == 0) {
            int r = __posix_spawn(pid_out, res, actions, attr, argv, envp);
            vPortFree(res);
            return r;
        }
        vPortFree(res);
        res = 0;
        if (!*end) break;
        p += len + 1;
    }
    return ENOENT;
}

int __kill(pid_t pid, int sig)
{
    if (sig <= 0 || sig >= MAX_SIG) {
        errno = EINVAL;
        return -1;
    }

    cmd_ctx_t *self = get_cmd_ctx();

    // Выбор режима
    int mode;
    pid_t target_pgid = 0;

    if (pid > 0) mode = 1;
    else if (pid == 0) { mode = 2; target_pgid = self->pgid; }
    else if (pid == -1) mode = 3;
    else { mode = 4; target_pgid = -pid; }

    int delivered = 0;

    for (size_t i = 1; i < pids->size; i++) {
        cmd_ctx_t* c = pids->p[i];
        if (!c) continue;

        switch (mode) {
            case 1: if (c->pid != pid) continue; break;
            case 2: if (c->pgid != target_pgid) continue; break;
            case 3: /* all */ break;
            case 4: if (c->pgid != target_pgid) continue; break;
        }

        // Посылаем сигнал — атомарно отмечаем, что он pending
        c->sig_pending |= (1U << sig);

        // Уведомляем задачу (разбудит waitpid и сам таск)
        xTaskNotifyGive(c->task);

        delivered = 1;
        if (mode == 1) break; // конкретный pid — один раз
    }

    if (!delivered) {
        errno = ESRCH;
        return -1;
    }

    return 0;
}

sighandler_t __signal(int sig, sighandler_t handler)
{
    if (sig <= 0 || sig >= MAX_SIG || sig == SIGKILL) {
        errno = EINVAL;
        return SIG_ERR;
    }

    cmd_ctx_t *ctx = get_cmd_ctx();
    sighandler_t old = ctx->sig_handler[sig];
    ctx->sig_handler[sig] = handler;
    return old;
}

int __raise(int sig) {
    return __kill(get_cmd_ctx()->pid, sig);
}

int __sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    cmd_ctx_t *ctx = get_cmd_ctx();

    if (oldset)
        *oldset = ctx->sig_blocked;

    if (!set)
        return 0;

    switch (how) {
        case SIG_BLOCK:
            ctx->sig_blocked |= *set;
            ctx->sig_blocked &= ~(1u << SIGKILL);
            break;
        case SIG_UNBLOCK:
            ctx->sig_blocked &= ~(*set);
            break;
        case SIG_SETMASK:
            ctx->sig_blocked = *set;
            ctx->sig_blocked &= ~(1u << SIGKILL);
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    return 0;
}
