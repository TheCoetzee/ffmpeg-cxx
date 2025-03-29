module;

#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
export module ffmpeg.format;

import ffmpeg.util;

export namespace ffmpeg::format {

struct Stream {
    Stream(const AVStream *stream);
    int index() const;
    AVMediaType type() const;
    AVCodecParameters codecParameters() const;
    AVRational timeBase() const;
    AVRational averageFrameRate() const;

private:
    const AVStream *stream_;
};

struct AVPacketDeleter {
    void operator()(AVPacket *pkt) const {
        if (pkt) {
            // av_packet_free expects AVPacket** and frees the packet's data
            // and the packet struct itself if it was allocated via
            // av_packet_alloc.
            av_packet_free(&pkt);
            // pkt is likely NULL after this call, but it doesn't matter
            // as the unique_ptr is managing the memory address.
        }
    }
};

class Packet {
public:
    Packet(AVPacket *packet);

    const AVPacket *avPacket() const;
    int streamIndex() const;

private:
    std::unique_ptr<AVPacket, AVPacketDeleter> packet_;
};

struct AVFormatContextDeleter {
    void operator()(AVFormatContext *ctx) const {
        if (ctx) {
            // avformat_close_input expects AVFormatContext**
            // It potentially NULLs the pointer passed to it.
            AVFormatContext *ptr_to_close = ctx;
            avformat_close_input(&ptr_to_close);
            // We don't need to do anything with ptr_to_close afterwards,
            // the unique_ptr itself will become null when reset or destroyed.
        }
    }
};

struct Demuxer {
    Demuxer(const std::string &filename);

    std::vector<Stream> streams() const;
    ffmpeg::util::ffmpeg_result<Packet> readPacket();
    int streamCount() const;
    int bestVideoStreamIndex() const;

private:
    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> ctx_;
};

} // namespace ffmpeg::format
