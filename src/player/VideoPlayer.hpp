#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <iostream>
#include <glad/glad.h>
#include "VideoDecoder.hpp"

class VideoPlayer {
private:
  AVFormatContext* formatContext;
  SwsContext* scalerContext;

  unsigned int width;
  unsigned int height;

public:
  VideoPlayer() : formatContext (0) {}
  ~VideoPlayer();

  bool init();
  bool load(const char* filename);

  const char* getFormatLongName() const;
  int64_t getDuration() const;
  int64_t getBitRate() const;

  void play();
  void decodeVideoStream(const AVCodec* codec, const AVCodecParameters* codecParams);
  void decodeAudioStream(const AVCodec* codec, const AVCodecParameters* codecParams);
  void handleFrameDecoded(const AVCodecContext* codecContext, const AVFrame* frame);

  VideoDecoder* createDecoderFirstVideoStream() const;

  void takeScreenshot(const unsigned char* bytes, int wrap, int width, int height,  const char* filename) const;

};
