#include <unistd.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else

#endif

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavdevice/avdevice.h"
}
#define DATA_SIZE 1452
#define min(a,b) a>b?b:a;




void h264_send_data(char *data, int len);


int sockfd;
int run_flag = 1;
struct sockaddr_in send_addr,recv_addr;


int create_sock();
int init_ffmpeg();

int main(int argc, char *argv[])
{
    int ret;
    ret = create_sock();
    ret = init_ffmpeg();
    return 0;
}


int create_sock()
{
#ifdef _WIN32
    WSADATA wsData = {0};
    if(0 != WSAStartup(0x202, &wsData))
    {
        WSACleanup();
        return -1;
    }
    int socklen;
#else
    sock_len socklen;
#endif


    unsigned int opt = 0;
    struct ip_mreq mreq;
    socklen = sizeof (struct sockaddr_in);
    /* 创建 socket 用于UDP通讯 */
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf ("socket creating err in udptalk\n");
        exit (1);
    }
    /* 设置要加入组播的地址 */
    memset(&mreq, 0, sizeof (struct ip_mreq));
    mreq.imr_multiaddr.s_addr = inet_addr("225.3.3.11");
    /* 设置发送组播消息的源主机的地址信息 */
    mreq.imr_interface.s_addr = htonl (INADDR_ANY);

    /* 把本机加入组播地址，即本机网卡作为组播成员，只有加入组才能收到组播消息 */
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP , (char *)&mreq,sizeof (struct ip_mreq)) == -1)
    {
        perror ("setsockopt");
        exit (-1);
    }
    memset (&send_addr, 0, socklen);
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons (9999);
    send_addr.sin_addr.s_addr = inet_addr("225.3.3.11");

    opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR , (char *)&opt, sizeof(opt)) == -1)
    {

        perror("setsockopt");
        exit (-1);
    }

    opt = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&opt,sizeof (opt)) == -1)
    {
        printf("IP_MULTICAST_LOOP set fail!\n");
    }

    int buf_size= 32*1024;//设置为32K
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size,sizeof (opt)) == -1)
    {
        printf("IP_MULTICAST_LOOP set fail!\n");
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size,sizeof (opt)) == -1)
    {
        printf("IP_MULTICAST_LOOP set fail!\n");
    }


    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    recv_addr.sin_port = htons(7838);


    /* 绑定自己的端口和IP信息到socket上 */
    if (bind(sockfd, (struct sockaddr *) &recv_addr,sizeof (struct sockaddr_in)) == -1)
    {
        perror("Bind error");
        exit (0);
    }
}


int init_ffmpeg()
{
    AVFormatContext	*pFormatCtx;
    int				i, videoindex;
    AVCodecContext	*pCodecCtx;
    AVCodec			*pCodec;
    AVCodecContext	*pH264CodecCtx;
    AVCodec			*pH264Codec;
    av_register_all();
    avformat_network_init();
    avdevice_register_all();//Register Device
    pFormatCtx = avformat_alloc_context();

    AVDictionary* options = NULL;

    av_dict_set(&options,"framerate","25",0);
    //抓取屏幕
    AVInputFormat *ifmt=av_find_input_format("gdigrab");
    if(avformat_open_input(&pFormatCtx,"desktop",ifmt, &options)!=0){
        printf("Couldn't open input stream. ");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx,NULL)<0)
    {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
    {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoindex=i;
        }
    }
    if(videoindex==-1)
    {
        printf("Couldn't find a video stream.\n");
        return -1;
    }
    //根据视频中的流打开选择解码器
    pCodecCtx=pFormatCtx->streams[videoindex]->codec;
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL)
    {
        printf("Codec not found.\n");
        return -1;
    }
    //打开解码器
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
    {
        printf("Could not open codec.\n");
        return -1;
    }
    AVFrame	*pFrame,*pFrameYUV;
    pFrame=av_frame_alloc();
    pFrame->width = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;
    pFrame->format = AV_PIX_FMT_YUV420P;

    pFrameYUV=av_frame_alloc();
    pFrameYUV->width = pCodecCtx->width;
    pFrameYUV->height = pCodecCtx->height;
    pFrameYUV->format = AV_PIX_FMT_YUV420P;

    uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    int ret, got_picture;
    AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    AVPacket *packetH264=(AVPacket *)av_malloc(sizeof(AVPacket));
    FILE *fp_h264=fopen("c://h264//intput.h264","wb");
    struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    ///这里打印出视频的宽高
    fprintf(stderr,"w= %d h= %d\n",pCodecCtx->width, pCodecCtx->height);
    ///我们就读取100张图像
    //查找h264编码器
    pH264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!pH264Codec)
    {
        fprintf(stderr, "---------h264 codec not found----\n");
        exit(1);
    }

    int bps = 80;//*((int *)(quality));

    if(bps < 60)
    {
        bps = 1000000;
    }
    else if(bps > 80)
    {
        bps = 4000000;
    }
    else
    {
        bps = 2000000;
    }

    pH264CodecCtx = avcodec_alloc_context3(pH264Codec);
    pH264CodecCtx->codec_id = AV_CODEC_ID_H264;
    pH264CodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pH264CodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pH264CodecCtx->width = pCodecCtx->width;
    pH264CodecCtx->height = pCodecCtx->height;
    pH264CodecCtx->time_base.num = 1;
    pH264CodecCtx->time_base.den = 25;//帧率(既一秒钟多少张图片)
    pH264CodecCtx->bit_rate = bps; //比特率(调节这个大小可以改变编码后视频的质量)
    pH264CodecCtx->gop_size= 12;
    //H264 还可以设置很多参数 自行研究吧
    pH264CodecCtx->qmin = 10;
    pH264CodecCtx->qmax = 51;
    pH264CodecCtx->max_b_frames = 0;
    // some formats want stream headers to be separate
    if (pH264CodecCtx->flags & AVFMT_GLOBALHEADER)
        pH264CodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    // Set Option
    AVDictionary *param = 0;
    //H.264
    av_dict_set(&param, "preset", "superfast", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);  //实现实时编码
    if (avcodec_open2(pH264CodecCtx, pH264Codec,&param) < 0){
        printf("Failed to open video encoder1! 编码器打开失败！\n");
        return false;
    }
    for(;;)
    {
        //读取截屏中的数据--->packet
        if(av_read_frame(pFormatCtx, packet) < 0)
        {
            break;
        }
        if(packet->stream_index==videoindex)
        {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0){
                printf("Decode Error.\n");
                return -1;
            }
            if(got_picture)
            {
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

                //编码成h264文件
                ret = avcodec_encode_video2(pH264CodecCtx, packet,pFrameYUV, &got_picture);
                if(ret < 0){
                    printf("Failed to encode! \n");
                    return -1;
                }
                h264_send_data((char *)packet->data, packet->size);
            }
        }
        av_free_packet(packet);

        if(!run_flag)
        {
            break;
        }
    }
    sws_freeContext(img_convert_ctx);
    fclose(fp_h264);
    av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);


    return 0;
}

unsigned short count = 0;




void h264_send_data(char *data, int len)
{
    int num = (len + DATA_SIZE - 1)/ DATA_SIZE;
    int pending = len;
    int dataLen = 0;
    char *ptr = data;
    char buf[DATA_SIZE + 8] = {0};
    buf[0] = 0xff;
    buf[1] = 0xff;
    //buf[2] = 0xff;
    //buf[3] = 0xff;
    *(unsigned short *)(&buf[2]) = count++;
    *(unsigned int *)(&buf[4]) = len;
    int first = 1;
    while(pending > 0)
    {
        dataLen = min(pending, DATA_SIZE);

        if(first)  //+8个字节头 1 flag 4 datalen
        {
            memcpy(buf + 8, ptr, dataLen);
            ptr += dataLen;
            sendto(sockfd, buf,  dataLen + 8, 0, (struct  sockaddr *)&send_addr, sizeof (send_addr));
            first = 0;
        }
        else
        {
            memcpy(buf, ptr, dataLen);
            ptr += dataLen;
            sendto(sockfd, buf,  dataLen, 0, (struct sockaddr *)&send_addr, sizeof (send_addr));
        }
        pending -= dataLen;
        //usleep(50);
    }
}
