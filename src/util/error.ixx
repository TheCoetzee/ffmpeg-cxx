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

    int errorCode() const { return errorCode_; }

private:
    int errorCode_;
};

struct FFmpegEOF {};
struct FFmpegAGAIN {};

using FFmpegErrors = std::variant<FFmpegError, FFmpegEOF, FFmpegAGAIN>;

template <typename T> using ffmpeg_result = std::expected<T, FFmpegErrors>;

template <typename T>
ffmpeg_result<T> check_ffmpeg_result(int errCode, T value) {
    if (errCode >= 0) {
        return value;
    } else {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(errCode, errbuf, sizeof(errbuf));
        return std::unexpected(FFmpegError(errCode, errbuf));
    }
}

ffmpeg_result<void> check_ffmpeg_result(int errCode) {
    if (errCode >= 0) {
        return {};
    } else {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(errCode, errbuf, sizeof(errbuf));
        return std::unexpected(FFmpegError(errCode, errbuf));
    }
}

FFmpegError get_ffmpeg_error(int errCode) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errCode, errbuf, sizeof(errbuf));
    return FFmpegError(errCode, errbuf);
}

void throw_on_ffmpeg_error(int errCode, const std::string &contextMessage) {
    if (errCode < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(errCode, errbuf, sizeof(errbuf));
        throw FFmpegError(errCode,
                          std::format("{}: {}", contextMessage, errbuf));
    }
}
} // namespace ffmpeg::util
