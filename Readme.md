# 推流地址
.\ffmpeg.exe -f dshow -i video="USB2.0 HD UVC WebCam" -c:v libx264 -preset medium -b:v 500k -c:a copy -rtsp_transport tcp -r 25 -video_size 1280x720 -f rtsp rtsp://localhost:8554/cam1