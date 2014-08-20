#include "stdafx.h"
#include "RTPScene.h"

RTPParameter::RTPParameter()
	: UdpParameter()
{
}

void RTPScene::Maintain(UdpParameter *parameter)
{
	RTPParameter *param = reinterpret_cast<RTPParameter *>(parameter);

}
