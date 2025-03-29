module;

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
}

export module ffmpeg.codec:decoder;

import ffmpeg.util;
import ffmpeg.frame;
import ffmpeg.format;

export namespace ffmpeg::codec {

struct AVCodecContextDeleter {
    void operator()(AVCodecContext *ctx) const {
        if (ctx) {
            // avcodec_free_context expects AVCodecContext**
            avcodec_free_context(&ctx);
            // ctx is likely NULL after this call.
        }
    }
};

class Decoder {
public:
    explicit Decoder(AVCodecParameters *params);

    ffmpeg::util::ffmpeg_result<ffmpeg::frame::Frame> decodeNextFrame();
    ffmpeg::util::ffmpeg_result<void>
    sendPacket(ffmpeg::format::Packet &packet);

private:
    bool eof_reached_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> ctx_;
};
} // namespace ffmpeg::codec
