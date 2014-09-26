#include "stdafx.h"
#include "DiscProxyScene.h"

void DiscProxyScene::Maintain(AvsProxy *&avs_proxy)
{
	if(avs_proxy)
	{
		avs_proxy->Destory();

		delete avs_proxy;
		avs_proxy = nullptr;
	}
}
