module;

#include <expected>
#include <string>
#include <utility>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
}

export module ffmpeg.util:frame;
import ffmpeg.util.error;

export namespace ffmpeg::util {
struct Frame {
    Frame() : frame_(av_frame_alloc()) {}
    ~Frame() { av_frame_free(&frame_); }

    Frame(const Frame &other) : frame_(av_frame_clone(other.frame_)) {}
    auto operator=(const Frame &other) -> Frame & {
        if (&other == this) {
            return *this;
        }
        av_frame_unref(frame_);
        auto ret = av_frame_ref(frame_, other.frame_);
        if (ret < 0) {
            throw get_ffmpeg_error(ret);
        }
        return *this;
    }

    Frame(Frame &&other) noexcept
        : frame_(std::exchange(other.frame_, nullptr)) {}
    auto operator=(Frame &&other) noexcept -> Frame & {
        if (&other == this) {
            return *this;
        }
        av_frame_unref(frame_);
        av_frame_move_ref(frame_, other.frame_);
        return *this;
    }

    [[nodiscard]] auto get() -> AVFrame * { return frame_; }
    [[nodiscard]] auto get() const -> AVFrame const * { return frame_; }

    [[nodiscard]] auto width() const -> int { return frame_->width; }

    [[nodiscard]] auto height() const -> int { return frame_->height; }

    [[nodiscard]] auto format() const -> AVPixelFormat {
        return static_cast<AVPixelFormat>(frame_->format);
    }

    [[nodiscard]] auto sample_aspect_ratio() const -> AVRational {
        return frame_->sample_aspect_ratio;
    }

private:
    AVFrame *frame_;
};
}; // namespace ffmpeg::util
