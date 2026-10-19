#pragma once

#include <string>
#include <utility>

namespace aester {

enum class ExitCode : int {
    Ok = 0,
    GeneralError = 1,
    InvalidArgs = 2,
    NotSupported = 3,
    WriteOrVerifyFailed = 4
};

struct Error {
    ExitCode code;
    std::string message;
};

template <typename T>
class Result {
public:
    Result(const T& val) : is_ok_(true), value_(val), error_{ExitCode::Ok, ""} {}
    Result(T&& val) : is_ok_(true), value_(std::move(val)), error_{ExitCode::Ok, ""} {}
    Result(Error err) : is_ok_(false), value_{}, error_(std::move(err)) {}
    Result(ExitCode code, std::string msg) : is_ok_(false), value_{}, error_{code, std::move(msg)} {}

    bool ok() const { return is_ok_; }
    const T& value() const { return value_; }
    T& value() { return value_; }
    const Error& error() const { return error_; }
    ExitCode code() const { return is_ok_ ? ExitCode::Ok : error_.code; }
    const std::string& message() const { return error_.message; }

    static Result<T> success(T val) { return Result<T>(std::move(val)); }
    static Result<T> failure(ExitCode code, std::string msg) { return Result<T>(code, std::move(msg)); }

private:
    bool is_ok_;
    T value_;
    Error error_;
};

template <>
class Result<void> {
public:
    Result() : is_ok_(true), error_{ExitCode::Ok, ""} {}
    Result(Error err) : is_ok_(false), error_(std::move(err)) {}
    Result(ExitCode code, std::string msg) : is_ok_(false), error_{code, std::move(msg)} {}

    bool ok() const { return is_ok_; }
    const Error& error() const { return error_; }
    ExitCode code() const { return is_ok_ ? ExitCode::Ok : error_.code; }
    const std::string& message() const { return error_.message; }

    static Result<void> success() { return Result<void>(); }
    static Result<void> failure(ExitCode code, std::string msg) { return Result<void>(code, std::move(msg)); }

private:
    bool is_ok_;
    Error error_;
};

} // namespace aester
