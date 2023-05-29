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
use crate::epoll_manager::EpollManager;
use std::mem::drop;
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "stream_socket_ffi"
};

/// create unique_ptr of stream_socket for C++
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn StreamSocketCreate() -> *mut EpollManager {
    let epoll_manager: Box::<EpollManager> = Box::default(); 
    Box::into_raw(epoll_manager)
}
/// drop unique_ptr of stream_socket for C++
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn StreamSocketDelete(raw: *mut EpollManager) {
    if !raw.is_null() {
        drop(Box::from_raw(raw));
    }
}
/// Get Fd
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn StreamSocketGetFd(object: *const EpollManager) -> i32 {
    info!(LOG_LABEL, "enter StreamSocketGetFd");
    if let Some(obj) = EpollManager::as_ref(object) {
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
pub unsafe extern "C" fn StreamSocketGetEpollFd(object: *const EpollManager) -> i32 {
    info!(LOG_LABEL, "enter StreamSocketGetEpollFd");
    if let Some(obj) = EpollManager::as_ref(object) {
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
pub unsafe extern "C" fn StreamSocketEpollCreate(object: *mut EpollManager, size: i32) -> i32 {
    info!(LOG_LABEL, "enter StreamSocketEpollCreate");
    if let Some(obj) = EpollManager::as_mut(object) {
        obj.create_epoll_fd(size);
        obj.epoll_fd()
    } else {
        SocketStatusCode::EpollCreateFail.into()
    }

}

/// StreamSocketEpollCtl
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn StreamSocketEpollCtl(object: *const EpollManager, fd: i32, op: i32,
    event: *mut libc::epoll_event, epoll_fd: i32) -> i32 {
    info!(LOG_LABEL, "enter StreamSocketEpollCtl");
    if let Some(obj) = EpollManager::as_ref(object) {
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
        EpollManager::epoll_ctl(fd, op, event, epoll_fd)
    } else {
        SocketStatusCode::EpollCtlFail.into()
    }
}

/// StreamSocketEpollWait
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn StreamSocketEpollWait(
    object: *const EpollManager, events: *mut libc::epoll_event, maxevents: i32, timeout: i32, epoll_fd: i32) -> i32 {
    info!(LOG_LABEL, "enter StreamSocketEpollWait");
    if let Some(obj) = EpollManager::as_ref(object) {
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
        EpollManager::epoll_wait(events, maxevents, timeout, epoll_fd)
    } else {
        SocketStatusCode::EpollWaitFail.into()
    }
}

/// StreamSocketEpollClose
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn StreamSocketEpollClose(object: *mut EpollManager) -> i32 {
    info!(LOG_LABEL, "enter StreamSocketEpollClose");
    if let Some(obj) = EpollManager::as_mut(object) {
        obj.epoll_close();
        SocketStatusCode::Ok.into()
    } else {
        SocketStatusCode::EpollCloseFail.into()
    }
}
/// StreamSocketClose
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn StreamSocketClose(object: *mut EpollManager) -> i32 {
    info!(LOG_LABEL, "enter StreamSocketClose");
    if let Some(obj) = EpollManager::as_mut(object) {
        obj.socket_close();
        SocketStatusCode::Ok.into()
    } else {
        SocketStatusCode::SocketCloseFail.into()
    }
}


