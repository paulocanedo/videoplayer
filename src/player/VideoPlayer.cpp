#include "VideoPlayer.hpp"

VideoPlayer::~VideoPlayer() {
    if(formatContext) {
        avformat_free_context(formatContext);
    }

    if(scalerContext) {
        sws_freeContext(scalerContext);
    }

}

bool VideoPlayer::load(const char* filename) {
    const int result = avformat_open_input(&formatContext, filename, NULL, NULL);
    if(result != 0) {
        std::cerr << "Falha ao abrir arquivo: " << AVERROR(result) << std::endl;
        return false;
    }

    if (avformat_find_stream_info(formatContext, NULL)) {
        std::cerr << "Falha ao obter informacoes da stream\n";
    }

    return true;
}

void VideoPlayer::play() {
    const ushort nstreams = formatContext->nb_streams;
    for(uint i=0; i < nstreams; i++) {
        std::cout << i << ": ";

        const AVStream* stream = formatContext->streams[i];
        const AVCodecParameters* codecParams = stream->codecpar;

        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);

        printf("\n------------ %s ------------\n", codec->long_name);

        if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO) {
            this->decodeVideoStream(codec, codecParams);
        } else if (codecParams->codec_type == AVMEDIA_TYPE_AUDIO) {
            this->decodeAudioStream(codec, codecParams);
        }
        std::cout << std::endl;
    }
}

void VideoPlayer::decodeVideoStream(const AVCodec* codec, const AVCodecParameters* codecParams) {
    printf("%s (%d x %d)", codec->long_name, codecParams->width, codecParams->height);

    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecContext, codecParams);

    if(avcodec_open2(codecContext, codec, NULL) < 0) {
        std::cerr << "falha ao abrir codec\n";
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    uint nframe = 0;

    while(av_read_frame(formatContext, packet) >= 0) {
        if(avcodec_send_packet(codecContext, packet) == 0) {
            int result = avcodec_receive_frame(codecContext, frame);
            if(result == 0)
            {
                this->handleFrameDecoded(codecContext, frame);

                av_packet_unref(packet);

                if(++nframe >= 5) break;
            }
        }
    }

    av_frame_free(&frame);
    av_packet_free(&packet);

    avcodec_free_context(&codecContext);
}

void VideoPlayer::decodeAudioStream(const AVCodec *codec, const AVCodecParameters *codecParams) {
    printf("%s (%dch x %d sample rate)",
        codec->long_name,
        codecParams->ch_layout.nb_channels,
        codecParams->sample_rate
    );
}

void VideoPlayer::handleFrameDecoded(const AVCodecContext* codecContext, const AVFrame* frame) {
    printf("%c (%d) pts %ld dts %ld keyframe %d [coded_picture_number %d, display_pic_num %d]\n",
        av_get_picture_type_char(frame->pict_type),
        codecContext->frame_number,
        frame->pts,
        frame->pkt_dts,
        frame->key_frame,
        frame->coded_picture_number,
        frame->display_picture_number
    );

    char frame_filename[1024];
    snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "/home/paulocanedo/frame", codecContext->frame_number);

    const int width = frame->width;
    const int height = frame->height;

    if(!scalerContext) {
        scalerContext = sws_getContext(width, height, codecContext->pix_fmt, width, height, AV_PIX_FMT_RGB0, SWS_BILINEAR, NULL, NULL, NULL);
        if(!scalerContext) {
            std::cerr << "Falha ao criar scaler context.\n";
            return;
        }
    }

    uint8_t *data_rgb = (uint8_t*) (malloc(sizeof(uint8_t) * width * height * 4));
    uint8_t* dest[4] = { data_rgb , NULL, NULL, NULL };
    int dest_linesize[4] = { width * 4, 0, 0, 0 };
    sws_scale(scalerContext, frame->data, frame->linesize, 0, height, dest, dest_linesize);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_rgb);
    glGenerateMipmap(GL_TEXTURE_2D);

    free(data_rgb);
}

bool VideoPlayer::init() {
    if(!formatContext) {
        formatContext = avformat_alloc_context();

        if(!formatContext) {
            std::cerr << "Problema ao carregar avformat!\n";
            return false;
        }
    }

    return true;
}

const char* VideoPlayer::getFormatLongName() const {
    return formatContext->iformat->long_name;
}

int64_t VideoPlayer::getDuration() const {
    return formatContext->duration;
}

int64_t VideoPlayer::getBitRate() const {
    return formatContext->bit_rate;
}

void VideoPlayer::takeScreenshot(const unsigned char *bytes, int wrap, int width, int height, const char *filename) const {
    FILE *f = fopen(filename,"w");

    fprintf(f, "P5\n%d %d\n%d\n", width, height, 255);

    printf("** %d **", wrap);
    for(int i = 0; i < height; i++) {
        fwrite(bytes + i * wrap, 1, width, f);
    }

    fclose(f);
}

AVFormatContext* VideoPlayer::getFormatContext() const {
    return formatContext;
}
