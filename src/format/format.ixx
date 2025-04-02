module;

#include <chrono>
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
export module ffmpeg.format;

import ffmpeg.util;

export namespace ffmpeg::format {

struct Stream {
    explicit Stream(const AVStream *stream);
    [[nodiscard]] auto index() const -> int;
    [[nodiscard]] auto type() const -> AVMediaType;
    [[nodiscard]] auto codecParameters() const -> AVCodecParameters;
    [[nodiscard]] auto timeBase() const -> AVRational;
    [[nodiscard]] auto averageFrameRate() const -> AVRational;

private:
    const AVStream *stream_;
};

struct AVFormatContextDeleter {
    void operator()(AVFormatContext *ctx) const {
        if (ctx != nullptr) {
            // avformat_close_input expects AVFormatContext**
            // It potentially NULLs the pointer passed to it.
            AVFormatContext *ptr_to_close = ctx;
            avformat_close_input(&ptr_to_close);
            // We don't need to do anything with ptr_to_close afterwards,
            // the unique_ptr itself will become null when reset or destroyed.
        }
    }
};

struct Demuxer {
    explicit Demuxer(const std::string &filename);

    [[nodiscard]] auto streams() const -> std::vector<Stream>;
    auto readPacket() -> util::ffmpeg_result<util::Packet>;
    [[nodiscard]] auto streamCount() const -> unsigned int;
    [[nodiscard]] auto bestVideoStreamIndex() const -> int;
    auto seek(std::chrono::nanoseconds timestamp) -> void;

private:
    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> ctx_;
};

} // namespace ffmpeg::format
