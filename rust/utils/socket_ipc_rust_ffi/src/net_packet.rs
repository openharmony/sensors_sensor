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
 
/// C++ call function
pub mod ffi;
use std::ffi::{CString, c_char};
use hilog_rust::{hilog, error, HiLogLabel, LogType};
use std::mem::size_of;
use crate::stream_buffer::StreamBuffer;
const STREAM_BUF_WRITE_FAIL: i32 = 2;
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "NetPacket"
};

/// struct PackHead
#[repr(packed(1))]
#[repr(C)]
pub struct PackHead {
    /// NetPacket type
    pub id_msg: MessageId,
    /// SufferBuffer size
    pub size: i32,
}

/// NetPacket type
#[derive(Copy, Clone)]
#[repr(C)]
pub enum MessageId {
    ///
    Invalid = 0,
    ///
    Device,
    ///
    DeviceIds,
    ///
    DeviceSupportKeys,
    ///
    AddDeviceListener,
    ///
    DeviceKeyboardType,
    ///
    DisplayInfo,
    ///
    NoticeAnr,
    ///
    MarkProcess,
    ///
    OnSubscribeKey,
    ///
    OnKeyEvent,
    ///
    OnPointerEvent,
    ///
    ReportKeyEvent,
    ///
    ReportPointerEvent,
    ///
    OnDeviceAdded,
    ///
    OnDeviceRemoved,
    ///
    CoordinationAddListener,
    ///
    CoordinationMessage,
    ///
    CoordinationGetState,

    ///
    DragNotifyResult,
    ///
    DragStateListener,
}

/// struct NetPacket
#[derive(Copy, Clone)]
#[repr(C)]
pub struct NetPacket {
    /// NetPacket head
    pub msg_id: MessageId,
    /// NetPacket streambuffer
    pub stream_buffer: StreamBuffer,
}

impl Default for NetPacket {
    fn default() -> Self {
        Self {
            msg_id: MessageId::Invalid,
            stream_buffer: Default::default(),
        }
    }
}

impl NetPacket {
    /// get refenrance from pointer
    ///
    ///# Safety
    ///
    /// object pointer is valid
    pub unsafe fn as_ref<'a>(object: *const Self) -> Option<&'a Self>{
        object.as_ref()
    }
    /// get mut refenrance from pointer
    ///
    ///# Safety
    ///
    /// object pointer is valid
    pub unsafe fn as_mut<'a>(object: *mut Self) -> Option<&'a mut Self>{
        object.as_mut()
    }
    /// write
    pub fn write<T>(&mut self, data: T) {
        let data: *const c_char = &data as *const T as *const c_char;
        let size = size_of::<T>();
        self.stream_buffer.write_char_usize(data, size);
    }
    /// read
    pub fn read<T>(&mut self, data: &mut T) {
        let data: *mut c_char = data as *mut T as *mut c_char;
        let size = size_of::<T>();
        self.stream_buffer.read_char_usize(data, size);
    }
    /// get_size
    pub fn size(&self) -> usize {
        self.stream_buffer.size()
    }
    /// get_packet_length
    pub fn get_packet_length(&self) -> i32 {
        size_of::<PackHead>() as i32 + self.stream_buffer.w_pos
    }
    /// get_data
    pub fn get_data(&self) -> *const c_char {
        self.stream_buffer.data()
    }
    /// get_msg_id
    pub fn get_msg_id(&self) -> MessageId {
        self.msg_id
    }
    /// make_data
    pub fn make_data(&self, buf: &mut StreamBuffer) {
        let head = PackHead {
            id_msg: self.msg_id,
            size: self.stream_buffer.w_pos,
        };
        buf.write(head);
        if self.stream_buffer.w_pos > 0 && !buf.write_char_usize(&self.stream_buffer.sz_buff[0] as *const c_char,
            self.stream_buffer.w_pos as usize) {
            error!(LOG_LABEL, "Write data to stream failed, errCode:{}", STREAM_BUF_WRITE_FAIL);
        }
    }
}

