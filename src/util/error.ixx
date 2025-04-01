module;
#include <expected>
#include <format>
#include <stdexcept>

extern "C" {
#include <libavutil/avutil.h>
}

export module ffmpeg.util.error;

export namespace ffmpeg::util {
class FFmpegError : public std::runtime_error {
public:
    FFmpegError(int errCode, const std::string &message)
        : std::runtime_error(
              std::format("FFmpeg error {}: {}", errCode, message)),
          errorCode_(errCode) {}

    [[nodiscard]] auto errorCode() const -> int { return errorCode_; }

private:
    int errorCode_;
};

struct FFmpegEOF {};
struct FFmpegAGAIN {};

using FFmpegErrors = std::variant<FFmpegError, FFmpegEOF, FFmpegAGAIN>;

template <typename T> using ffmpeg_result = std::expected<T, FFmpegErrors>;

template <typename T>
auto check_ffmpeg_result(int errCode, T value) -> ffmpeg_result<T> {
    if (errCode >= 0) {
        return value;
    }
    std::array<char, AV_ERROR_MAX_STRING_SIZE> errbuf{0};
    av_strerror(errCode, errbuf.data(), errbuf.size());
    return std::unexpected(FFmpegError(errCode, errbuf.data()));
}


template <typename T>
auto get_result_value(ffmpeg_result<T> result) -> T {
    if (!result.has_value()) {
        throw result.error();
    }
    return result.value();
}

auto check_ffmpeg_result(int errCode) -> ffmpeg_result<void> {
    if (errCode >= 0) {
        return {};
    }
    std::array<char, AV_ERROR_MAX_STRING_SIZE> errbuf{0};
    av_strerror(errCode, errbuf.data(), errbuf.size());
    return std::unexpected(FFmpegError(errCode, errbuf.data()));
}

auto get_ffmpeg_error(int errCode) -> FFmpegError {
    std::array<char, AV_ERROR_MAX_STRING_SIZE> errbuf{0};
    av_strerror(errCode, errbuf.data(), errbuf.size());
    return {errCode, errbuf.data()};
}

void throw_on_ffmpeg_error(int errCode, const std::string &contextMessage) {
    if (errCode < 0) {
        std::array<char, AV_ERROR_MAX_STRING_SIZE> errbuf{0};
        av_strerror(errCode, errbuf.data(), errbuf.size());
        throw FFmpegError(errCode,
                          std::format("{}: {}", contextMessage, errbuf.data()));
    }
}
} // namespace ffmpeg::util
