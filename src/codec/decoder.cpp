module;

#include <expected>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_par.h>
}

module ffmpeg.codec;
import ffmpeg.util;
import ffmpeg.frame;
import ffmpeg.format;

namespace ffmpeg::codec {

Decoder::Decoder(AVCodecParameters *params) {
    const AVCodec *codec = avcodec_find_decoder(params->codec_id);
    if (!codec) {
        throw ffmpeg::util::FFmpegError(AVERROR(ENOMEM), "Unsupported codec");
    }

    ctx_ = avcodec_alloc_context3(codec);
    if (!ctx_) {
        throw ffmpeg::util::FFmpegError(AVERROR_INVALIDDATA,
                                        "Failed to allocate codec context");
    }

    int ret = avcodec_parameters_to_context(ctx_, params);
    if (ret < 0) {
        throw std::unexpected(ffmpeg::util::get_ffmpeg_error(ret));
    }

    ret = avcodec_open2(ctx_, codec, nullptr);
    if (ret < 0) {
        throw std::unexpected(ffmpeg::util::get_ffmpeg_error(ret));
    }
}

Decoder::~Decoder() { avcodec_free_context(&ctx_); }

ffmpeg::util::ffmpeg_result<ffmpeg::frame::Frame> Decoder::decodeNextFrame() {
    if (eof_reached_) {
        return std::unexpected(ffmpeg::util::FFmpegEOF{});
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        return std::unexpected(ffmpeg::util::FFmpegError(
            AVERROR(ENOMEM), "Failed to allocate frame."));
    }

    int ret = avcodec_receive_frame(ctx_, frame);
    if (ret == AVERROR(EAGAIN)) {
        return std::unexpected(ffmpeg::util::FFmpegAGAIN{});
    } else if (ret == AVERROR_EOF) {
        eof_reached_ = true;
        return std::unexpected(ffmpeg::util::FFmpegEOF{});
    } else if (ret < 0) {
        return std::unexpected(ffmpeg::util::get_ffmpeg_error(ret));
    }

    return ffmpeg::frame::Frame(frame);
}

ffmpeg::util::ffmpeg_result<void>
Decoder::sendPacket(ffmpeg::format::Packet& packet) {
    int ret = avcodec_send_packet(ctx_, packet.avPacket());
    if (ret < 0) {
        return std::unexpected(ffmpeg::util::get_ffmpeg_error(ret));
    }
    return ffmpeg::util::ffmpeg_result<void>{};
}

} // namespace ffmpeg::codec
