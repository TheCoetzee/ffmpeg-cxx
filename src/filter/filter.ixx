module;

#include <memory>
#include <optional>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec_par.h>
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
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
    VideoFilter(util::Frame const &input_frame, AVRational input_time_base,
                int output_framerate, int output_width, int output_height,
                AVPixelFormat output_pix_fmt = AV_PIX_FMT_RGB32);

    auto sendFrame(util::Frame &input_frame) -> std::optional<util::FFmpegErrors>;
    auto filterFrame() -> util::ffmpeg_result<util::Frame>;

    auto sendEOF() -> util::ffmpeg_result<void>;

private:
    void initializeFilterGraph(util::Frame const &input_frame,
                               AVRational input_time_base, int output_framerate,
                               int output_width, int output_height);

    std::unique_ptr<AVFilterGraph, AVFilterGraphDeleter> filter_graph_;
    AVFilterContext *buffersrc_ctx_ = nullptr;  // Managed by the graph
    AVFilterContext *buffersink_ctx_ = nullptr; // Managed by the graph

    int target_width_;
    int target_height_;
    AVPixelFormat target_pix_fmt_;

    bool eof_sent_to_graph_ = false;
    bool eof_received_from_graph_ = false;
};

} // namespace ffmpeg::filter
