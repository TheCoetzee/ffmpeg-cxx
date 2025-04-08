module;

#include <chrono>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
}

module ffmpeg.format;

namespace ffmpeg::format {

Demuxer::Demuxer(const std::string &filename) {
    AVFormatContext *format_ctx = nullptr;
    int ret =
        avformat_open_input(&format_ctx, filename.c_str(), nullptr, nullptr);

    if (ret < 0) {
        throw ffmpeg::util::get_ffmpeg_error(ret);
    }

    ret = avformat_find_stream_info(format_ctx, nullptr);
    if (ret < 0) {
        avformat_close_input(&format_ctx);
        throw ffmpeg::util::get_ffmpeg_error(ret);
    }

    ctx_.reset(format_ctx);
}

auto Demuxer::streams() const -> std::vector<Stream> {
    std::vector<Stream> streams;
    std::span raw_streams{ctx_->streams, ctx_->nb_streams};
    for (auto *stream : raw_streams) {
        streams.emplace_back(stream);
    }
    return streams;
}

auto Demuxer::readPacket() -> ffmpeg::util::ffmpeg_result<util::Packet> {
    util::Packet packet;

    int ret = av_read_frame(ctx_.get(), packet.get());

    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            return std::unexpected(ffmpeg::util::FFmpegEOF{});
        }
        return std::unexpected(ffmpeg::util::get_ffmpeg_error(ret));
    }
    return packet;
}
auto Demuxer::streamCount() const -> unsigned int { return ctx_->nb_streams; }

auto Demuxer::bestVideoStreamIndex() const -> int {
    return av_find_best_stream(ctx_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr,
                               0);
}

auto Demuxer::seek(std::chrono::nanoseconds timestamp) -> void {
    auto ret = av_seek_frame(
        ctx_.get(), -1,
        av_rescale_q(timestamp.count(),
                     AVRational{std::chrono::nanoseconds::period::num,
                                std::chrono::nanoseconds::period::den},
                     AV_TIME_BASE_Q),
        0);
    if (ret < 0) {
        throw util::get_ffmpeg_error(ret);
    }
}

} // namespace ffmpeg::format
