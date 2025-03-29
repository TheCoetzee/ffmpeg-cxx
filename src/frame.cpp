module;

extern "C" {
#include <libavutil/frame.h>
}

module ffmpeg.frame;

namespace ffmpeg::frame {
Frame::Frame(AVFrame *frame) : frame_(frame) {};

AVFrame *Frame::avFrame() { return frame_.get(); }
}; // namespace ffmpeg::frame
