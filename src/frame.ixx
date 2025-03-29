module;

extern "C" {
#include <libavutil/frame.h>
}

export module ffmpeg.frame;

export namespace ffmpeg::frame {
class Frame {
public:
    Frame(AVFrame *frame);
	~Frame();

	AVFrame* avFrame();
private:
	AVFrame* frame_;
};
}; // namespace ffmpeg::frame
