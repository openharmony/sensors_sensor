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

//! Safe Rust interface to OHOS msdp
#![feature(rustc_private)]
#![allow(dead_code)]

extern crate libc;
/// mod stream socket
pub mod stream_socket;
/// mod stream buffer
pub mod stream_buffer;
/// mod stream session
pub mod stream_session;
/// mod net packet
pub mod net_packet;
/// mod binding for binding extern C interface
pub mod binding;
mod error;
/// annotation
pub type Result<T> = std::result::Result<T, i32>;

