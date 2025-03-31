module;

#include <utility>

extern "C" {
#include "libavcodec/packet.h"
}

export module ffmpeg.util:packet;
import ffmpeg.util.error;

export namespace ffmpeg::util {

struct Packet {
    Packet() : packet_(av_packet_alloc()) {}
    ~Packet() { av_packet_free(&packet_); }

    Packet(const Packet &other) : packet_(av_packet_clone(other.packet_)) {}
    auto operator=(const Packet &other) -> Packet & {
        if (&other == this) {
            return *this;
        }
        av_packet_unref(packet_);
        auto ret = av_packet_ref(packet_, other.packet_);
        if (ret < 0) {
            throw get_ffmpeg_error(ret);
        }
        return *this;
    }

    Packet(Packet &&other) noexcept
        : packet_(std::exchange(other.packet_, nullptr)) {}
    auto operator=(Packet &&other) noexcept -> Packet & {
        av_packet_unref(packet_);
        av_packet_move_ref(packet_, other.packet_);
        return *this;
    }

    auto get() -> AVPacket * { return packet_; }

private:
    AVPacket *packet_;
};
}; // namespace ffmpeg::util
