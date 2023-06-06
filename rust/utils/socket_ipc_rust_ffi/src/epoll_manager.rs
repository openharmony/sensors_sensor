/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
/// provide C interface to C++ for calling
pub mod ffi;
use hilog_rust::{info, error, hilog, HiLogLabel, LogType};
use std::{ptr, ffi::{CString, c_char}};
use libc::c_int;
const ONCE_PROCESS_NETPACKET_LIMIT: i32 = 100;
const MAX_PACKET_BUF_SIZE: usize = 256;
const SEND_RETRY_SLEEP_TIME: u64 = 10000;
const SEND_RETRY_LIMIT: i32 = 32;
const RET_ERR: i32 = -1;
const RET_OK: i32 = 0;

const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "EpollManager"
};

/// struct EpollManager
#[repr(C)]
pub struct EpollManager {
    /// socket_fd
    pub socket_fd: i32,
    /// epoll_fd
    pub epoll_fd: i32,
}

impl Default for EpollManager {
    fn default() -> Self {
        Self {
            socket_fd: -1,
            epoll_fd: -1,
        }
    }
}

impl EpollManager {
    fn as_ref<'a>(object: *const Self) -> Option<&'a Self> {
        // SAFETY: as_ref has already done no-null verification inside
        unsafe {
            object.as_ref()
        }
    }
    fn as_mut<'a>(object: *mut Self) -> Option<&'a mut Self> {
        // SAFETY: as_mut has already done no-null verification inside
        unsafe {
            object.as_mut()
        }
    }
    fn socket_fd(&self) -> i32 {
        self.socket_fd
    }
    fn epoll_fd(&self) -> i32 {
        self.epoll_fd
    }
    fn is_valid_epoll(&self) -> bool {
        self.epoll_fd > 0
    }
    fn create_epoll_fd(&mut self, size: i32) {
        // SAFETY: call unsafe function
        let epoll_fd = unsafe {
            libc::epoll_create(size)
        };
        if epoll_fd < 0 {
            error!(LOG_LABEL, "epoll_create return {}", epoll_fd);
        } else {
            self.epoll_fd = epoll_fd;
            info!(LOG_LABEL, "epoll_create, epoll_fd:{}", epoll_fd);
        }
    }
    fn epoll_ctl(fd: i32, op: i32, event: *mut libc::epoll_event, epoll_fd: i32) -> i32 {
        if event.is_null() {
            error!(LOG_LABEL, "event is nullptr");
            return RET_ERR;
        }
        if op == libc::EPOLL_CTL_DEL {
            // SAFETY: call unsafe function
            unsafe {
                libc::epoll_ctl(epoll_fd as c_int, op as c_int, fd as c_int, ptr::null_mut())
            }
        } else {
            // SAFETY: call unsafe function
            unsafe {
                libc::epoll_ctl(epoll_fd as c_int, op as c_int, fd as c_int, event as *mut libc::epoll_event)
            }
        }
    }
    fn epoll_wait(events: *mut libc::epoll_event, maxevents: i32, timeout: i32, epoll_fd: i32) -> i32 {
        // SAFETY: call unsafe function
        let ret = unsafe {
            libc::epoll_wait(epoll_fd as c_int, events as *mut libc::epoll_event, maxevents as c_int, timeout as c_int)
        };
        if ret < 0 {
            // SAFETY: call unsafe function
            let errno = unsafe { 
                *libc::__errno_location()
            };
            error!(LOG_LABEL, "epoll_wait ret:{},errno:{}", ret, errno);
        }
        ret
    }
    fn epoll_close(&mut self) {
        if self.epoll_fd >= RET_OK {
            // SAFETY: call unsafe function
            unsafe {
                libc::close(self.epoll_fd as c_int);
            }
            self.epoll_fd = RET_ERR;
        }
    }
    fn socket_set_fd(&mut self, fd: i32) {
        self.socket_fd = fd
    }
    fn find_epoll_fd(&self, epoll_fd: i32) -> i32 {
        if epoll_fd < 0 {
            if self.is_valid_epoll(){
                self.epoll_fd()
            } else {
                RET_ERR
            }
        } else {
            epoll_fd
        }
    }
    fn socket_close(&mut self) {
        if self.socket_fd >= RET_OK {
            // safety: call unsafe function
            let result = unsafe {
                libc::close(self.socket_fd as c_int)
            };
            if result > RET_OK {
                error!(LOG_LABEL, "Socket close failed result:{}", result);
            }
            self.socket_fd = RET_ERR;
        }
    }
}


