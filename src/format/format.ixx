module;

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

class Packet {
public:
    Packet(AVPacket *packet);
    ~Packet();

    Packet(Packet&&) = default;
    Packet& operator=(Packet&&) = default;
    Packet(const Packet&) = delete;
    Packet& operator=(const Packet&) = delete;

    const AVPacket *avPacket() const;
    int streamIndex() const;

private:
    AVPacket *packet_;
};

struct Demuxer {
    Demuxer(const std::string &filename);
    ~Demuxer();

    std::vector<Stream> streams() const;
    ffmpeg::util::ffmpeg_result<Packet> readPacket();
    int streamCount() const;
    int bestVideoStreamIndex() const;

private:
    AVFormatContext *ctx_;
};

} // namespace ffmpeg::format
