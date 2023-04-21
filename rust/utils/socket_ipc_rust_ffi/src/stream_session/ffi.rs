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
use crate::error::SessionStatusCode;
use hilog_rust::{info, hilog, HiLogLabel, LogType};
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "stream_session_ffi"
};

/// get_uid
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_uid(object: *const StreamSession) -> i32 {
    info!(LOG_LABEL, "enter get_uid");
    if let Some(obj) = StreamSession::as_ref(object) {
        obj.uid()
    } else {
        SessionStatusCode::UidFail.into()
    }
}
/// get_pid
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_pid(object: *const StreamSession) -> i32 {
    info!(LOG_LABEL, "enter get_pid");
    if let Some(obj) = StreamSession::as_ref(object) {
        obj.pid()
    } else {
        SessionStatusCode::PidFail.into()
    }
}
/// get module type
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_module_type(object: *const StreamSession) -> i32 {
    info!(LOG_LABEL, "enter get_module_type");
    if let Some(obj) = StreamSession::as_ref(object) {
        obj.module_type()
    } else {
        SessionStatusCode::ModuleTypeFail.into()
    }
}
/// get session fd
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_session_fd(object: *const StreamSession) -> i32 {
    info!(LOG_LABEL, "enter get_session_fd");
    if let Some(obj) = StreamSession::as_ref(object) {
        obj.session_fd()
    } else {
        SessionStatusCode::FdFail.into()
    }
}
/// get token type
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn set_token_type(object: *mut StreamSession, style: i32) -> i32 {
    info!(LOG_LABEL, "enter set_token_type");
    if let Some(obj) = StreamSession::as_mut(object) {
        obj.set_token_type(style);
        SessionStatusCode::Ok.into()
    } else {
        SessionStatusCode::SetTokenTypeFail.into()
    }
}
/// get token type
///
/// # Safety
///
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_token_type(object: *const StreamSession) -> i32 {
    info!(LOG_LABEL, "enter get_token_type");
    if let Some(obj) = StreamSession::as_ref(object) {
        obj.token_type()
    } else {
        SessionStatusCode::TokenTypeFail.into()
    }
}
/// get session close
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn session_close(object: *mut StreamSession) -> i32 {
    info!(LOG_LABEL, "enter session_close");
    if let Some(obj) = StreamSession::as_mut(object) {
        obj.session_close();
        SessionStatusCode::Ok.into()
    } else {
        SessionStatusCode::CloseFail.into()
    }
}

/// session send message
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn session_send_msg(object: *const StreamSession, buf: *const c_char, size: usize) -> bool {
    info!(LOG_LABEL, "enter session_send_msg");
    if let Some(obj) = StreamSession::as_ref(object) {
        obj.session_send_msg(buf, size)
    } else {
        false
    }
}