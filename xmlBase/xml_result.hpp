#ifndef XML_RESULT_HPP
#define XML_RESULT_HPP

#include <variant>
#include <optional>
#include <string>
#include <sstream>  // Added: Required for std::ostringstream in toString()

namespace xml_framework {

enum class XmlErrorCode {
    Success = 0,
    ParseError,
    NodeNotFound,
    AttributeNotFound,
    TypeConversionError,
    ValidationError,
    FileNotFound,
    EmptyDocument,
    Unknown
};

struct XmlError {
    XmlErrorCode code;
    std::string message;
    std::string context;
    size_t line = 0;
    size_t column = 0;

    XmlError() : code(XmlErrorCode::Success) {}
    
    XmlError(XmlErrorCode c, const std::string& msg, 
             const std::string& ctx = "", size_t l = 0, size_t col = 0)
        : code(c), message(msg), context(ctx), line(l), column(col) {}

    bool isSuccess() const { return code == XmlErrorCode::Success; }
    bool isError() const { return code != XmlErrorCode::Success; }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Error[" << static_cast<int>(code) << "]: " << message;
        if (!context.empty()) oss << " (Context: " << context << ")";
        if (line > 0) oss << " at line " << line << ", col " << column;
        return oss.str();
    }
};

// Result type for operations that may fail
template<typename T>
class XmlResult {
public:
    // Success construction
    static XmlResult success(T value) {
        return XmlResult(std::move(value));
    }

    // Error construction
    static XmlResult error(XmlError err) {
        return XmlResult(std::move(err));
    }

    static XmlResult error(XmlErrorCode code, const std::string& msg) {
        return XmlResult(XmlError{code, msg});
    }
    
    // Added: Three-argument error() for consistency with usage in other files
    static XmlResult error(XmlErrorCode code, const std::string& msg, 
                          const std::string& ctx) {
        return XmlResult(XmlError{code, msg, ctx});
    }

    bool isSuccess() const { return std::holds_alternative<T>(data_); }
    bool isError() const { return std::holds_alternative<XmlError>(data_); }

    const T& value() const& { return std::get<T>(data_); }
    T& value() & { return std::get<T>(data_); }
    T&& value() && { return std::get<T>(std::move(data_)); }
    
    const XmlError& error() const { return std::get<XmlError>(data_); }

    // Monadic operations
    template<typename Func>
    auto map(Func&& f) const -> XmlResult<decltype(f(std::declval<T>()))> {
        using ResultType = decltype(f(std::declval<T>()));
        if (isSuccess()) {
            return XmlResult<ResultType>::success(f(value()));
        }
        return XmlResult<ResultType>::error(error());
    }

    T valueOr(T defaultValue) const {
        return isSuccess() ? value() : defaultValue;
    }

private:
    explicit XmlResult(T value) : data_(std::move(value)) {}
    explicit XmlResult(XmlError err) : data_(std::move(err)) {}

    std::variant<T, XmlError> data_;
};

// Specialization for void operations
template<>
class XmlResult<void> {
public:
    static XmlResult success() { return XmlResult(true); }
    
    static XmlResult error(XmlError err) { return XmlResult(std::move(err)); }
    
    // Added: Two-argument error() to match generic template
    static XmlResult error(XmlErrorCode code, const std::string& msg) {
        return XmlResult(XmlError{code, msg});
    }
    
    // Added: Three-argument error() for consistency with usage in other files
    static XmlResult error(XmlErrorCode code, const std::string& msg,
                          const std::string& ctx) {
        return XmlResult(XmlError{code, msg, ctx});
    }

    bool isSuccess() const { return success_; }
    bool isError() const { return !success_; }
    const XmlError& error() const { return error_; }

private:
    explicit XmlResult(bool) : success_(true) {}
    explicit XmlResult(XmlError err) : success_(false), error_(std::move(err)) {}

    bool success_ = false;
    XmlError error_;
};

} // namespace xml_framework

#endif // XML_RESULT_HPP
