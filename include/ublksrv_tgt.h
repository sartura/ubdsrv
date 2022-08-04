// SPDX-License-Identifier: MIT or LGPL-2.1-only

#ifndef UBLKSRV_TGT_INC_H
#define UBLKSRV_TGT_INC_H

#include <libgen.h>
#include <coroutine>
#include <iostream>
#include <type_traits>

//we are free to use the private definition
#include "ublksrv_priv.h"

#define MAX_NR_UBLK_DEVS	128
#define UBLKSRV_PID_DIR  "/var/run/ublksrvd"

/* json device data is stored at this offset of pid file */
#define JSON_OFFSET   32

char *ublksrv_tgt_return_json_buf(struct ublksrv_dev *dev, int *size);
char *ublksrv_tgt_realloc_json_buf(struct ublksrv_dev *dev, int *size);

static inline unsigned ublksrv_convert_cmd_op(const struct ublksrv_io_desc *iod)
{
	unsigned ublk_op = ublksrv_get_op(iod);

	switch (ublk_op) {
	case UBLK_IO_OP_READ:
		return IORING_OP_READ;
	case UBLK_IO_OP_WRITE:
		return IORING_OP_WRITE;
	case UBLK_IO_OP_FLUSH:
		return IORING_OP_FSYNC;
	case UBLK_IO_OP_DISCARD:
	case UBLK_IO_OP_WRITE_SAME:
	case UBLK_IO_OP_WRITE_ZEROES:
		return IORING_OP_FALLOCATE;
	default:
		return -1;
	}
}

/* For using C++20 coroutine */
/*
 * Due to the use of std::cout, the member functions await_ready,
 * await_suspend, and await_resume cannot be declared as constexpr.
 */
struct ublk_suspend_always {
    bool await_ready() const noexcept {
        return false;
    }
    void await_suspend(std::coroutine_handle<>) const noexcept {
    }
    void await_resume() const noexcept {
    }
};

/*
 * When you don't resume the Awaitable such as the coroutine object returned
 * by the member function final_suspend, the function await_resume is not
 * processed. In contrast, the Awaitable's ublk_suspend_never the function is
 * immediately ready because await_ready returns true and, hence, does
 * not suspend.
 */
struct ublk_suspend_never {
    bool await_ready() const noexcept {
        return true;
    }
    void await_suspend(std::coroutine_handle<>) const noexcept {
    }
    void await_resume() const noexcept {
    }
};

using co_handle_type = std::coroutine_handle<>;
struct co_io_job {
    struct promise_type {
        co_io_job get_return_object() {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        ublk_suspend_never initial_suspend() {
            return {};
        }
        ublk_suspend_never final_suspend() noexcept {
            return {};
        }
        void return_void() {}
        void unhandled_exception() {}
    };

    co_handle_type coro;

    co_io_job(co_handle_type h): coro(h){}

    void resume() {
        coro.resume();
    }

    operator co_handle_type() const { return coro; }
};

/*
 * c++20 is stackless coroutine, and can't handle nested coroutine, so
 * the following two have to be defined as macro
 */
#define co_io_job_submit_and_wait() do {		\
	co_await ublk_suspend_always();			\
} while (0)

#define co_io_job_return() do {		\
	co_return;			\
} while (0)

struct ublk_io_tgt {
	char __reserved[sizeof(struct ublk_io) - sizeof(co_handle_type)];
	co_handle_type co;
};

static_assert(sizeof(struct ublk_io_tgt) == sizeof(struct ublk_io), "ublk_io is defined as wrong");

#endif
