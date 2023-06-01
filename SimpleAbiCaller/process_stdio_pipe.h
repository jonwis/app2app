#pragma once

#include <string>
#include <string_view>
#include <mutex>
#include <variant>
#include <optional>
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <winrt/base.h>

using namespace std::literals;

struct process_string_channel
{
    bool try_start(std::wstring arguments)
    {
        return try_start(std::nullopt, std::move(arguments));
    }

    bool try_start(std::optional<std::wstring> imageFilePath, std::wstring arguments)
    {
        // Windows uses pipes to communicate over stdin/stdout. Create them, marked so they can
        // be inherited by a child process. Put the "right" half of the pipe into the startup
        // info for the new process to use.
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;
        THROW_IF_WIN32_BOOL_FALSE(::CreatePipe(&m_stdOut[pipe_read], &m_stdOut[pipe_write], &sa, 0));
        THROW_IF_WIN32_BOOL_FALSE(::SetHandleInformation(m_stdOut[pipe_read].get(), HANDLE_FLAG_INHERIT, 0));
        THROW_IF_WIN32_BOOL_FALSE(::CreatePipe(&m_stdIn[pipe_read], &m_stdIn[pipe_write], &sa, 0));
        THROW_IF_WIN32_BOOL_FALSE(::SetHandleInformation(m_stdIn[pipe_write].get(), HANDLE_FLAG_INHERIT, 0));

        STARTUPINFOW startupInfo = { sizeof(startupInfo) };
        startupInfo.hStdError = m_stdOut[pipe_write].get();
        startupInfo.hStdOutput = m_stdOut[pipe_write].get();
        startupInfo.hStdInput = m_stdIn[pipe_read].get();
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;

        // Note the 'dangerous' const cast here; CreateProcess may write to this buffer, but
        // only ever inserting and removing null-character markers within its body. The buffer
        // is de-modified before it returns.
        wchar_t const* filePath = imageFilePath ? imageFilePath.value().c_str() : nullptr;
        if (!::CreateProcessW(filePath, const_cast<wchar_t*>(arguments.c_str()), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &m_processInfo))
        {
            m_error = HRESULT_FROM_WIN32(GetLastError());
            return false;
        }

        // Start a read/write thread
        m_inputPumpThread = std::thread([&] { this->read_input_thread(); });
        return true;
    }

    winrt::hresult error() const
    {
        return this->m_error;
    }

    void close(bool waitForProcessEnd = false)
    {
        m_stdIn[pipe_read].reset();
        m_stdIn[pipe_write].reset();
        // m_stdOut[pipe_read].reset(); <<-- this hangs?
        m_stdOut[pipe_write].reset();

        if (waitForProcessEnd && m_processInfo.hProcess)
        {
            WaitForSingleObject(m_processInfo.hProcess, INFINITE);
        }

        // Close all the pipes; this will cause the input read thread to break as well
        if (m_inputPumpThread.joinable())
        {
            m_inputPumpThread.join();
        }
        m_processInfo.reset();
    }

    void send(std::string_view content)
    {
        DWORD written;
        THROW_IF_WIN32_BOOL_FALSE(WriteFile(m_stdIn[pipe_write].get(), content.data(), content.size(), &written, nullptr));
    }

    void sendLine(std::string_view content)
    {
        send(content);
        send("\r\n"sv);
    }

    // Override this if you want a callback as lines are produced byt the reader. Note that the
    // callbacks happen from an std::thread that's not yours. You will need to deal with handing
    // back the responses appropriately.
    virtual void notify_line(std::string_view nextLine) {}

private:

    void read_input_thread()
    {
        const auto chunk_size = 12;
        auto buffer = std::make_unique<char[]>(chunk_size);
        std::string pending;
        DWORD readBytes;
        while (ReadFile(m_stdOut[pipe_read].get(), buffer.get(), chunk_size, &readBytes, nullptr) && (readBytes != 0))
        {
            pending.insert(pending.end(), buffer.get(), buffer.get() + readBytes);
            pump_pending_lines(pending);
        }

        // Finish pumping anything left
        pump_pending_lines(pending);

        // ... and send anything left over
        if (!pending.empty())
        {
            // TODO: this should probably remove trailing \r\n from the pending string...
            notify_line(pending);
        }
    }

    void pump_pending_lines(std::string& src)
    {
        while (!src.empty())
        {
            auto offset = src.find("\r\n");
            if (offset == src.npos)
            {
                return;
            }
            std::string_view chunk(src.data(), offset);
            if (!chunk.empty())
            {
                notify_line(chunk);
            }
            src.erase(0, offset + 2);
        }
    }

    winrt::hresult m_error;
    const int pipe_read = 0;
    const int pipe_write = 1;
    wil::unique_handle m_stdIn[2];
    wil::unique_handle m_stdOut[2];
    std::thread m_inputPumpThread;
    wil::unique_process_information m_processInfo;
};

struct send_recv_process : process_string_channel
{
    std::optional<std::string> send_one(std::string_view content)
    {
        auto lock = std::unique_lock(m_lock);
        sendLine(content);
        m_cv.wait(lock, [this] { return m_pending.has_value(); });
        return std::exchange(m_pending, {});
    }

    void notify_line(std::string_view line) override
    {
        auto lock = std::unique_lock(m_lock);
        m_pending = line;
        lock.unlock();
        m_cv.notify_one();
    }

private:
    std::mutex m_lock;
    std::condition_variable m_cv;
    std::optional<std::string> m_pending;
};

inline std::variant<std::monostate, std::string, winrt::hresult> launch_and_get_one_response(
    std::optional<std::wstring> imagePath,
    std::wstring arguments,
    std::string_view toSend)
{
    send_recv_process childProcess;
    std::optional<std::string> response;
    if (childProcess.try_start(std::move(imagePath), std::move(arguments)))
    {
        auto response = childProcess.send_one(toSend);
        childProcess.close();
        if (response)
        {
            return std::move(response.value());
        }
        else
        {
            return std::monostate{};
        }
    }
    else
    {
        return childProcess.error();
    }
}