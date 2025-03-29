module;

#include <memory>

extern "C" {
#include <libavutil/frame.h>
}

export module ffmpeg.frame;

export namespace ffmpeg::frame {
struct AVFrameDeleter {
    void operator()(AVFrame *frame) const {
        if (frame) {
            // av_frame_free expects AVFrame** and frees the frame struct
            // and any data buffers it references (if ref-counted).
            av_frame_free(&frame);
            // frame is likely NULL after this call.
        }
    }
};

class Frame {
public:
    Frame(AVFrame *frame);

    AVFrame *avFrame();

private:
    std::unique_ptr<AVFrame, AVFrameDeleter> frame_;
};
}; // namespace ffmpeg::frame
