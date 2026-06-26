/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SSL_STREAM_H__
#define __SSL_STREAM_H__

#include "Socket.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/system/error_code.hpp>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>

// Thin wrapper over boost::asio::ssl::stream that exposes the same tcp::socket-like
// interface expected by the stream-templated Socket<T, Stream> base (see Socket.h).
// The TLS payload is handled by the ssl::stream, while connection-level operations
// (close, shutdown, remote_endpoint, set_option, async_wait) are forwarded to the
// underlying TCP socket via next_layer().
template<class WrappedStream = IoContextTcpSocket>
class SslStream
{
public:
    using executor_type = typename WrappedStream::executor_type;

    explicit SslStream(IoContextTcpSocket&& socket, boost::asio::ssl::context& sslContext) : _sslSocket(std::move(socket), sslContext)
    {
        _sslSocket.set_verify_mode(boost::asio::ssl::verify_none);
    }

    explicit SslStream(boost::asio::io_context& context, boost::asio::ssl::context& sslContext) : _sslSocket(context, sslContext)
    {
        _sslSocket.set_verify_mode(boost::asio::ssl::verify_none);
    }

    // adapting tcp::socket api
    boost::asio::io_context::executor_type get_executor()
    {
        return _sslSocket.get_executor();
    }

    [[nodiscard]] bool is_open() const
    {
        return _sslSocket.next_layer().is_open();
    }

    void close(boost::system::error_code& error)
    {
        _sslSocket.next_layer().close(error);
    }

    void shutdown(boost::asio::socket_base::shutdown_type what, boost::system::error_code& shutdownError)
    {
        _sslSocket.shutdown(shutdownError);
        _sslSocket.next_layer().shutdown(what, shutdownError);
    }

    template<typename MutableBufferSequence, typename ReadHandlerType>
    decltype(auto) async_read_some(MutableBufferSequence const& buffers, ReadHandlerType&& handler)
    {
        return _sslSocket.async_read_some(buffers, std::forward<ReadHandlerType>(handler));
    }

    template<typename ConstBufferSequence, typename WriteHandlerType>
    decltype(auto) async_write_some(ConstBufferSequence const& buffers, WriteHandlerType&& handler)
    {
        return _sslSocket.async_write_some(buffers, std::forward<WriteHandlerType>(handler));
    }

    template<typename ConstBufferSequence>
    std::size_t write_some(ConstBufferSequence const& buffers, boost::system::error_code& error)
    {
        return _sslSocket.write_some(buffers, error);
    }

    template<typename WaitHandlerType>
    decltype(auto) async_wait(boost::asio::socket_base::wait_type type, WaitHandlerType&& handler)
    {
        return _sslSocket.next_layer().async_wait(type, std::forward<WaitHandlerType>(handler));
    }

    template<typename SettableSocketOption>
    void set_option(SettableSocketOption const& option, boost::system::error_code& error)
    {
        _sslSocket.next_layer().set_option(option, error);
    }

    IoContextTcpSocket::endpoint_type remote_endpoint() const
    {
        return _sslSocket.next_layer().remote_endpoint();
    }

    // ssl api
    template<typename HandshakeHandlerType>
    decltype(auto) async_handshake(boost::asio::ssl::stream_base::handshake_type type, HandshakeHandlerType&& handler)
    {
        return _sslSocket.async_handshake(type, std::forward<HandshakeHandlerType>(handler));
    }

    void set_server_name(std::string const& serverName, boost::system::error_code& error)
    {
        if (!SSL_set_tlsext_host_name(_sslSocket.native_handle(), serverName.c_str()))
            error.assign(static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category());
    }

protected:
    boost::asio::ssl::stream<WrappedStream> _sslSocket;
};

#endif // __SSL_STREAM_H__
