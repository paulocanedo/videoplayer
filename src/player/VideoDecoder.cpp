#include "VideoDecoder.hpp"
#include <glad/glad.h>

VideoDecoder::VideoDecoder(AVFormatContext* formatContext) {
    this->formatContext = formatContext;

    this->streamIndex = -1;
    this->stream = nullptr;
    this->codec = nullptr;
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
    int index = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(index < 0) {
        std::cerr << "Nenhuma stream de video encontrada.\n";
    }

    this->streamIndex = index;
    this->stream = this->formatContext->streams[index];
    this->codec = avcodec_find_decoder(stream->codecpar->codec_id);
    std::cout << "CODEC: " << avcodec_get_name(stream->codecpar->codec_id) << std::endl;

    if(this->codec == NULL) {
        std::cerr << "Não encontrou um decoder apropriado.\n";
    }

    codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecContext, stream->codecpar);

//-----------------------------------------------------------------
    codecContext->thread_count = 0;

    if (codec->capabilities & AV_CODEC_CAP_FRAME_THREADS)
        codecContext->thread_type = FF_THREAD_FRAME;
    else if (codec->capabilities & AV_CODEC_CAP_SLICE_THREADS)
        codecContext->thread_type = FF_THREAD_SLICE;
    else
        codecContext->thread_count = 1; //don't use multithreading
//-----------------------------------------------------------------
    printf("pixel format: %s\n", av_get_pix_fmt_name(codecContext->pix_fmt));
    if(avcodec_open2(codecContext, codec, NULL) < 0) {
        std::cerr << "falha ao abrir codec\n";
    }

    packet = av_packet_alloc();
    frame = av_frame_alloc();
}

double VideoDecoder::nextFrame(uint32_t textureIdY, uint32_t textureIdU, uint32_t textureIdV) {
    auto start = high_resolution_clock::now();

    int readedFrame = av_read_frame(formatContext, packet);
    if(readedFrame != 0) {
        std::cerr << "Falha ao obter um pacote do container.\n";
        return -1.0;
    }

    if(packet->stream_index != this->streamIndex) {
        return nextFrame(textureIdY, textureIdU, textureIdV);
    }

    // ------------------------------

    int ret = avcodec_send_packet(codecContext, packet);
    if(ret < 0) {
        av_packet_unref(packet);
        std::cerr << "Falha ao enviar pacote para decode\n";
        exit(1);
    }

    int receivedFrame = 0;
    while (receivedFrame >=0) {
        receivedFrame = avcodec_receive_frame(codecContext, frame);

        if(receivedFrame == AVERROR(EAGAIN) || receivedFrame == AVERROR_EOF) {
            av_packet_unref(packet);

            return -1.0;
        } else if(receivedFrame < 0) {
            std::cerr << "Falha ao decodificar pacote\n";
            exit(1);
        }

        handleFrameDecoded(frame, textureIdY, textureIdU, textureIdV);
    }

    //auto stop = high_resolution_clock::now();
    //auto duration = duration_cast<milliseconds>(stop - start);
    //std::cout << "__stop_1__ " << av_get_picture_type_char(frame->pict_type) << ": " << duration << "; " << std::endl;
    //start = high_resolution_clock::now();

    //handleFrameDecoded(frame, scalerContext, buffer);
    av_packet_unref(packet);

    //stop = high_resolution_clock::now();
    //duration = duration_cast<milliseconds>(stop - start);
    //std::cout << "__stop_2__ " << av_get_picture_type_char(frame->pict_type) << ": " << duration << "; " << std::endl;
    //start = high_resolution_clock::now();

    return frame->pts * stream->time_base.num / (double)stream->time_base.den;
}

void VideoDecoder::handleFrameDecoded(const AVFrame* frame, uint32_t textureIdY, uint32_t textureIdU, uint32_t textureIdV) const {
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

    //float aspect = frame->width / (float) frame->height;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIdY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width, frame->height, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, textureIdU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width/2, frame->height/2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);
    glGenerateMipmap(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, textureIdV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width/2, frame->height/2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);
    glGenerateMipmap(GL_TEXTURE_2D);
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
