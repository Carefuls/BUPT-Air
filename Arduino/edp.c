#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define CONNREQ             0x10
#define CONNRESP            0x20
#define PUSHDATA            0x30
#define SAVEDATA            0x80
#define SAVEACK             0x90
#define CMDREQ              0xA0
#define CMDRESP             0xB0
#define PINGREQ             0xC0
#define PINGRESP            0xD0
#define ENCRYPTREQ          0xE0
#define ENCRYPTRESP         0xF0

#define MAX_LEN				200
#define PROTOCOL_NAME       "EDP"
#define PROTOCOL_VERSION    1

typedef unsigned char   uint8;
typedef char            int8;
typedef unsigned int    uint16;
typedef int             int16;
typedef unsigned long   uint32;
typedef long            int32;

typedef struct
{
  uint8 data[MAX_LEN];
  int16 len;
  int16 read_p;
} edp_pkt;


/*
 * packetCreate
 * 鍒涘缓涓�涓狤DP鍖呯紦瀛樼┖闂�
 */
edp_pkt *packetCreate(void)
{
  edp_pkt *p;

  if((p = (edp_pkt *)malloc(sizeof(edp_pkt))) != NULL)
    memset(p, 0, sizeof(edp_pkt));
  return p;
}

/*
 * writeRemainlen
 * 鍚慐DP鍖呬腑鍐欏叆鍓╀綑闀垮害瀛楁
 * len_val: 鍓╀綑闀垮害鐨勫��
 */
int8 writeRemainlen(edp_pkt* pkt, int16 len_val)
{
  int8 remaining_count = 0;
  int8 tmp = 0;

  do {
    tmp = len_val % 128;
    len_val = len_val / 128;
    /* If there are more digits to encode, set the top bit of this digit */
    if (len_val > 0) {
      tmp = tmp | 0x80;
    }
    pkt->data[pkt->len++] = tmp;
    remaining_count++;
  } while (len_val > 0 && remaining_count < 5);

  return remaining_count;
}

/*
 * writeByte
 * 鍚慐DP鍖呬腑鍐欏叆涓�涓瓧鑺�
 */
int16 writeByte(edp_pkt* pkt, int8 byte)
{
  pkt->data[pkt->len++] = byte;
  return 0;
}

/*
 * writeBytes
 * 鍚慐DP鍖呬腑鍐欏叆澶氫釜瀛楄妭
 */
int16 writeBytes(edp_pkt* pkt, const void* bytes, int16 count)
{
  memcpy(pkt->data + pkt->len, bytes, count);
  pkt->len += count;
  return 0;
}


/*
 * writeStr
 * 鍚慐DP鍖呬腑鍐欏叆瀛楃涓插瓧娈�
 * 棣栧厛鍐欏叆涓や釜瀛楄妭鐨勯暱搴︼紝闅忓悗绱ц窡瀛楃涓插唴瀹�
 */
int16 writeStr(edp_pkt* pkt, const int8* str)
{
  short len = strlen(str);

  writeByte(pkt, len >> 8);
  writeByte(pkt, len & 0x00ff);

  memcpy(pkt->data + pkt->len, str, len);
  pkt->len += len;
  return 0;
}


/*---------------------------------------------------------------------------*/
/*
 * readUint8
 * 浠嶦DP鍖呬腑璇诲嚭涓�涓瓧鑺�
 */
uint8 readUint8(edp_pkt* pkt)
{
  return pkt->data[pkt->read_p++];
}

/*
 * readUint16
 * 浠嶦DP鍖呬腑璇诲嚭16bit鐨勫瓧娈�
 */
uint16 readUint16(edp_pkt* pkt)
{
  uint16 tmp;
  uint8 msb, lsb;
  
  msb = readUint8(pkt);
  lsb = readUint8(pkt);

  tmp = (msb<<8) | lsb;
  return tmp;
}

/*
 * readUint32
 * 浠嶦DP鍖呬腑璇诲嚭4涓瓧鑺傜殑瀛楁
 */
uint32 readUint32(edp_pkt* pkt)
{
  uint32 tmp = 0;
  int i = 4;

  while (--i >= 0) 
  {
    tmp <<= 8;
    tmp |= readUint8(pkt);
  }
  return tmp;
}

/*
 * readStr
 * 鏍规嵁闀垮害锛屼粠EDP鍖呬腑璇诲嚭瀛楃涓叉暟鎹�
 * len : 瀛楃涓茬殑闀垮害
 */
void readStr(edp_pkt* pkt, char* str, uint16 len)
{
  memcpy(str, pkt->data + pkt->read_p, len);
  pkt->read_p += len;
}

/*
 * readRemainlen
 * 浠嶦DP鍖呬腑璇诲嚭鍓╀綑闀垮害
 */
int32 readRemainlen(edp_pkt* pkt)
{
  uint32 multiplier = 1;
  uint32 len_len = 0;
  uint8 onebyte = 0;
  int32 len_val = 0;
  do 
  {
    onebyte = readUint8(pkt);

    len_val += (onebyte & 0x7f) * multiplier;
    multiplier *= 0x80;

    len_len++;
    if (len_len > 4) 
    {
      return -1; /*len of len more than 4;*/
    }
  } while((onebyte & 0x80) != 0);
  return len_val;
}

/*
 * packetConnect锛氱粍EDP杩炴帴鍖�
 * 棣栧厛鍒涘缓EDP缂撳瓨绌洪棿锛屾寜鐓DP鍗忚缁凟DP杩炴帴鍖�
 * 鍒嗛厤鐨勫唴瀛橀渶瑕佸湪鍙戦�佷箣鍚巉ree鎺�
 * devid: 璁惧id
 * key锛欰PIKey
 */
edp_pkt *packetConnect(const int8* devid, const int8* key)
{
  int32 remainlen;
  edp_pkt* pkt;
  
  if((pkt = packetCreate()) == NULL)
    return NULL;
  
  /* msg type */
  writeByte(pkt, CONNREQ);
  /* remain len */
  remainlen = (2 + 3) + 1 + 1 + 2 + (2 + strlen(devid)) + (2 + strlen(key));
  writeRemainlen(pkt, remainlen);
  /* protocol desc */
  writeStr(pkt, PROTOCOL_NAME);
  /* protocol version */
  writeByte(pkt, PROTOCOL_VERSION);
  /* connect flag */
  writeByte(pkt, 0x40);
  /* keep time */
  writeByte(pkt, 0);
  writeByte(pkt, 0x80);

  /* DEVID */
  writeStr(pkt, devid);
  /* auth key */
  writeStr(pkt, key);
  
  return pkt;
}


/*
 * packetDataSaveTrans锛氱粍EDP鏁版嵁瀛樺偍杞彂鍖�
 * 棣栧厛鍒涘缓EDP缂撳瓨绌洪棿锛屾寜鐓DP鍗忚缁凟DP鏁版嵁瀛樺偍杞彂鍖�
 * 鍒嗛厤鐨勫唴瀛橀渶瑕佸湪鍙戦�佷箣鍚巉ree鎺�
 * devid: 璁惧id
 * streamId锛氭暟鎹祦ID锛屽嵆鏁版嵁娴佸悕
 * val: 瀛楃涓插舰寮忕殑鏁版嵁鍊�
 */
edp_pkt *packetDataSaveTrans(const int8* destId, const int8* streamId, const int8 *val)
{
  int32 remainlen;
  int8 tmp[200];
  int16 str_len;
  edp_pkt *pkt;
  
  if((pkt = packetCreate()) == NULL)
    return pkt;

  /* 鐢熸垚鏁版嵁绫诲瀷鏍煎紡5鐨勬暟鎹被鍨� */
  sprintf(tmp, ",;%s,%s", streamId, val);
  str_len = strlen(tmp);

  /* msg type */
  writeByte(pkt, SAVEDATA);

  if (destId != NULL)
  {
    /* remain len */
    remainlen = 1 + (2 + strlen(destId)) + 1 + (2 + str_len);
    writeRemainlen(pkt, remainlen);
    /* translate address flag */
    writeByte(pkt, 0x80);
    /* dst devid */
    writeStr(pkt, destId);
  }
  else
  {
    /* remain len */
    remainlen = 1 + 1 + (2 + str_len);
    writeRemainlen(pkt, remainlen);
    /* translate address flag */
    writeByte(pkt, 0x00);
  }

  /* json flag */
  writeByte(pkt, 5);
  /* json */
  writeStr(pkt, tmp);
  
  return pkt;
}


void packetClear(edp_pkt* pkt)
{
  memset(pkt, 0, sizeof(edp_pkt));
}


/*
 * isEdpPkt
 * 鎸夌収EDP鏁版嵁鏍煎紡锛屽垽鏂槸鍚︽槸瀹屾暣鏁版嵁鍖�
 */
int16 isEdpPkt(edp_pkt* pkt)
{
  uint32 data_len = 0;
  uint32 multiplier = 1;
  uint32 len_val = 0;
  uint32 len_len = 1;
  uint32 pkt_total_len = 0;
  uint8* pdigit;

  pdigit = pkt->data;
  data_len = pkt->len;

  if (data_len <= 1)
  {
    return 0;   /* continue receive */
  }

  do {
    if (len_len > 4)
    {
      return -1;  /* protocol error; */
    }
    if (len_len > data_len - 1)
    {
      return 0;   /* continue receive */
    }
    len_len++;
    pdigit++;
    len_val += ((*pdigit) & 0x7f) * multiplier;
    multiplier *= 0x80;
  } while (((*pdigit) & 0x80) != 0);

  pkt_total_len = len_len + len_val;

  /* receive payload */
  if (pkt_total_len == data_len)
  {
    return 1;   /* all data for this pkt is read */
  }
  else
  {
    return 0;   /* continue receive */
  }
}


/*
 * edpCommandReqParse
 * 鎸夌収EDP鍛戒护璇锋眰鍗忚锛岃В鏋愭暟鎹�
 */
int edpCommandReqParse(edp_pkt* pkt, char *id, char *cmd, int32 *rmlen, int32 *id_len, int32 *cmd_len)
{
  readUint8(pkt);     /* 鍖呯被鍨� */
  *rmlen = readRemainlen(pkt);    /* 鍓╀綑闀垮害 */
  *id_len = readUint16(pkt);      /* ID闀垮害 */
  readStr(pkt, id, *id_len);      /* 鍛戒护ID */
  *cmd_len = readUint32(pkt);     /* 鍛戒护闀垮害 */
  readStr(pkt, cmd, *cmd_len);    /* 鍛戒护鍐呭 */
}


/*
 * edpPushDataParse
 * 鎸夌収EDP閫忎紶鏁版嵁鏍煎紡锛岃В鏋愭暟鎹�
 */
int edpPushDataParse(edp_pkt* pkt, char *srcId, char *data)
{
  uint32 remain_len;
  uint16 id_len;
  
  readUint8(pkt);     /* 鍖呯被鍨� */
  remain_len = readRemainlen(pkt);    /* 鍓╀綑闀垮害 */
  id_len = readUint16(pkt);           /* 婧怚D闀垮害 */
  readStr(pkt, srcId, id_len);    /* 婧怚D */
  readStr(pkt, data, remain_len - 2 - id_len);    /* 鏁版嵁鍐呭 */
}


