/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef RUST_BINDING_H
#define RUST_BINDING_H
#include <stdint.h>
#include <string.h>
#include "proto.h"
#include "accesstoken_kit.h"

extern "C" {
    enum class ErrorStatus : int32_t {
        ERROR_STATUS_Ok = 0,
        ERROR_STATUS_Read = 1,
        ERROR_STATUS_Write = 2,
    };
    struct RustStreamSocket {
        int32_t fd_ { -1 };
        int32_t epollFd_ { -1 };
    };
    struct RustStreamSession {
        int32_t moduleType_ { -1 };
        int32_t fd_ { -1 };
        int32_t uid_ { -1 };
        int32_t pid_ { -1 };
        int32_t tokenType_ { OHOS::Security::AccessToken::ATokenTypeEnum::TOKEN_INVALID };
    };
    struct RustStreamClient {
        bool isExit { false };
        bool isRunning_ { false };
        bool isConnected_ { false };
    };
    struct RustStreamBuffer {
        ErrorStatus rwErrorStatus_ { ErrorStatus::ERROR_STATUS_Ok };
        int32_t rCount_ { 0 };
        int32_t wCount_ { 0 };
        int32_t rPos_ { 0 };
        int32_t wPos_ { 0 };
        char szBuff_[OHOS::Sensors::MAX_STREAM_BUF_SIZE+1] { };
    };
    struct RustNetPacket {
        OHOS::Sensors::MessageId msgId_ { OHOS::Sensors::MessageId::INVALID };
        struct RustStreamBuffer rustStreamBuffer;
    };
    int32_t get_fd(const RustStreamSocket*);
    int32_t get_epoll_fd(const RustStreamSocket*);
    int32_t rust_epoll_create(RustStreamSocket*, int32_t);
    int32_t rust_epoll_ctl(RustStreamSocket*, int32_t, int32_t, struct epoll_event*, int32_t);
    int32_t rust_epoll_wait(RustStreamSocket*, struct epoll_event*, int32_t, int32_t, int32_t);
    int32_t rust_epoll_close(RustStreamSocket*);
    int32_t rust_close(RustStreamSocket*);
    bool send_msg_buf_size(const RustStreamSocket*, const char*, size_t);
    void session_close(RustStreamSession*);
    bool session_send_msg(const RustStreamSession*, const char*, size_t);
    bool get_connected_status(const RustStreamClient&);
    void stop(RustStreamClient&, RustStreamSocket&);
    void reset(RustStreamBuffer*);
    void clean(RustStreamBuffer*);
    bool read_streambuffer(RustStreamBuffer*, RustStreamBuffer*);
    bool write_streambuffer(RustStreamBuffer*, const RustStreamBuffer*);
    const char* read_buf(const RustStreamBuffer*);
    bool read_char_usize(RustStreamBuffer*, char *, size_t);
    bool write_char_usize(RustStreamBuffer*, const char*, size_t);
    const char* data(const RustStreamBuffer*);
    size_t size(const RustStreamBuffer*);
    int32_t unread_size(RustStreamBuffer*);
    int32_t get_available_buf_size(RustStreamBuffer*);
    const char* get_error_status_remark(const RustStreamBuffer*);
    bool chk_rwerror(const RustStreamBuffer*);
    bool check_write(RustStreamBuffer*, size_t);
    bool circle_write(RustStreamBuffer*, const char*, size_t);
    void copy_data_to_begin(RustStreamBuffer*);
    int32_t get_uid(const RustStreamSession*);
    int32_t get_pid(const RustStreamSession*);
    int32_t get_module_type(const RustStreamSession*);
    int32_t get_session_fd(const RustStreamSession*);
    void set_token_type(RustStreamSession*, int32_t type);
    int32_t get_token_type(const RustStreamSession*);
    bool seek_read_pos(RustStreamBuffer*, int32_t);
    const char* read_buf(const RustStreamBuffer*);
    bool is_empty(const RustStreamBuffer*);
    size_t get_size(const RustNetPacket*);
    int32_t get_packet_length(const RustNetPacket*);
    const char* get_data(const RustNetPacket*);
    OHOS::Sensors::MessageId get_msg_id(const RustNetPacket*);
}
#endif // RUST_BINDING_H