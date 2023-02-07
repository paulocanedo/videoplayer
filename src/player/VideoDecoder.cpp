#include "VideoDecoder.hpp"

VideoDecoder::VideoDecoder(AVFormatContext* formatContext, const AVCodecParameters* codecParameters) {
    if(codecParameters->codec_type != AVMEDIA_TYPE_VIDEO) {
        std::cerr << "parametro codec não é vídeo.\n";
    }

    this->formatContext = formatContext;
    this->codecParameters = codecParameters;
    this->codec = avcodec_find_decoder(codecParameters->codec_id);

    if(this->codec == NULL) {
        std::cerr << "Não encontrou um decoder apropriado.\n";
    }

    this->codecContext = nullptr;
    this->packet = nullptr;
    this->frame = nullptr;
}

VideoDecoder::~VideoDecoder() {
    if(frame)
        av_frame_free(&frame);

    if(packet)
        av_packet_free(&packet);

    if(codecContext)
        avcodec_free_context(&codecContext);
}

void VideoDecoder::init() {
    codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecContext, codecParameters);

    if(avcodec_open2(codecContext, codec, NULL) < 0) {
        std::cerr << "falha ao abrir codec\n";
    }

    packet = av_packet_alloc();
    frame = av_frame_alloc();
}

double VideoDecoder::nextFrame(uint8_t* buffer, SwsContext* scalerContext) {
    //if(currentFrame++ > 30) return false;
    int readedFrame = av_read_frame(formatContext, packet);
    if(readedFrame != 0) {
        std::cerr << "Falha ao obter um pacote do decoder.\n";
        return -1.0;
    }

    int sentPacket = avcodec_send_packet(codecContext, packet);
    if(sentPacket == AVERROR(EAGAIN) || sentPacket == AVERROR_EOF) {
        av_packet_unref(packet);

        return nextFrame(buffer, scalerContext);
    }

    int receivedFrame = avcodec_receive_frame(codecContext, frame);
    if(receivedFrame == AVERROR(EAGAIN) || receivedFrame == AVERROR_EOF) {
        av_packet_unref(packet);

        return nextFrame(buffer, scalerContext);
    } else if(receivedFrame == 0) {
        handleFrameDecoded(frame, scalerContext, buffer);
        av_packet_unref(packet);

        return frame->pts / 24.0;
    }

    return -1.0;
}

void VideoDecoder::handleFrameDecoded(const AVFrame* frame, SwsContext* scalerContext, uint8_t* buffer) const {
/*
    printf("%c (%d) pts %ld dts %ld keyframe %d [coded_picture_number %d, display_pic_num %d]\n",
        av_get_picture_type_char(frame->pict_type),
        codecContext->frame_number,
        frame->pts,
        frame->pkt_dts,
        frame->key_frame,
        frame->coded_picture_number,
        frame->display_picture_number
    );
*/

    //char frame_filename[1024];
    //snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "/home/paulocanedo/frame", codecContext->frame_number);

    int width = frame->width;
    int height = frame->height;

    //uint8_t *data_rgb = (uint8_t*) (malloc(sizeof(uint8_t) * width * height * 4));
    uint8_t* dest[4] = { buffer , NULL, NULL, NULL };
    int dest_linesize[4] = { width * 4, 0, 0, 0 };
    sws_scale(scalerContext, frame->data, frame->linesize, 0, height, dest, dest_linesize);

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_rgb);
    //glGenerateMipmap(GL_TEXTURE_2D);

    //free(data_rgb);
}

unsigned int VideoDecoder::getWidth() const {
    return codecContext->width;
}

unsigned int VideoDecoder::getHeight() const {
    return codecContext->height;
}

AVPixelFormat VideoDecoder::getPixelFormat() const {
    return codecContext->pix_fmt;
}
