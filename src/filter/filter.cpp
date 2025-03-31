module;

#include <expected>
#include <format>
#include <memory>
#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h> // For AVCodecParameters
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h> // For pixel format names
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

VideoFilter::VideoFilter(const AVCodecParameters *input_codecpar,
                         AVRational input_time_base, int output_framerate,
                         AVPixelFormat output_pix_fmt)
    : target_width_(input_codecpar->width),
      target_height_(input_codecpar->height), target_pix_fmt_(output_pix_fmt) {
    if (input_codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
        throw std::runtime_error("VideoFilter can only process video streams.");
    }
    if (input_codecpar->width <= 0 || input_codecpar->height <= 0 ||
        input_codecpar->format == AV_PIX_FMT_NONE) {
        throw std::runtime_error(
            "Input codec parameters lack valid dimensions or pixel format.");
    }

    // 1. Initialize Filter Graph for Frame Rate Conversion
    initializeFilterGraph(input_codecpar, input_time_base, output_framerate);

    // 2. Initialize Scaler for Color Space Conversion (if necessary)
    // Note: Scaler might be re-initialized later if dimensions change,
    // but we initialize it here based on input params.
    if (static_cast<AVPixelFormat>(input_codecpar->format) != output_pix_fmt) {
        initializeScaler(input_codecpar->width, input_codecpar->height,
                         static_cast<AVPixelFormat>(input_codecpar->format),
                         target_width_, target_height_, target_pix_fmt_);
    }
}

void VideoFilter::initializeFilterGraph(const AVCodecParameters *input_codecpar,
                                        AVRational input_time_base,
                                        int output_framerate) {
    int ret = 0;
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

    auto args = std::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
        input_codecpar->width, input_codecpar->height, input_codecpar->format,
        input_time_base.num, input_time_base.den,
        input_codecpar->sample_aspect_ratio.num,
        input_codecpar->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx_, buffersrc, "in",
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

    auto filter_spec = std::format("fps={}", output_framerate);

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

void VideoFilter::initializeScaler(int srcW, int srcH, AVPixelFormat srcFormat,
                                   int dstW, int dstH,
                                   AVPixelFormat dstFormat) {
    // Only initialize if needed
    if (srcW == dstW && srcH == dstH && srcFormat == dstFormat) {
        sws_ctx_.reset(); // No scaling needed
        return;
    }

    SwsContext *scaler =
        sws_getContext(srcW, srcH, srcFormat, dstW, dstH, dstFormat,
                       SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (scaler == nullptr) {
        throw std::runtime_error(std::format(
            "Failed to create SwsContext for format conversion from {} to {}",
            av_get_pix_fmt_name(srcFormat), av_get_pix_fmt_name(dstFormat)));
    }
    sws_ctx_.reset(scaler);
    target_width_ = dstW; // Store actual target dimensions
    target_height_ = dstH;
}

auto VideoFilter::filterFrame(util::Frame &input_frame)
    -> util::ffmpeg_result<util::Frame> {
    if (eof_received_from_graph_) {
        return std::unexpected(util::FFmpegEOF{});
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
            return std::unexpected(util::get_ffmpeg_error(ret));
        }
    }

    // --- Step 2: Receive filtered frame from the graph ---
    while (true) {
        util::Frame filtered_frame;
        ret = av_buffersink_get_frame(buffersink_ctx_, filtered_frame.get());

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

        // Check if scaler is needed (pixel format differs or scaler is already
        // initialized)
        if (sws_ctx_ || static_cast<AVPixelFormat>(
                            filtered_frame.get()->format) != target_pix_fmt_) {
            // Re-initialize scaler if dimensions or format changed unexpectedly
            if (!sws_ctx_ || filtered_frame.get()->width != target_width_ ||
                filtered_frame.get()->height != target_height_ ||
                static_cast<AVPixelFormat>(filtered_frame.get()->format) !=
                    target_pix_fmt_) {
                initializeScaler(
                    filtered_frame.get()->width, filtered_frame.get()->height,
                    static_cast<AVPixelFormat>(filtered_frame.get()->format),
                    filtered_frame.get()->width, filtered_frame.get()->height,
                    target_pix_fmt_); // Keep dimensions
            }

            // If scaler initialization failed or wasn't needed, sws_ctx_ is
            // null
            if (sws_ctx_) {
                util::Frame scaled_frame;
                // Prepare the destination frame
                scaled_frame.get()->format = target_pix_fmt_;
                scaled_frame.get()->width = target_width_;
                scaled_frame.get()->height = target_height_;
                ret = av_frame_get_buffer(scaled_frame.get(),
                                          0); // Allocate buffer
                if (ret < 0) {
                    return std::unexpected(util::get_ffmpeg_error(ret));
                }

                // Perform scaling/conversion
                ret = sws_scale(
                    sws_ctx_.get(),
                    static_cast<const uint8_t *const *>(
                        filtered_frame.get()->data),
                    static_cast<int *>(filtered_frame.get()->linesize), 0,
                    filtered_frame.get()->height,
                    static_cast<uint8_t **>(scaled_frame.get()->data),
                    static_cast<int *>(scaled_frame.get()->linesize));

                if (ret < 0) {
                    return std::unexpected(util::get_ffmpeg_error(ret));
                }
                // Copy timestamp and other relevant fields
                av_frame_copy_props(scaled_frame.get(), filtered_frame.get());

                // Return the scaled frame
                return scaled_frame; // Move the scaled frame out
            }
        }
        // If no scaling was performed or needed, return the frame from the
        // filter graph directly
        return filtered_frame; // Move the filtered frame out
    } // end while loop for receiving frames
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
