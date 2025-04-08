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
    constexpr explicit Stream(const AVStream *stream) : stream_(stream) {}
    [[nodiscard]] constexpr auto index() const -> int { return stream_->index; }
    [[nodiscard]] constexpr auto type() const -> AVMediaType {
        return stream_->codecpar->codec_type;
    }
    [[nodiscard]] constexpr auto codecParameters() const
        -> AVCodecParameters * {
        return stream_->codecpar;
    }
    [[nodiscard]] constexpr auto timeBase() const -> AVRational {
        return stream_->time_base;
    }
    [[nodiscard]] constexpr auto averageFrameRate() const -> AVRational {
        return stream_->avg_frame_rate;
    }

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
