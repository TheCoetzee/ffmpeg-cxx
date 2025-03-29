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
    util::ffmpeg_result<util::Packet> readPacket();
    int streamCount() const;
    int bestVideoStreamIndex() const;

private:
    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> ctx_;
};

} // namespace ffmpeg::format
