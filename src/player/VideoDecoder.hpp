#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <iostream>

class VideoDecoder {
private:
    AVFormatContext* formatContext;
    const AVCodecParameters* codecParameters;

    const AVCodec* codec;
    AVCodecContext *codecContext = nullptr;

    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;

    uint currentFrame = 0;

public:
    VideoDecoder(AVFormatContext* formatCtx, const AVCodecParameters* codecParameters);
    ~VideoDecoder();

    void init();

    double nextFrame(uint8_t* buffer, SwsContext* scalerContext);
    void handleFrameDecoded(const AVFrame* frame, SwsContext* scalerContext, uint8_t* buffer) const;

    AVPixelFormat getPixelFormat() const;
    unsigned int getWidth() const;
    unsigned int getHeight() const;
};