#ifndef __MONITOR_SCREEN__
#define __MONITOR_SCREEN__

#include <thread>
#include <mutex>
#include "PoolThread.hpp"

using std::lock_guard;
using std::mutex;
using std::thread;

typedef struct vid_channel
{
    pjmedia_dir		    dir;	    /**< Channel direction.	    */
    pjmedia_port	    port;	    /**< Port interface.	    */
    unsigned		    pt;		    /**< Payload type.		    */
    pj_bool_t		    paused;	    /**< Paused?.		    */
    void		       *buf;	    /**< Output buffer.		    */
    unsigned		    buf_size;	/**< Size of output buffer.	    */
    pjmedia_rtp_session rtp;	/**< RTP session.		    */
} vid_channel_t;

typedef struct vid_stream
{
	pj_pool_t         *own_pool;        /**< Internal pool.		    */
	vid_channel_t     *dec;	            /**< Decoding channel.	    */
	pj_mutex_t        *jb_mutex;
    pjmedia_jbuf      *jb;	            /**< Jitter buffer.		    */
	pjmedia_vid_codec *codec;	        /**< Codec instance being used. */
	unsigned           dec_max_size;    /**< Size of decoded/raw picture*/
	pjmedia_frame      dec_frame;	    /**< Current decoded frame.     */
	unsigned           rx_frame_cnt;    /**< # of array in rx_frames    */
    pjmedia_frame     *rx_frames;	    /**< Temp. buffer for incoming frame assembly.	    */
} vid_stream_t;

class Screen
	: public CWnd
{
public:
	Screen(pj_uint32_t);
	virtual ~Screen();
	pj_status_t Prepare(const CRect &, const CWnd *, pj_uint32_t);
	pj_status_t Launch();
	void Refresh(const CRect &);
	void Hide();
	void Painting(const SDL_Rect &, const void *, int);
	void OnRxAudio(const pj_uint8_t *, pj_uint16_t);
	void OnRxVideo(const pj_uint8_t *, pj_uint16_t);

protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	pj_status_t decode_vid_frame();

private:
	const CWnd       *wrapper_;
	const pj_uint32_t index_;
	std::mutex        mutex_;
	CRect             rect_;
	pj_uint32_t       uid_;
	SDL_Window       *window_;
	SDL_Renderer     *render_;
	SDL_Texture      *texture_;
	pj_bool_t         active_;
	pj_uint32_t       call_status;
	vid_stream_t      stream;
	PoolThread<std::function<void ()>> sync_thread_pool_;
};

#endif
