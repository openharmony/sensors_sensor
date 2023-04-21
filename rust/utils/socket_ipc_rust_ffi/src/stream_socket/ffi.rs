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
 
use super::*;
use hilog_rust::{info, error, hilog, HiLogLabel, LogType};
use crate::error::SocketStatusCode;
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "stream_socket_ffi"
};

/// Get Fd
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_fd(object: *const StreamSocket) -> i32 {
    info!(LOG_LABEL, "enter get_fd");
    // 等价于
    // let obj = object.as_ref()                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     ;
    // match obj {
    //     Some(obj) => obj.fd(),
    //     None => return -1;
    // }
    if let Some(obj) = StreamSocket::as_ref(object) {
        obj.socket_fd()
    } else {
        SocketStatusCode::FdFail.into()
    }
}

/// Get Fd
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_epoll_fd(object: *const StreamSocket) -> i32 {
    info!(LOG_LABEL, "enter get_epoll_fd");
    if let Some(obj) = StreamSocket::as_ref(object) {
        obj.epoll_fd()
    } else {
        SocketStatusCode::EpollFdFail.into()
    }
}

/// Get Fd
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn rust_epoll_create(object: *mut StreamSocket, size: i32) -> i32 {
    info!(LOG_LABEL, "enter rust_epoll_create");

    if let Some(obj) = StreamSocket::as_mut(object) {
        obj.create_epoll_fd(size);
        obj.epoll_fd()
    } else {
        SocketStatusCode::EpollCreateFail.into()
    }

}

/// rust_epoll_ctl
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn rust_epoll_ctl(object: *const StreamSocket, fd: i32, op: i32,
    event: *mut libc::epoll_event, epoll_fd: i32) -> i32 {
    info!(LOG_LABEL, "enter rust_epoll_ctl");
    if let Some(obj) = StreamSocket::as_ref(object) {
        if fd < 0 {
            error!(LOG_LABEL, "Invalid fd");
            return -1
        }
        let epoll_fd = 
            if epoll_fd < 0 {
                if obj.is_valid_epoll(){
                    obj.epoll_fd()
                } else {
                    return -1;
                }
            } else {
                epoll_fd
            };
        StreamSocket::epoll_ctl(fd, op, event, epoll_fd)
    } else {
        SocketStatusCode::EpollCtlFail.into()
    }
}

/// rust_epoll_wait
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn rust_epoll_wait(
    object: *const StreamSocket, events: *mut libc::epoll_event, maxevents: i32, timeout: i32, epoll_fd: i32) -> i32 {
    info!(LOG_LABEL, "enter rust_epoll_wait");
    if let Some(obj) = StreamSocket::as_ref(object) {
        let epoll_fd = 
            if epoll_fd < 0 {
                if obj.is_valid_epoll() {
                    obj.epoll_fd()
                } else {
                    return -1;
                }
            } else {
                epoll_fd
            };
        StreamSocket::epoll_wait(events, maxevents, timeout, epoll_fd)
    } else {
        SocketStatusCode::EpollWaitFail.into()
    }
}

/// rust_epoll_close
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn rust_epoll_close(object: *mut StreamSocket) -> i32 {
    info!(LOG_LABEL, "enter rust_epoll_close");
    if let Some(obj) = StreamSocket::as_mut(object) {
        obj.epoll_close();
        SocketStatusCode::Ok.into()
    } else {
        SocketStatusCode::EpollCloseFail.into()
    }
}
/// rust_close
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn rust_close(object: *mut StreamSocket) -> i32 {
    info!(LOG_LABEL, "enter rust_close");
    if let Some(obj) = StreamSocket::as_mut(object) {
        obj.socket_close();
        SocketStatusCode::Ok.into()
    } else {
        SocketStatusCode::SocketCloseFail.into()
    }
}
/// send_msg_buf_size
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn send_msg_buf_size(object: *const StreamSocket, buf: *const c_char, size: usize) -> bool {
    info!(LOG_LABEL, "enter send_msg_buf_size");
    if let Some(obj) = StreamSocket::as_ref(object) {
        obj.send_msg(buf, size)
    } else {
        false
    }
}

