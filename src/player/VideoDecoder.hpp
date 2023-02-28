#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}

#include <iostream>

#include <chrono>
using namespace std::chrono;

class VideoDecoder {
private:
    AVFormatContext* formatContext;
    AVStream* stream;
    int streamIndex;
    const AVCodecParameters* codecParameters;

    const AVCodec* codec;
    AVCodecContext *codecContext = nullptr;

    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;

public:
    VideoDecoder(AVFormatContext* formatCtx);
    ~VideoDecoder();

    void init();

    double nextFrame(uint32_t textureIdY, uint32_t textureIdU, uint32_t textureIdV);
    void handleFrameDecoded(const AVFrame* frame, uint32_t textureIdY, uint32_t textureIdU, uint32_t textureIdV) const;

    AVPixelFormat getPixelFormat() const;
    unsigned int getWidth() const;
    unsigned int getHeight() const;
};