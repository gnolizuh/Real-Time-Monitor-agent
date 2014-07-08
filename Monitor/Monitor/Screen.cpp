#include "stdafx.h"
#include "afxdialogex.h"
#include "Screen.h"

BEGIN_MESSAGE_MAP(Screen, CWnd)
END_MESSAGE_MAP()

Screen::Screen()
	: CWnd()
	, msg_queue()
	, screen_rect(0, 0, 0, 0)
	, wrapper(NULL)
	, index(0)
	, window(NULL)
	, render(NULL)
	, texture(NULL)
	, sdl_window_mutex(NULL)
{
}

Screen::~Screen()
{
}

void Screen::Prepare(const CRect &rect, const CWnd *wrapper, pj_uint32_t idx)
{
	pj_uint32_t width = PJ_ABS(rect.right - rect.left);
	pj_uint32_t height = PJ_ABS(rect.bottom - rect.top);

	this->screen_rect = rect;
	this->wrapper = wrapper;
	this->index = idx;

	pj_bool_t ret = this->Create(
		NULL,
		NULL,
		WS_BORDER | WS_VISIBLE | WS_CHILD,
		rect,
		(CWnd *)wrapper,
		idx);
	pj_assert(ret == PJ_TRUE);

	window = SDL_CreateWindowFrom(GetSafeHwnd());
	pj_assert(window != nullptr);

	render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	pj_assert(render != nullptr);

	texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
	pj_assert(texture != nullptr);

	sdl_window_mutex = SDL_CreateMutex();
	pj_assert(sdl_window_mutex != nullptr);

	msg_thread = thread( (std::bind(&Screen::WorkThread, this)) );
}

void Screen::Refresh(const CRect &rect)
{
	pj_uint32_t width = PJ_ABS(rect.right - rect.left);
	pj_uint32_t height = PJ_ABS(rect.bottom - rect.top);

	this->screen_rect = rect;
	int x, y;
	SDL_GetWindowPosition(window, &x, &y);
	SDL_SetWindowPosition(window, x + rect.left, y + rect.top);
	SDL_GetWindowPosition(window, &x, &y);
	SDL_RestoreWindow(window);
	SDL_ShowWindow(window);
	SDL_RaiseWindow(window);
	/*this->MoveWindow(rect);
	this->ShowWindow(SW_SHOW);*/
}

void Screen::Hide()
{
	SDL_HideWindow(window);
}

void Screen::Painting(const SDL_Rect &rect, const void *pixels, int pitch)
{
	SDL_LockMutex(sdl_window_mutex);

	SDL_UpdateTexture( texture, &rect, pixels, pitch );
	SDL_RenderClear( render );
	SDL_RenderCopy( render, texture, NULL, NULL );
	SDL_RenderPresent( render );

	SDL_UnlockMutex(sdl_window_mutex);
}

void Screen::WorkThread()
{
	/*while (1)
	{
		msg_queue.Wait();

		int *package = NULL;
		do
		{
			package = msg_queue.Pop();
			if ( package )
			{
			}
		} while ( package != NULL );
	}*/

	AVFormatContext *fmt_cont=NULL;
	if ( avformat_open_input(&fmt_cont, "test.264", NULL, NULL) < 0 )
	{
	}

	// Analyse h264 file.
	if ( avformat_find_stream_info(fmt_cont, NULL) < 0 )
	{
	}

	AVCodecContext *codecContext = fmt_cont->streams[0]->codec;

	AVCodec *decoder = avcodec_find_decoder(codecContext->codec_id);
	if( !decoder )
	{
	}

	if ( avcodec_open2(codecContext, decoder, NULL) < 0 )
	{
	}

	int const WIDTH = codecContext->width, HEIGHT = codecContext->height;
	int left_pitch = WIDTH * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_IYUV);
	SDL_Rect left_rect = {0, 0, WIDTH, HEIGHT};

	int dec_buf_size = avpicture_get_size(codecContext->pix_fmt, WIDTH, HEIGHT);
	uint8_t *dec_buf = (uint8_t *)malloc(dec_buf_size);
	if( !dec_buf )
	{
	}

	AVFrame *frame = NULL;
	AVPacket pkt;

	int got_frame = 0;
	while( true )
	{
		av_init_packet( &pkt );
		if ( av_read_frame(fmt_cont, &pkt) < 0 )
		{
			break; // End of the stream.
		}

		frame = av_frame_alloc();

		if ( avcodec_decode_video2(codecContext, frame, &got_frame, &pkt) < 0 )
		{
			printf(" avcodec_decode_video2 error !\n");
			break;
		}

		if ( got_frame )
		{
			avpicture_layout ( (AVPicture*)frame, codecContext->pix_fmt, WIDTH, HEIGHT, dec_buf, dec_buf_size );
			Painting(left_rect, dec_buf, left_pitch);

			SDL_Delay(50);
		}

		av_free(frame);
	}

	av_init_packet( &pkt );
	pkt.data = NULL;
	pkt.size = 0;
	frame = av_frame_alloc();
	if ( avcodec_decode_video2(codecContext, frame, &got_frame, &pkt) < 0 )
	{
		printf(" avcodec_decode_video2 error !\n");
		exit ( -1 );
	}

	if ( got_frame )
	{
		avpicture_layout ( (AVPicture*)frame, codecContext->pix_fmt, WIDTH, HEIGHT, dec_buf, dec_buf_size );
		Painting(left_rect, dec_buf, left_pitch);
	}

	if ( frame )
	{
		av_free ( frame );
	}

	if ( dec_buf )
	{
		free ( dec_buf );
	}

	avformat_close_input(&fmt_cont);
}
