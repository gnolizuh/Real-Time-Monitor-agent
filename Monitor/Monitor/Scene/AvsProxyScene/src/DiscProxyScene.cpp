#include "stdafx.h"
#include "DiscProxyScene.h"

void DiscProxyScene::Maintain(AvsProxy *&avs_proxy)
{
	if(avs_proxy)
	{
		if(avs_proxy->pfunction_)
		{
			delete avs_proxy->pfunction_;
			avs_proxy->pfunction_ = nullptr;
		}

		delete avs_proxy;
		avs_proxy = nullptr;
	}
}
