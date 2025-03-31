module;

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
}

export module ffmpeg.codec:decoder;

import ffmpeg.util;
import ffmpeg.format;

export namespace ffmpeg::codec {

struct AVCodecContextDeleter {
    void operator()(AVCodecContext *ctx) const {
        if (ctx != nullptr) {
            avcodec_free_context(&ctx);
        }
    }
};

class Decoder {
public:
    explicit Decoder(AVCodecParameters *params);

    auto decodeNextFrame() -> ffmpeg::util::ffmpeg_result<util::Frame>;
    auto sendPacket(util::Packet &packet) -> ffmpeg::util::ffmpeg_result<void>;

private:
    bool eof_reached_;
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> ctx_;
};
} // namespace ffmpeg::codec
