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
use hilog_rust::{debug, error, hilog, HiLogLabel, LogType};
use libc::{int32_t, int64_t, c_int, c_uint};
use std::ffi::{CString, c_char};
use crate::net_packet::NetPacket;
use crate::stream_buffer::StreamBuffer;
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "StreamSession"
};
const MAX_PACKET_BUF_SIZE: usize = 256;
const SEND_RETRY_LIMIT: i32 = 32;
const SEND_RETRY_SLEEP_TIME: c_uint = 10000;
#[repr(C)]
struct EventTime {
    id: int32_t,
    event_time: int64_t,
    timer_id: int32_t,
}

/// struct stream session
#[repr(C)]
pub struct StreamSession {
    /// module_type field
    pub module_type: i32,
    /// fd field
    pub fd: i32,
    /// uid field
    pub uid : i32,
    /// pid field
    pub pid: i32,
    /// token type field
    pub token_type: i32,
}

impl StreamSession {
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

    fn uid(&self) -> i32 {
        self.uid
    }
    fn pid(&self) -> i32 {
        self.pid
    }
    fn module_type(&self) -> i32 {
        self.module_type
    }
    fn session_fd(&self) -> i32 {
        self.fd
    }
    fn set_token_type(&mut self, style: i32) {
        self.token_type = style
    }
    fn token_type(&self) -> i32 {
        self.token_type
    }
    fn session_close(&mut self) {
        debug!(LOG_LABEL, "Enter fd_:{}.", self.fd);
        if self.fd >= 0 {
            unsafe {
                libc::close(self.fd as c_int);
            }
            self.fd = -1;
        }
    }
    fn session_send_msg(&self, buf: *const c_char, size: usize) -> bool {
        if buf.is_null() {
            error!(LOG_LABEL, "CHKPF(buf) is null");
            return false;
        }
        if size == 0 || size > MAX_PACKET_BUF_SIZE {
            error!(LOG_LABEL, "buf size:{}", size);
            return false;
        }
        if self.fd < 0 {
            error!(LOG_LABEL, "The fd is less than 0");
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
            // safety: call libc library function which is unsafe function
            unsafe {
                count = libc::send(self.fd as c_int, buf.add(idx) as *const libc::c_void,
                    rem_size, libc::MSG_DONTWAIT | libc::MSG_NOSIGNAL);
                errno = *libc::__errno_location();
            };
            if count < 0 {
                if errno == libc::EAGAIN || errno == libc::EINTR || errno == libc::EWOULDBLOCK {
                    // safety: call libc library function which is unsafe function
                    unsafe {
                        libc::usleep(SEND_RETRY_SLEEP_TIME);
                    }
                    error!(LOG_LABEL, "Continue for errno EAGAIN|EINTR|EWOULDBLOCK, errno:{}", errno);
                    continue;
                }
                error!(LOG_LABEL, "Send return failed,error:{} fd:{}", errno, self.fd);
                return false;
            }
            idx += count as usize;
            rem_size -= count as usize;
            if rem_size > 0 {
                // safety: call libc library function which is unsafe function
                unsafe {
                    libc::usleep(SEND_RETRY_SLEEP_TIME);
                }
            }
        }
        if retry_count >= SEND_RETRY_LIMIT || rem_size != 0 {
            error!(LOG_LABEL, "Send too many times:{}/{},size:{}/{} fd:{}",
                retry_count, SEND_RETRY_LIMIT, idx, buf_size, self.fd);
            return false;
        }
        true
    }
    /// session send message
    pub fn send_msg_pkt(&self, pkt: &NetPacket) -> bool {
        if pkt.stream_buffer.chk_rwerror() {
            error!(LOG_LABEL, "Read and write status is error");
            return false;
        }
        let mut buf: StreamBuffer = Default::default();
        pkt.make_data(&mut buf);
        self.session_send_msg(buf.data(), buf.size())
    }
}
