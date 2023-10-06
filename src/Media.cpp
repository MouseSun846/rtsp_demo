#include "Media.h"
#include <iostream>
#include <iomanip>
#include "libavutil/log.h"
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/imgcodecs.hpp>
#include <thread>
#include <chrono>

int ConvertYuvToBgr(int picCount, AVFrame *frame) {
    // 设置源图像参数
    int src_width = frame->width;  // 图像宽度
    int src_height = frame->height;  // 图像高度
    AVPixelFormat src_pix_fmt = (AVPixelFormat)frame->format;  // 像素格式
    uint8_t * const src_data[4] = { frame->data[0], frame->data[1], frame->data[2], NULL };  // 图像数据

    // 设置目标图像参数
    int dst_width = src_width;
    int dst_height = src_height;
    AVPixelFormat dst_pix_fmt = AV_PIX_FMT_BGR24;
    uint8_t *dst_data[4] = { NULL };
    int dst_linesize[4] = { 0 };
    int dst_buffer_size = av_image_alloc(dst_data, dst_linesize, dst_width, dst_height, dst_pix_fmt, 1);
    if(dst_buffer_size < 0) {
        std::cerr<<"av_image_alloc failed!"<<std::endl;
    }

    // 创建 SwsContext 对象
    SwsContext *sws_ctx = sws_getContext(src_width, src_height, src_pix_fmt,
                                        dst_width, dst_height, dst_pix_fmt,
                                        SWS_BICUBIC, NULL, NULL, NULL);

    // 转换图像数据
    sws_scale(sws_ctx, src_data, frame->linesize, 0, src_height, dst_data, dst_linesize);

    // 处理转换后的图像数据（在 dst_data 中）
    cv::Mat bgrImage(dst_height, dst_width, CV_8UC3, dst_data[0], dst_linesize[0]);

    if(picCount%100 == 0) {
        std::string picName = "/mnt/d/Code/FFmpeg/example/out/";
        picName += std::to_string(picCount);
        picName += ".jpg";
        // 写图
        bool result = cv::imwrite(picName, bgrImage);
        if(!result) {
            std::cerr<<"cv::imwrite failed!"<<std::endl;
        }
    }


    // 释放资源
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);    
    return 0;
}


int video_decode_example(const char *input_filename)
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
    static int picCount = 0;
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
                goto finish;
            }

            number_of_written_bytes = av_image_copy_to_buffer(byte_buffer, byte_buffer_size,
                                    (const uint8_t* const *)fr->data, (const int*) fr->linesize,
                                    ctx->pix_fmt, ctx->width, ctx->height, 1);
            if (number_of_written_bytes < 0) {
                std::cerr<<"Can't copy image to buffer"<<std::endl;
                av_frame_unref(fr);
                goto finish;
            }

            ConvertYuvToBgr(picCount, fr);
            picCount++;
            std::cout <<"pix_fmt:"<<ctx->pix_fmt<<" video_stream: "<< video_stream << ", " << fr->pts << ", " << fr->pkt_dts << ", " 
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
    return -1;
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
    while (video_decode_example("rtsp://192.168.1.3:8554/cam1") != 0) {
        std::cout<<"runing..."<<std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
        return 1;

    return 0;
}