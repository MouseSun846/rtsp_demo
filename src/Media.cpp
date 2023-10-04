#include "Media.h"
#include <iostream>
#include <iomanip>
#include "libavutil/log.h"

static int video_decode_example(const char *input_filename)
{
    const AVCodec *codec = NULL;
    AVCodecContext *ctx= NULL;
    AVCodecParameters *origin_par = NULL;
    AVFrame *fr = NULL;
    uint8_t *byte_buffer = NULL;
    AVPacket *pkt;
    AVFormatContext *fmt_ctx = NULL;
    int number_of_written_bytes;
    int video_stream;
    int byte_buffer_size;
    int i = 0;
    int result;

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);

    result = avformat_open_input(&fmt_ctx, input_filename, NULL, &opts);
    if (result < 0) {
        std::cerr<<"Can't open file"<<std::endl;
        return result;
    }

    result = avformat_find_stream_info(fmt_ctx, &opts);
    if (result < 0) {
        std::cerr<<"Can't get stream info"<<std::endl;
        return result;
    }

    video_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream < 0) {
      std::cerr<<"Can't find video stream in input file"<<std::endl;
      return -1;
    }

    origin_par = fmt_ctx->streams[video_stream]->codecpar;

    codec = avcodec_find_decoder(origin_par->codec_id);
    if (!codec) {
        std::cerr<<"Can't find decoder"<<std::endl;
        return -1;
    }

    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        std::cerr<<"Can't allocate decoder context"<<std::endl;
        return AVERROR(ENOMEM);
    }

    result = avcodec_parameters_to_context(ctx, origin_par);
    if (result) {
        std::cerr<<"Can't copy decoder context"<<std::endl;
        return result;
    }

    result = avcodec_open2(ctx, codec, NULL);
    if (result < 0) {
        std::cerr<< "Can't open decoder"<<std::endl;
        return result;
    }

    fr = av_frame_alloc();
    if (!fr) {
        std::cerr<<"Can't allocate frame"<<std::endl;
        return AVERROR(ENOMEM);
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        std::cerr<<"Cannot allocate packet"<<std::endl;
        return AVERROR(ENOMEM);
    }

    byte_buffer_size = av_image_get_buffer_size(ctx->pix_fmt, ctx->width, ctx->height, 16);
    byte_buffer = (uint8_t *)av_malloc(byte_buffer_size);
    if (!byte_buffer) {
        std::cerr<<"Can't allocate buffer"<<std::endl;
        return AVERROR(ENOMEM);
    }

    std::cout << "#tb " << video_stream << ": " << fmt_ctx->streams[video_stream]->time_base.num << "/" << fmt_ctx->streams[video_stream]->time_base.den << std::endl;
    i = 0;

    result = 0;
    while (result >= 0) {
        result = av_read_frame(fmt_ctx, pkt);
        if (result >= 0 && pkt->stream_index != video_stream) {
            av_packet_unref(pkt);
            continue;
        }

        if (result < 0)
            result = avcodec_send_packet(ctx, NULL);
        else {
            if (pkt->pts == AV_NOPTS_VALUE)
                pkt->pts = pkt->dts = i;
            result = avcodec_send_packet(ctx, pkt);
        }
        av_packet_unref(pkt);

        if (result < 0) {
            std::cerr<<"Error submitting a packet for decoding"<<std::endl;
            return result;
        }

        while (result >= 0) {
            result = avcodec_receive_frame(ctx, fr);
            if (result == AVERROR_EOF)
                goto finish;
            else if (result == AVERROR(EAGAIN)) {
                result = 0;
                break;
            } else if (result < 0) {
                std::cerr<<"Error decoding frame"<<std::endl;
                return result;
            }

            number_of_written_bytes = av_image_copy_to_buffer(byte_buffer, byte_buffer_size,
                                    (const uint8_t* const *)fr->data, (const int*) fr->linesize,
                                    ctx->pix_fmt, ctx->width, ctx->height, 1);
            if (number_of_written_bytes < 0) {
                std::cerr<<"Can't copy image to buffer"<<std::endl;
                av_frame_unref(fr);
                return number_of_written_bytes;
            }
            std::cout <<"video_stream: "<< video_stream << ", " << fr->pts << ", " << fr->pkt_dts << ", " 
            << std::setw(8) << fr->duration << ", " << std::setw(8) << number_of_written_bytes << ", 0x" << std::setw(8) << std::setfill('0') << std::hex 
            << av_adler32_update(0, (const uint8_t*)byte_buffer, number_of_written_bytes) << std::dec << std::endl;
            av_frame_unref(fr);
        }
        i++;
    }

finish:
    av_packet_free(&pkt);
    av_frame_free(&fr);
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&ctx);
    av_freep(&byte_buffer);
    return 0;
}

static void log_callback_help(void *ptr, int level, const char *fmt, va_list vl)
{
    vfprintf(stdout, fmt, vl);
}

int openInput() {
    // av_log_set_level(AV_LOG_DEBUG);
    // av_log_set_callback(log_callback_help);  
    // if (video_decode_example("rtsp://admin:IVNEKL@192.168.1.2:554/h264/ch1/main/av_stream") != 0)
    //     return 1;      
    if (video_decode_example("rtsp://192.168.1.5:8554/cam1") != 0)
        return 1;

    return 0;
}