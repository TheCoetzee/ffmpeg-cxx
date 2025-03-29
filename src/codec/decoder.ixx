module;
extern "C" {
#include <libavcodec/avcodec.h>
}

export module ffmpeg.codec:decoder;

import ffmpeg.util;
import ffmpeg.frame;
import ffmpeg.format;

export namespace ffmpeg::codec {

class Decoder {
public:
    explicit Decoder(AVCodecParameters *params);
    ~Decoder();

    ffmpeg::util::ffmpeg_result<ffmpeg::frame::Frame> decodeNextFrame();
    ffmpeg::util::ffmpeg_result<void> sendPacket(ffmpeg::format::Packet& packet);

private:
    bool eof_reached_;
    AVCodecContext *ctx_;
};
} // namespace ffmpeg::codec
