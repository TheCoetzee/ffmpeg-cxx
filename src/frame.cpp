module;

extern "C" {
#include <libavutil/frame.h>
}

module ffmpeg.frame;

namespace ffmpeg::frame {
Frame::Frame(AVFrame *frame) : frame_(frame) {};

Frame::~Frame() { av_frame_free(&frame_); }
AVFrame *Frame::avFrame() { return frame_; }
}; // namespace ffmpeg::frame
