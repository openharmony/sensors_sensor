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
use crate::error::BufferStatusCode;
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "stream_buffer_ffi"
};
/// data
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn data(object: *const StreamBuffer) -> *const c_char {
    info!(LOG_LABEL, "enter data");
    if let Some(obj) = StreamBuffer::as_ref(object) {
        obj.data()
    } else {
        std::ptr::null()
    }
}
/// size
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn size(object: *const StreamBuffer) -> usize {
    info!(LOG_LABEL, "enter size");
    if let Some(obj) = StreamBuffer::as_ref(object) {
        obj.size()
    } else {
        0
    }
}
/// reset
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn reset(object: *mut StreamBuffer) -> i32 {
    info!(LOG_LABEL, "enter reset");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.reset();
        BufferStatusCode::Ok.into()
    } else {
        BufferStatusCode::ResetFail.into()
    }
}
/// clean
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn clean(object: *mut StreamBuffer) -> i32 {
    info!(LOG_LABEL, "enter clean");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.clean();
        BufferStatusCode::Ok.into()
    } else {
        BufferStatusCode::CleanFail.into()
    }
}
/// unread_size
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn unread_size(object: *mut StreamBuffer) -> i32 {
    info!(LOG_LABEL, "enter unread_size");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.unread_size()
    } else {
        BufferStatusCode::UnreadSizeFail.into()
    }
}
/// is_empty
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn is_empty(object: *const StreamBuffer) -> bool {
    info!(LOG_LABEL, "enter is_empty");
    if let Some(obj) = StreamBuffer::as_ref(object) {
        obj.is_empty()
    } else {
        false
    }
}
/// write_streambuffer
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn write_streambuffer(object: *mut StreamBuffer, buf: *const StreamBuffer) -> bool {
    info!(LOG_LABEL, "enter write_streambuffer");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        if let Some(buffer) = StreamBuffer::as_ref(buf) {

            obj.write_streambuffer(buffer)
        } else {
            false
        }
    } else {
        false
    }
}
/// read_streambuffer
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn read_streambuffer(object: *const StreamBuffer, buf: *mut StreamBuffer) -> bool {
    info!(LOG_LABEL, "enter read_streambuffer");
    if let Some(obj) = StreamBuffer::as_ref(object) {
        if let Some(buffer) = StreamBuffer::as_mut(buf) {
            obj.read_streambuffer(buffer)
        } else {
            false
        }
    } else {
        false
    }
}
/// chk_rwerror
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn chk_rwerror(object: *const StreamBuffer) -> bool {
    if let Some(obj) = StreamBuffer::as_ref(object) {
        obj.chk_rwerror()
    } else {
        false
    }
}
/// get_error_status_remark
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn get_error_status_remark(object: *const StreamBuffer) -> *const c_char {
    info!(LOG_LABEL, "enter get_error_status_remark");
    if let Some(obj) = StreamBuffer::as_ref(object) {
        obj.get_error_status_remark()
    } else {
        std::ptr::null()
    }
}
/// write_char_usize
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn write_char_usize(object: *mut StreamBuffer, buf: *const c_char, size: usize) -> bool {
    info!(LOG_LABEL, "enter write_char_usize");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.write_char_usize(buf, size)
    } else {
        false
    }
}
/// check_write
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn check_write(object: *mut StreamBuffer, size: usize) -> bool {
    info!(LOG_LABEL, "enter check_write");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.check_write(size)
    } else {
        false
    }
}
/// copy_data_to_begin
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn copy_data_to_begin(object: *mut StreamBuffer) -> i32 {
    info!(LOG_LABEL, "enter copy_data_to_begin");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.copy_data_to_begin();
        BufferStatusCode::Ok.into()
    } else {
        BufferStatusCode::CopyDataToBeginFail.into()
    }
}
/// read_char_usize
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn read_char_usize(object: *mut StreamBuffer, buf: *const c_char, size: usize) -> bool {
    info!(LOG_LABEL, "enter read_char_usize");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.read_char_usize(buf, size)
    } else {
        false
    }
}
/// circle_write
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn circle_write(object: *mut StreamBuffer, buf: *const c_char, size: usize) -> bool {
    info!(LOG_LABEL, "enter circle_write");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.circle_write(buf, size)
    } else {
        false
    }
}
/// read_server_packets
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn read_server_packets(object: *mut StreamBuffer, stream_server: *const CStreamServer,
    fd: i32, callback_fun: ServerPacketCallBackFun) -> i32 {
    info!(LOG_LABEL,"enter read_server_packets");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.read_server_packets(stream_server, fd, callback_fun);
        BufferStatusCode::Ok.into()
    } else {
        BufferStatusCode::ReadServerPacketsFail.into()
    }
}
/// read_client_packets
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn read_client_packets(object: *mut StreamBuffer, stream_client: *const CClient,
    callback_fun: ClientPacketCallBackFun) -> i32 {
    info!(LOG_LABEL,"enter read_client_packets");
    if let Some(obj) = StreamBuffer::as_mut(object) {
        obj.read_client_packets(stream_client, callback_fun);
        BufferStatusCode::Ok.into()
    } else {
        BufferStatusCode::ReadClientPacketsFail.into()
    }
}
/// read_buf
///
/// # Safety
/// 
/// object is valid
#[no_mangle]
pub unsafe extern "C" fn read_buf(object: *const StreamBuffer) -> *const c_char {
    if let Some(obj) = StreamBuffer::as_ref(object) {
        obj.read_buf()
    } else {
        std::ptr::null()
    }
}

