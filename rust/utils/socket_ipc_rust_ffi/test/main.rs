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

//! socket communication example
#![feature(rustc_private)]

extern crate sensor_rust_util;
extern crate libc;
use libc::c_int;
use libc::c_char;
use sensor_rust_util::stream_session::StreamSession;
use sensor_rust_util::net_packet::NetPacket;
use sensor_rust_util::net_packet::MessageId;
use sensor_rust_util::stream_socket::StreamSocket;
use sensor_rust_util::stream_buffer::{StreamBuffer, MAX_PACKET_BUF_SIZE};
use std::mem::size_of;

unsafe fn read_packets_callback(pkt: *mut NetPacket) {
    if let Some(obj) = NetPacket::as_mut(pkt) {
        println!("msg_id={}", obj.msg_id as i32);
        let mut d: i32 = 0;
        let mut e: i32 = 0;
        let mut f: i32 = 0;
        obj.read(&mut d);
        obj.read(&mut e);
        obj.read(&mut f);
        println!("d={},e={},f={}", d, e, f);
    } else {

    }
}

fn main() {
    const MAX_EVENT_SIZE: i32 = 100;
    let mut socket_fds: [c_int; 2] = [-1; 2];
    unsafe {
        if libc::socketpair(libc::AF_UNIX, libc::SOCK_STREAM, 0, &mut socket_fds[0]) != 0 {
            println!("Call socketpair failed");
        }
    }
    let server_fd = socket_fds[0];
    let client_fd = socket_fds[1];
    let buffer_size: libc::size_t = 32 * 1024;

    println!("server_fd={}, client_fd={}", server_fd, client_fd);
    unsafe {
        let pid: libc::pid_t = libc::fork();
        if pid == -1 {
            println!("Error: fork failed.");
        } else if pid == 0 {
            println!("I am the child process, my PID is {}.", libc::getpid());
            libc::close(server_fd);
            if libc::setsockopt(client_fd, libc::SOL_SOCKET, libc::SO_SNDBUF,
                &buffer_size as *const libc::size_t as *const libc::c_void, size_of::<libc::size_t>() as u32) != 0 {
                println!("setsockopt client_fd failed");
            }
            if libc::setsockopt(client_fd, libc::SOL_SOCKET, libc::SO_RCVBUF,
                &buffer_size as *const libc::size_t as *const libc::c_void, size_of::<libc::size_t>() as u32) != 0 {
                println!("setsockopt client_fd failed");
            }
            let mut events: [libc::epoll_event; MAX_EVENT_SIZE as usize] = [
                libc::epoll_event {
                    events: 0,
                    u64: 0,
                }; MAX_EVENT_SIZE as usize
            ];
            let mut stream_socket: StreamSocket = Default::default();
            let mut ev: libc::epoll_event= libc::epoll_event {
                events: 0,
                u64: 0,
            };
            ev.events = libc::EPOLLIN as u32;
            ev.u64 = client_fd as u64;
            stream_socket.create_epoll_fd(MAX_EVENT_SIZE);
            if StreamSocket::epoll_ctl(client_fd, libc::EPOLL_CTL_ADD, &mut ev, stream_socket.epoll_fd) != 0 {
                println!("epoll_ctl fail");
            }
            loop {
                let _count: i32 = StreamSocket::epoll_wait(&mut events[0], MAX_EVENT_SIZE, -1, stream_socket.epoll_fd);
                let mut sz_buf: [c_char; MAX_PACKET_BUF_SIZE] = [0; MAX_PACKET_BUF_SIZE];
                let mut buf: StreamBuffer = Default::default();
                let size = libc::recv(client_fd, &mut sz_buf[0] as *mut c_char as *mut libc::c_void,
                    MAX_PACKET_BUF_SIZE, libc::MSG_DONTWAIT | libc::MSG_NOSIGNAL);
                if !buf.circle_write(&sz_buf[0], size as usize) {
                    println!("Write data failed. size:{}", size);
                }
                buf.read_packets(read_packets_callback);
            }
        } else {
            println!("I am the parent process, my PID is {} and my child's PID is {}", libc::getpid(), pid);
            libc::close(client_fd);
            if libc::setsockopt(server_fd, libc::SOL_SOCKET, libc::SO_SNDBUF,
                &buffer_size as *const libc::size_t as *const libc::c_void, size_of::<libc::size_t>() as u32) != 0 {
                println!("setsockopt server_fd failed");
            }
            if libc::setsockopt(server_fd, libc::SOL_SOCKET, libc::SO_RCVBUF,
                &buffer_size as *const libc::size_t as *const libc::c_void, size_of::<libc::size_t>() as u32) != 0 {
                println!("setsockopt serverFd failed");
            }
            let session: StreamSession = StreamSession {
                module_type: 0,
                fd: server_fd,
                uid : 0,
                pid,
                token_type: 0,
            };
            loop {
                let mut pkt: NetPacket = NetPacket {
                    msg_id: MessageId::DragNotifyResult,
                    ..Default::default()
                };
                let a: i32 = 777;
                let b: i32 = 888;
                let c: i32 = 999;
                pkt.write(a);
                pkt.write(b);
                pkt.write(c);
                session.send_msg_pkt(&pkt);
                libc::usleep(1000000);
            }
        }
    }
}