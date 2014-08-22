#ifndef __AVS_PROXY_CLIENT_SCREEN__
#define __AVS_PROXY_CLIENT_SCREEN__

#include <vector>
#include <thread>
#include <mutex>
#include "resource.h"
#include "RoomTreeCtl.h"
#include "PoolThread.hpp"
#include "AvsProxyStructs.h"

using std::lock_guard;
using std::mutex;
using std::thread;
using std::vector;

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
	pj_uint32_t		   last_dec_ts;   /**< Last decoded timestamp.    */
    int			       last_dec_seq;  /**< Last decoded sequence.     */
} vid_stream_t;

class Screen
	: public CWnd
{
public:
	Screen(pj_uint32_t index,
		pj_sock_t &ref_tcp_sock,
		mutex &ref_tcp_lock);
	virtual ~Screen();
	pj_status_t Prepare(const CRect &, const CWnd *, pj_uint32_t);
	pj_status_t Launch();
	void        Destory();
	pj_status_t LinkRoomUser(User *user);
	void MoveToRect(const CRect &);
	void HideWindow();
	void Painting(const SDL_Rect &, const void *, int);
	void AudioScene(const pj_uint8_t *rtp_frame, pj_uint16_t framelen);
	void OnRxAudio(vector<pj_uint8_t> &audio_frame);
	void VideoScene(const pj_uint8_t *rtp_frame, pj_uint16_t framelen);
	void OnRxVideo(vector<pj_uint8_t> &video_frame);

protected:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	pj_status_t SendTCPPacket(const void *buf, pj_ssize_t *len);
	pj_status_t decode_vid_frame();

private:
	const CWnd       *wrapper_;
	const pj_uint32_t index_;
	mutex             render_mutex_;
	CRect             rect_;
	pj_uint32_t       uid_;
	SDL_Window       *window_;
	SDL_Renderer     *render_;
	SDL_Texture      *texture_;
	pj_bool_t         active_;
	pj_uint32_t       call_status_;
	vid_stream_t      stream_;
	pj_sock_t        &ref_tcp_sock_;
	mutex            &ref_tcp_lock_;
	PoolThread<std::function<void ()>> audio_thread_pool_;
	PoolThread<std::function<void ()>> video_thread_pool_;
};

#endif
