#include "sgp_parser.h"
#include "crc.h"
#include "sys_time.h"
#include "stdlib.h"
#include "cl_event_system.h"

typedef enum
{
    PS_Head,    //2 bytes
    PS_MsgType, //1 byte
    PS_Length,  //1 byte
    PS_Data,    //n bytes
    PS_Verify,  //2 byte
} ParseStatus_t;

typedef struct
{
    int8_t handle;
    SgpRecvMsg_t recvMsg;
    SGP_SendFunc sendFunc;
    ParseStatus_t parseStatus; // = PS_Head;
    uint16_t headCount;        // = 0;
    int recvDataCount;         // = 0;
    uint8_t verifyCount;       // = 0;
    uint16_t verifyValue;
} SgpChannel_t;

static SgpChannel_t sgpChannels[SpgChannelHandle_Max];

static inline SgpChannel_t *Handle2Channel(SpgChannelHandle_t handle)
{ // TODO Assert

    return sgpChannels + handle;
}

//protocol head
static const uint8_t protoHead[] = {0xfe, 0xef};

static void ToParseHead(SgpChannel_t *channel)
{
    channel->parseStatus = PS_Head;
    channel->headCount = 0;
}

static void ToParseCmd(SgpChannel_t *channel)
{
    channel->parseStatus = PS_MsgType;
}

static void ToParseLength(SgpChannel_t *channel)
{
    channel->parseStatus = PS_Length;
}

static void ToParseData(SgpChannel_t *channel)
{
    channel->parseStatus = PS_Data;
    channel->recvDataCount = 0;
}

static void ToParseVerify(SgpChannel_t *channel)
{
    channel->parseStatus = PS_Verify;
    channel->verifyCount = 0;
}

static void VeriyProcess(SgpChannel_t *channel, uint8_t byte)
{
    uint16_t crc = 0;
    switch (channel->verifyCount)
    {
    case 0:
        channel->verifyValue = ((uint16_t)byte) << 8; //verify code 1st byte
        break;
    case 1:
        channel->verifyValue |= (uint16_t)byte;                                       //verify code secode byte
        crc = CalcCRC16((uint8_t *)(&channel->recvMsg), 2 + channel->recvMsg.length); //calculate crc

        if (crc == channel->verifyValue)
        {
            CL_EventSysRaise(CL_Event_SgpRecvMsg, channel->handle, &channel->recvMsg);
            ToParseHead(channel); //reset parse status
        }
        else
        {
            //verify code mismatched
            ToParseHead(channel); //reset parse status
        }
        break;
    default:
        break;
    }

    channel->verifyCount++;
}

void SgpParser_RecvByte(SpgChannelHandle_t handle, uint8_t byte)
{
    SgpChannel_t *channel = Handle2Channel(handle); // TODO Assert

    static uint64_t lastRecvTime = 0;
    if (TimeElapsed(lastRecvTime) > SGP_FRAME_TIMEOUT)
    {
        //frame timeout
        ToParseHead(channel);
    }
    SetToCurTime(&lastRecvTime); //save current time

    switch (channel->parseStatus)
    {
    case PS_Head:
        if (byte == protoHead[channel->headCount])
        {
            //parse frame head
            channel->headCount++;
            if (channel->headCount >= CL_ARRAY_LENGTH(protoHead))
            {
                ToParseCmd(channel);
            }
        }
        else
        {
            channel->headCount = 0;
        }
        break;
    case PS_MsgType:
        channel->recvMsg.msgType = byte;
        ToParseLength(channel);
        break;
    case PS_Length:
        if (byte == 0)
        {
            //length is 0, means no data
            channel->recvMsg.length = 0;
            ToParseVerify(channel); //prepare to parse verify code
        }
        else
        {
            //length is not 0
            channel->recvMsg.length = byte;
            ToParseData(channel); //prepare to receive data
        }
        break;
    case PS_Data:
        channel->recvMsg.data[channel->recvDataCount++] = byte;
        if (channel->recvDataCount >= channel->recvMsg.length)
        {
            //receiving data
            ToParseVerify(channel);
        }
        break;
    case PS_Verify:
        VeriyProcess(channel, byte);
        break;
    default:
        break;
    }
}

void SgpParser_RecvData(SpgChannelHandle_t handle, const uint8_t *buff, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++)
    {
        SgpParser_RecvByte(handle, buff[i]);
    }
}

CL_Result_t SgpParser_SendMsg(SpgChannelHandle_t handle, uint8_t msgType, const uint8_t *data, uint8_t length)
{
    SgpChannel_t *channel = Handle2Channel(handle); // TODO Assert

    uint8_t mt[2];
    uint16_t crc = 0;
    CL_Result_t res;

    if (channel->sendFunc == CL_NULL)
        return CL_ResFailed;

    //send frame head
    res = channel->sendFunc(protoHead, sizeof(protoHead));
    if (res != CL_ResSuccess)
        return res;

    //send frame type and length of data
    mt[0] = msgType;
    mt[1] = length;
    res = channel->sendFunc(mt, 2);
    if (res != CL_ResSuccess)
        return res;

    //send data if it has
    if (length != 0)
    {
        res = channel->sendFunc(data, length);
        if (res != CL_ResSuccess)
            return res;
    }

    //crc
    crc = CalcCRC16(mt, 2);
    if (length != 0)
        crc = CalcCRC16Ex(crc, data, length);

    mt[0] = (crc >> 8) & 0xff;
    mt[1] = crc & 0xff;
    res = channel->sendFunc(mt, 2);
    if (res != CL_ResSuccess)
        return res;

    return CL_ResSuccess;
}

void SgpParser_Init(void)
{
    for (uint8_t i = 0; i < CL_ARRAY_LENGTH(sgpChannels); i++)
    {
        sgpChannels[i].handle = -1;
    }
}

void SgpParser_AddChannel(SpgChannelHandle_t handle, SGP_SendFunc s)
{
    SgpChannel_t *channel = Handle2Channel(handle);

    channel->handle = handle;
    channel->parseStatus = PS_Head;
    channel->headCount = 0;
    channel->recvDataCount = 0;
    channel->verifyCount = 0;
    channel->sendFunc = s;
}

void SgpParser_RmChannel(SpgChannelHandle_t handle)
{
    SgpChannel_t *channel = Handle2Channel(handle); // TODO Assert
    channel->handle = -1;
}
