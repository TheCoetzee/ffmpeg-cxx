module;

#include <expected>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_par.h>
}

module ffmpeg.codec;
import ffmpeg.util;
import ffmpeg.format;

namespace ffmpeg::codec {

Decoder::Decoder(const AVCodecParameters *params) {
    const AVCodec *codec = avcodec_find_decoder(params->codec_id);
    if (codec == nullptr) {
        throw ffmpeg::util::FFmpegError(AVERROR(ENOMEM), "Unsupported codec");
    }

    AVCodecContext *context_raw = avcodec_alloc_context3(codec);
    if (context_raw == nullptr) {
        throw ffmpeg::util::FFmpegError(AVERROR_INVALIDDATA,
                                        "Failed to allocate codec context");
    }
    ctx_.reset(context_raw);
    int ret = avcodec_parameters_to_context(ctx_.get(), params);
    if (ret < 0) {
        throw ffmpeg::util::get_ffmpeg_error(ret);
    }

    ctx_->thread_count = 0;
    if ((codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) != 0) {
        ctx_->thread_type = FF_THREAD_SLICE;
    }
    ret = avcodec_open2(ctx_.get(), codec, nullptr);
    if (ret < 0) {
        throw ffmpeg::util::get_ffmpeg_error(ret);
    }
    eof_reached_ = false;
}

auto Decoder::decodeNextFrame() -> ffmpeg::util::ffmpeg_result<util::Frame> {
    if (eof_reached_) {
        return std::unexpected(ffmpeg::util::FFmpegEOF{});
    }

    util::Frame frame;

    int ret = avcodec_receive_frame(ctx_.get(), frame.get());
    if (ret == AVERROR(EAGAIN)) {
        return std::unexpected(ffmpeg::util::FFmpegAGAIN{});
    }
    if (ret == AVERROR_EOF) {
        eof_reached_ = true;
        return std::unexpected(ffmpeg::util::FFmpegEOF{});
    }
    if (ret < 0) {
        return std::unexpected(ffmpeg::util::get_ffmpeg_error(ret));
    }

    return frame;
}

auto Decoder::sendPacket(util::Packet &packet)
    -> ffmpeg::util::ffmpeg_result<void> {
    int ret = avcodec_send_packet(ctx_.get(), packet.get());
    if (ret < 0) {
        return std::unexpected(ffmpeg::util::get_ffmpeg_error(ret));
    }
    return ffmpeg::util::ffmpeg_result<void>{};
}

} // namespace ffmpeg::codec
