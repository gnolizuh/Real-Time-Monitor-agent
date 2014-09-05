#ifndef __AVS_PROXY_CLIENT_CONFIG__
#define __AVS_PROXY_CLIENT_CONFIG__

#include <mutex>
#include "Com.h"

using std::lock_guard;
using std::mutex;

class Config
{
public:
	pj_str_t    local_ip;
	pj_uint16_t local_media_port;
	pj_str_t    log_file_name;
	pj_str_t    tls_host;
	pj_str_t    tls_uri;
};

class Pipe
{
public:
	pj_status_t Create()
	{
		SECURITY_ATTRIBUTES saAttr; 

		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
		saAttr.bInheritHandle = TRUE; 
		saAttr.lpSecurityDescriptor = NULL;

		return CreatePipe(&read_, &write_, &saAttr, 0) ? PJ_SUCCESS : PJ_EINVAL;
	}

	pj_status_t Read(vector<pj_uint8_t> &buffer)
	{
		lock_guard<mutex> lock(lock_);
		DWORD readlen;
		char tmp[256];
		BOOL success = ReadFile(read_, tmp, sizeof(tmp), &readlen, NULL);
		RETURN_VAL_IF_FAIL(success, PJ_ETIMEDOUT);

		buffer.assign(tmp, tmp + readlen);
		return PJ_SUCCESS;
	}

	pj_status_t Write(const vector<pj_uint8_t> &buffer)
	{
		lock_guard<mutex> lock(lock_);
		DWORD writelen;
		BOOL success = WriteFile(write_, &buffer[0], buffer.size(), &writelen, 0);
		RETURN_VAL_IF_FAIL(success, PJ_EINVAL);

		return PJ_SUCCESS;
	}

private:
	mutex  lock_;
	HANDLE read_;
	HANDLE write_;
};

#endif
