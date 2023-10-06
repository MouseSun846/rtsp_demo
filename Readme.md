# 推流地址
.\ffmpeg.exe -f dshow -i video="USB2.0 HD UVC WebCam" -c:v libx264 -preset medium -b:v 500k -c:a copy -rtsp_transport tcp -r 25 -video_size 1280x720 -f rtsp rtsp://localhost:8554/cam1

# Media功能说明
Media是用于使用FFmpeg库对视频进行解码的示例。首先通过avformat_open_input函数打开视频文件，使用avformat_find_stream_info函数获取音视频文件流信息，在这段代码中只处理了视频流，通过av_find_best_stream函数找到最合适的视频流。然后找到对应的AVCodec，并为其分配AVCodecContext以及AVFrame和AVPacket等资源。接下来进入while循环，用av_read_frame函数读取每一帧，判断是否是视频流并发送到解码器，解码后得到AVFrame，将AVFrame转换成BGR格式并处理该帧图像，并输出一些调试信息，最后释放资源。需要注意的是，在循环中要不断调用avcodec_receive_frame函数直到返回值小于0或者AVERROR(EAGAIN)为止，以确保解码器能够处理完所有数据。