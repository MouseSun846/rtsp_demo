extern "C"
{
    #include "libavutil/adler32.h"
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/timestamp.h"
    #include "libswscale/swscale.h"
}

int openInput();