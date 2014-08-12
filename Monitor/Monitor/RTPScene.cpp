#include "stdafx.h"
#include "RTPScene.h"

RTPParameter::RTPParameter()
	: UdpParameter()
{
}

void RTPScene::Maintain(TcpParameter *parameter)
{
	RTPParameter *param = reinterpret_cast<RTPParameter *>(parameter);

}
