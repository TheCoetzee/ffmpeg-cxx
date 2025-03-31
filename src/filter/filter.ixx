module;

#include <memory>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/rational.h>
#include <libavutil/pixfmt.h>
#include <libavcodec/codec_par.h>
struct AVFilterGraph;
struct AVFilterContext;
struct SwsContext;
}

export module ffmpeg.filter;

import ffmpeg.util;

export namespace ffmpeg::filter {

// RAII Deleters for filter graph and swscale contexts
struct AVFilterGraphDeleter {
    void operator()(AVFilterGraph *graph) const;
};
struct SwsContextDeleter {
    void operator()(SwsContext *ctx) const;
};

class VideoFilter {
public:
    VideoFilter(AVCodecParameters *input_codecpar, AVRational input_time_base,
                int output_framerate, AVPixelFormat output_pix_fmt = AV_PIX_FMT_RGB32);

    auto filterFrame(util::Frame &input_frame) -> util::ffmpeg_result<util::Frame>;

    auto sendEOF() -> util::ffmpeg_result<void>;


private:
    void initializeScaler(int srcW, int srcH, AVPixelFormat srcFormat,
                          int dstW, int dstH, AVPixelFormat dstFormat);
    void initializeFilterGraph(AVCodecParameters *input_codecpar, AVRational input_time_base, int output_framerate);

    std::unique_ptr<AVFilterGraph, AVFilterGraphDeleter> filter_graph_;
    AVFilterContext *buffersrc_ctx_ = nullptr; // Managed by the graph
    AVFilterContext *buffersink_ctx_ = nullptr; // Managed by the graph

    std::unique_ptr<SwsContext, SwsContextDeleter> sws_ctx_;

    int target_width_;
    int target_height_;
    AVPixelFormat target_pix_fmt_;

    bool eof_sent_to_graph_ = false;
    bool eof_received_from_graph_ = false;

};

} // namespace ffmpeg::filter
