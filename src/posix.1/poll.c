#include <FreeRTOS.h>
#include <task.h>

#include "cmd.h"
#include "poll.h"
#include "errno.h"

void kbd_add_stdin_waiter(TaskHandle_t th);
void kbd_remove_stdin_waiter(TaskHandle_t th);
void deliver_signals(cmd_ctx_t *ctx);
extern volatile int __c; // keyboard.c

static short poll_fd_events(int fd, short events)
{
    // stdin
    if (fd == 0) {
        short re = 0;
        if ((events & POLLIN) && __c)
            re |= POLLIN;
        return re;
    }

    // stdout / stderr
    if (fd == 1 || fd == 2) {
        if (events & POLLOUT)
            return POLLOUT;
        return 0;
    }

    // обычные файлы
    if (fd >= 0) {
        short re = 0;
        if (events & POLLIN)  re |= POLLIN;
        if (events & POLLOUT) re |= POLLOUT;
        return re;
    }

    return POLLNVAL;
}


int __poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    cmd_ctx_t *ctx = get_cmd_ctx();
    int ready;

again:
    ready = 0;

    // 1. Очистить revents
    for (nfds_t i = 0; i < nfds; ++i)
        fds[i].revents = 0;

    // 2. Проверить события
    for (nfds_t i = 0; i < nfds; ++i) {
        if (fds[i].fd < 0) {
            fds[i].revents = POLLNVAL;
            ready++;
            continue;
        }

        short re = poll_fd_events(fds[i].fd, fds[i].events);
        if (re) {
            fds[i].revents = re;
            ready++;
        }
    }

    // 3. Если что-то готово — вернуть
    if (ready > 0)
        return ready;

    // 4. Немедленный возврат
    if (timeout == 0)
        return 0;

    // 5. Перед блокировкой — сигналы
    deliver_signals(ctx);

    int wants_stdin = 0;
    for (nfds_t i = 0; i < nfds; ++i) {
        if (fds[i].fd == 0 && (fds[i].events & POLLIN)) {
            wants_stdin = 1;
            break;
        }
    }

    TaskHandle_t me = xTaskGetCurrentTaskHandle();
    if (wants_stdin && !__c) {
        kbd_add_stdin_waiter(me);
    }

    // 6. Ожидание
    if (timeout < 0) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    } else {
        TickType_t ticks = pdMS_TO_TICKS(timeout);
        if (ticks == 0) ticks = 1;
        ulTaskNotifyTake(pdTRUE, ticks);
        timeout = 0;
    }

    if (wants_stdin) {
        kbd_remove_stdin_waiter(me);
    }

    /* ЛОЖНОЕ ПРОБУЖДЕНИЕ:
       poll имеет право проснуться, но не имеет права
       вернуть POLLIN без реального символа */
    if (wants_stdin && __c == 0) {
        goto again;
    }

    // 7. После пробуждения — сигналы
    deliver_signals(ctx);

    goto again;
}
