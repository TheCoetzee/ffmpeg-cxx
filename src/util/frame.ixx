module;

#include <utility>

extern "C" {
#include <libavutil/frame.h>
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

    Frame(Frame &&other) noexcept : frame_(std::exchange(other.frame_, nullptr)) {}
    auto operator=(Frame &&other) noexcept -> Frame & {
        av_frame_unref(frame_);
        av_frame_move_ref(frame_, other.frame_);
        return *this;
    }

    auto get() -> AVFrame * { return frame_; }

private:
    AVFrame *frame_;
};
}; // namespace ffmpeg::util
