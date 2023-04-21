/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
use std::ptr;
use libc::{c_int, c_uint};
use std::ffi::{CString, c_char};
const ONCE_PROCESS_NETPACKET_LIMIT: i32 = 100;
const MAX_PACKET_BUF_SIZE: usize = 256;
const SEND_RETRY_SLEEP_TIME: c_uint = 10000;
const SEND_RETRY_LIMIT: i32 = 32;

const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "StreamSocket"
};

/// struct StreamSocket
#[repr(C)]
pub struct StreamSocket {
    /// socket_fd
    pub socket_fd: i32,
    /// epoll_fd
    pub epoll_fd: i32,
}

impl Default for StreamSocket {
    fn default() -> Self {
        Self {
            socket_fd: -1,
            epoll_fd: -1,
        }
    }
}

impl StreamSocket {
    /// return const referance of self
    ///
    /// # Safety
    /// 
    /// Makesure object is null pointer
    unsafe fn as_ref<'a>(object: *const Self) -> Option<&'a Self>{
        object.as_ref()
    }
    /// return mutable referance of self
    ///
    /// # Safety
    /// 
    /// Makesure object is null pointer
    unsafe fn as_mut<'a>(object: *mut Self) -> Option<&'a mut Self>{
        object.as_mut()
    }
    /// get socket_fd
    pub fn socket_fd(&self) -> i32 {
        self.socket_fd
    }
    /// get epoll_fd
    pub fn epoll_fd(&self) -> i32 {
        self.epoll_fd
    }
    /// adjust whether epoll_fd is valid or not 
    pub fn is_valid_epoll(&self) -> bool {
        self.epoll_fd > 0
    }
    /// create epoll_fd
    pub fn create_epoll_fd(&mut self, size: i32) {
        let epoll_fd;
        unsafe {
            epoll_fd = libc::epoll_create(size);
        }
        if epoll_fd < 0 {
            error!(LOG_LABEL, "epoll_create return {}", epoll_fd);
        } else {
            self.epoll_fd = epoll_fd;
            info!(LOG_LABEL, "epoll_create, epoll_fd:{}", epoll_fd);
        }
    }
    /// event is null pointer
    pub fn epoll_ctl(fd: i32, op: i32, event: *mut libc::epoll_event, epoll_fd: i32) -> i32 {
        if event.is_null() {
            error!(LOG_LABEL, "event is nullptr");
            return -1;
        }
        if op == libc::EPOLL_CTL_DEL {
            // safety: call unsafe function
            unsafe {
                libc::epoll_ctl(epoll_fd as c_int, op as c_int, fd as c_int, ptr::null_mut())
            }
        } else {
            // safety: call unsafe function
            unsafe {
                libc::epoll_ctl(epoll_fd as c_int, op as c_int, fd as c_int, event as *mut libc::epoll_event)
            }
        }
    }
    /// epoll_wait
    ///
    /// # Safety
    /// 
    /// call libc
    pub unsafe fn epoll_wait(events: *mut libc::epoll_event, maxevents: i32, timeout: i32, epoll_fd: i32) -> i32 {
        let ret = libc::epoll_wait(
            epoll_fd as c_int, events as *mut libc::epoll_event, maxevents as c_int, timeout as c_int);
        if ret < 0 {
            let errno = *libc::__errno_location();
            error!(LOG_LABEL, "epoll_wait ret:{},errno:{}", ret, errno);
        }
        ret
    }
    /// epoll close
    pub fn epoll_close(&mut self) {
        if self.epoll_fd >= 0 {
            unsafe {
                libc::close(self.epoll_fd as c_int);
            }
            self.epoll_fd = -1;
        }
    }
    /// socket close
    pub fn socket_close(&mut self) {
        if self.socket_fd >= 0 {
            unsafe {
                let rf = libc::close(self.socket_fd as c_int);
                if rf > 0 {
                    error!(LOG_LABEL, "Socket close failed rf:{}", rf);
                }
            }
            self.socket_fd = -1;
        }
    }
    /// send message
    ///
    /// # Safety
    /// 
    /// call unsafe function
    pub unsafe fn send_msg(&self, buf: *const u8, size: usize) -> bool {
        if buf.is_null() {
            error!(LOG_LABEL, "CHKPF(buf) is null");
            return false;
        }
        if size == 0 || size > MAX_PACKET_BUF_SIZE {
            error!(LOG_LABEL, "Stream buffer size out of range");
            return false;
        }
        if self.socket_fd < 0 {
            error!(LOG_LABEL, "The fd_ is less than 0");
            return false;
        }
        let mut idx = 0;
        let mut retry_count = 0;
        let buf_size = size;
        let mut rem_size = buf_size;
        while rem_size > 0 && retry_count < SEND_RETRY_LIMIT {
            retry_count += 1;
            let count;
            let errno;
            unsafe {
                count = libc::send(self.socket_fd as c_int, buf.add(idx) as *const libc::c_void,
                    rem_size, libc::MSG_DONTWAIT | libc::MSG_NOSIGNAL);
                errno = *libc::__errno_location();
            }
            if count < 0 {
                if errno == libc::EAGAIN || errno == libc::EINTR || errno == libc::EWOULDBLOCK {
                    error!(LOG_LABEL, "Continue for errno EAGAIN|EINTR|EWOULDBLOCK, errno:{}", errno);
                    continue;
                }
                error!(LOG_LABEL, "Send return failed,error:{} fd:{}", errno, self.socket_fd);
                return false;
            }
            idx += count as usize;
            rem_size -= count as usize;
            if rem_size > 0 {
                unsafe {
                    libc::usleep(SEND_RETRY_SLEEP_TIME);
                }
            }
        }
        if retry_count >= SEND_RETRY_LIMIT || rem_size != 0 {
            error!(LOG_LABEL, "Send too many times:{}/{},size:{}/{} fd:{}",
                retry_count, SEND_RETRY_LIMIT, idx, buf_size, self.socket_fd);
            return false;
        }
        true
    }

}


