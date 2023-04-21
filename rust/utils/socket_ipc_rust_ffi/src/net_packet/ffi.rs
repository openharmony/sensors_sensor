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
use hilog_rust::{info, hilog, HiLogLabel, LogType};
use std::ffi::CString;
use crate::error::NetPacketStatusCode;
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "net_packet_ffi"
};

/// Get size
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_size(object: *const NetPacket) -> usize {
    info!(LOG_LABEL, "enter get_size");
    if let Some(obj) = NetPacket::as_ref(object) {
        obj.size()
    } else {
        0
    }
}
/// Get packet length
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_packet_length(object: *const NetPacket) -> i32 {
    info!(LOG_LABEL, "enter get_packet_length");
    if let Some(obj) = NetPacket::as_ref(object) {
        obj.get_packet_length()
    } else {
        NetPacketStatusCode::PacketLengthFail.into()
    }
}
/// Get data
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_data(object: *const NetPacket) -> *const c_char {
    info!(LOG_LABEL, "enter get_data");
    if let Some(obj) = NetPacket::as_ref(object) {
        obj.get_data()
    } else {
        std::ptr::null()
    }
}
/// Get msg_id
///
/// # Safety
/// 
/// object must be valid
#[no_mangle]
pub unsafe extern "C" fn get_msg_id(object: *const NetPacket) -> MessageId {
    info!(LOG_LABEL, "enter get_msg_id");
    if let Some(obj) = NetPacket::as_ref(object) {
        obj.get_msg_id()
    } else {
        MessageId::Invalid
    }
}
