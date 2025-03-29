module;

#include <expected>
#include <format>
#include <span>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
}

module ffmpeg.format;

namespace ffmpeg::format {

Stream::Stream(const AVStream *stream) : stream_(stream) {}

int Stream::index() const { return stream_->index; }

AVMediaType Stream::type() const { return stream_->codecpar->codec_type; }

AVCodecParameters Stream::codecParameters() const { return *stream_->codecpar; }

AVRational Stream::timeBase() const { return stream_->time_base; }

AVRational Stream::averageFrameRate() const { return stream_->avg_frame_rate; }

Packet::Packet(AVPacket *packet) : packet_(packet) {}


const AVPacket *Packet::avPacket() const { return packet_.get(); }

int Packet::streamIndex() const { return packet_->stream_index; }

Demuxer::Demuxer(const std::string &filename) {
    AVFormatContext * format_ctx = nullptr;
    int ret = avformat_open_input(&format_ctx, filename.c_str(), nullptr, nullptr);

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

std::vector<Stream> Demuxer::streams() const {
    std::vector<Stream> streams;
    for (unsigned int i = 0; i < ctx_->nb_streams; ++i) {
        streams.emplace_back(ctx_->streams[i]);
    }
    return streams;
}

ffmpeg::util::ffmpeg_result<Packet> Demuxer::readPacket() {
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return std::unexpected(ffmpeg::util::FFmpegError(
            AVERROR(ENOMEM), "Failed to allocate packet."));
    }

    int ret = av_read_frame(ctx_.get(), packet);

    if (ret < 0) {
        av_packet_free(&packet);
        if (ret == AVERROR_EOF) {
            return std::unexpected(ffmpeg::util::FFmpegEOF{});
        }
        return std::unexpected(ffmpeg::util::get_ffmpeg_error(ret));
    }
    return Packet(packet);
}
int Demuxer::streamCount() const { return ctx_->nb_streams; }

int Demuxer::bestVideoStreamIndex() const {
    return av_find_best_stream(ctx_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr,
                               0);
}

} // namespace ffmpeg::format
