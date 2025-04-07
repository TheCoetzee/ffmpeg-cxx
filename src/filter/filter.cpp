module;

#include <expected>
#include <format>
#include <memory>
#include <print>
#include <stdexcept>
#include <optional>

extern "C" {
#include <libavcodec/avcodec.h> // For AVCodecParameters
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h> // For pixel format names
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

module ffmpeg.filter;
import ffmpeg.util;

namespace ffmpeg::filter {

void AVFilterGraphDeleter::operator()(AVFilterGraph *graph) const {
    if (graph != nullptr) {
        avfilter_graph_free(&graph);
    }
}

void SwsContextDeleter::operator()(SwsContext *ctx) const {
    if (ctx != nullptr) {
        sws_freeContext(ctx);
    }
}

VideoFilter::VideoFilter(util::Frame const &frame, AVRational input_time_base,
                         int output_framerate, int output_width,
                         int output_height, AVPixelFormat output_pix_fmt)
    : target_width_(frame.width()), target_height_(frame.height()),
      target_pix_fmt_(output_pix_fmt) {
    if (frame.width() <= 0 || frame.height() <= 0 ||
        frame.format() == AV_PIX_FMT_NONE) {
        throw std::runtime_error(
            "Input codec parameters lack valid dimensions or pixel format.");
    }

    initializeFilterGraph(frame, input_time_base, output_framerate,
                          output_width, output_height);
}

void VideoFilter::initializeFilterGraph(util::Frame const &input_frame,
                                        AVRational input_time_base,
                                        int output_framerate, int output_width,
                                        int output_height) {
    auto width = input_frame.width();
    auto height = input_frame.height();
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    auto *outputs = avfilter_inout_alloc();
    auto *inputs = avfilter_inout_alloc();
    std::unique_ptr<AVFilterGraph, AVFilterGraphDeleter> graph(
        avfilter_graph_alloc());

    // Check allocations
    if ((outputs == nullptr) || (inputs == nullptr) || !graph) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        throw util::get_ffmpeg_error(AVERROR(ENOMEM));
    }

    filter_graph_ = std::move(graph); // Transfer ownership
    //
    auto sar = input_frame.sample_aspect_ratio();

    auto args =
        std::format("video_size={}x{}:pix_fmt={}:time_base={}/"
                    "{}:pixel_aspect={}/{}",
                    width, height, static_cast<int>(input_frame.format()),
                    input_time_base.num, input_time_base.den, sar.num, sar.den);

    auto ret = avfilter_graph_create_filter(&buffersrc_ctx_, buffersrc, "in",
                                            args.c_str(), nullptr,
                                            filter_graph_.get());
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        throw util::get_ffmpeg_error(ret);
    }

    // Buffer sink setup
    ret = avfilter_graph_create_filter(&buffersink_ctx_, buffersink, "out",
                                       nullptr, nullptr, filter_graph_.get());
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        throw util::get_ffmpeg_error(ret);
    }

    // Set output pixel format
    ret = av_opt_set_bin(buffersink_ctx_, "pix_fmts",
                         reinterpret_cast<uint8_t *>(&target_pix_fmt_),
                         sizeof(target_pix_fmt_), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        throw util::get_ffmpeg_error(ret);
    }

    // Configure inputs/outputs for graph parsing
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx_;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx_;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    auto filter_spec =
        std::format("fps={},scale=width={}:height={}", output_framerate, width,
                    height, output_width, output_height);

    ret = avfilter_graph_parse_ptr(filter_graph_.get(), filter_spec.c_str(),
                                   &inputs, &outputs, nullptr);
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        throw util::get_ffmpeg_error(ret);
    }

    // Configure graph
    ret = avfilter_graph_config(filter_graph_.get(), nullptr);
    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        throw util::get_ffmpeg_error(ret);
    }
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
}

auto VideoFilter::sendFrame(util::Frame &input_frame) -> std::optional<util::FFmpegErrors> {
    if (eof_received_from_graph_) {
        return util::FFmpegEOF{};
    }

    // --- Step 1: Send frame to the filter graph ---
    int ret = av_buffersrc_add_frame_flags(buffersrc_ctx_, input_frame.get(),
                                           AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        // If EOF was already signaled, EAGAIN is expected until graph is
        // flushed.
        if (eof_sent_to_graph_ && ret == AVERROR(EAGAIN)) {
            // Continue to try receiving frames below
        } else {
            // A real error occurred sending the frame
            return util::get_ffmpeg_error(ret);
        }
    }
    return {};
}

auto VideoFilter::filterFrame() -> util::ffmpeg_result<util::Frame> {
    util::Frame filtered_frame;
    auto ret = av_buffersink_get_frame(buffersink_ctx_, filtered_frame.get());

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        // Need more input frames or graph is flushed
        if (ret == AVERROR_EOF) {
            eof_received_from_graph_ = true;
        }
        // If we successfully sent a frame earlier, return EAGAIN to signal
        // that the *input* frame was consumed, but no *output* is ready
        // yet. If we couldn't send a frame (due to graph EOF/EAGAIN), and
        // also can't receive, signal EOF if the graph is done, otherwise
        // EAGAIN.
        if (eof_received_from_graph_) {
            return std::unexpected(util::FFmpegEOF{});
        }
        return std::unexpected(util::FFmpegAGAIN{});
    }
    if (ret < 0) {
        // A real error receiving frame
        return std::unexpected(util::get_ffmpeg_error(ret));
    }

    return filtered_frame; // Move the filtered frame out
}

auto VideoFilter::sendEOF() -> util::ffmpeg_result<void> {
    if (!eof_sent_to_graph_) {
        eof_sent_to_graph_ = true;
        // Signal EOF to the filter graph source buffer
        int ret = av_buffersrc_add_frame(buffersrc_ctx_, nullptr);
        if (ret < 0 && ret != AVERROR_EOF) { // EOF is expected, ignore it
            return std::unexpected(util::get_ffmpeg_error(ret));
        }
    }
    // After sending EOF, continue trying to receive frames until the sink
    // signals EOF. Return AGAIN to prompt the caller to keep calling
    // filterFrame.
    return std::unexpected(util::FFmpegAGAIN{});
}

} // namespace ffmpeg::filter
