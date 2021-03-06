/* vim: set ts=2 sw=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by IBM Corporation are Copyright (C) 2003
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdlib.h>
#include <stdio.h>
#include "plbase64.h"
#include "nsString.h"
#include "nsNativeCharsetUtils.h"
#include "nsReadableUtils.h"
#include "prmem.h"

/*
 * ReadNTLM : reads NTLM messages.
 *
 * based on http://davenport.sourceforge.net/ntlm.html
 */

#define kNegotiateUnicode               0x00000001
#define kNegotiateOEM                   0x00000002
#define kRequestTarget                  0x00000004
#define kUnknown1                       0x00000008
#define kNegotiateSign                  0x00000010
#define kNegotiateSeal                  0x00000020
#define kNegotiateDatagramStyle         0x00000040
#define kNegotiateLanManagerKey         0x00000080
#define kNegotiateNetware               0x00000100
#define kNegotiateNTLMKey               0x00000200
#define kUnknown2                       0x00000400
#define kUnknown3                       0x00000800
#define kNegotiateDomainSupplied        0x00001000
#define kNegotiateWorkstationSupplied   0x00002000
#define kNegotiateLocalCall             0x00004000
#define kNegotiateAlwaysSign            0x00008000
#define kTargetTypeDomain               0x00010000
#define kTargetTypeServer               0x00020000
#define kTargetTypeShare                0x00040000
#define kNegotiateNTLM2Key              0x00080000
#define kRequestInitResponse            0x00100000
#define kRequestAcceptResponse          0x00200000
#define kRequestNonNTSessionKey         0x00400000
#define kNegotiateTargetInfo            0x00800000
#define kUnknown4                       0x01000000
#define kUnknown5                       0x02000000
#define kUnknown6                       0x04000000
#define kUnknown7                       0x08000000
#define kUnknown8                       0x10000000
#define kNegotiate128                   0x20000000
#define kNegotiateKeyExchange           0x40000000
#define kNegotiate56                    0x80000000

static const char NTLM_SIGNATURE[] = "NTLMSSP";
static const char NTLM_TYPE1_MARKER[] = { 0x01, 0x00, 0x00, 0x00 };
static const char NTLM_TYPE2_MARKER[] = { 0x02, 0x00, 0x00, 0x00 };
static const char NTLM_TYPE3_MARKER[] = { 0x03, 0x00, 0x00, 0x00 };

#define NTLM_MARKER_LEN 4
#define NTLM_TYPE1_HEADER_LEN 32
#define NTLM_TYPE2_HEADER_LEN 32
#define NTLM_TYPE3_HEADER_LEN 64

#define LM_HASH_LEN 16
#define LM_RESP_LEN 24

#define NTLM_HASH_LEN 16
#define NTLM_RESP_LEN 24

static void PrintFlags(PRUint32 flags)
{
#define TEST(_flag) \
  if (flags & k ## _flag) \
    printf("    0x%08x (" # _flag ")\n", k ## _flag)

  TEST(NegotiateUnicode);
  TEST(NegotiateOEM);
  TEST(RequestTarget);
  TEST(Unknown1);
  TEST(NegotiateSign);
  TEST(NegotiateSeal);
  TEST(NegotiateDatagramStyle);
  TEST(NegotiateLanManagerKey);
  TEST(NegotiateNetware);
  TEST(NegotiateNTLMKey);
  TEST(Unknown2);
  TEST(Unknown3);
  TEST(NegotiateDomainSupplied);
  TEST(NegotiateWorkstationSupplied);
  TEST(NegotiateLocalCall);
  TEST(NegotiateAlwaysSign);
  TEST(TargetTypeDomain);
  TEST(TargetTypeServer);
  TEST(TargetTypeShare);
  TEST(NegotiateNTLM2Key);
  TEST(RequestInitResponse);
  TEST(RequestAcceptResponse);
  TEST(RequestNonNTSessionKey);
  TEST(NegotiateTargetInfo);
  TEST(Unknown4);
  TEST(Unknown5);
  TEST(Unknown6);
  TEST(Unknown7);
  TEST(Unknown8);
  TEST(Negotiate128);
  TEST(NegotiateKeyExchange);
  TEST(Negotiate56);

#undef TEST
}

static void
PrintBuf(const char *tag, const PRUint8 *buf, PRUint32 bufLen)
{
  int i;

  printf("%s =\n", tag);
  while (bufLen > 0)
  {
    int count = bufLen;
    if (count > 8)
      count = 8;

    printf("    ");
    for (i=0; i<count; ++i)
    {
      printf("0x%02x ", int(buf[i]));
    }
    for (; i<8; ++i)
    {
      printf("     ");
    }

    printf("   ");
    for (i=0; i<count; ++i)
    {
      if (isprint(buf[i]))
        printf("%c", buf[i]);
      else
        printf(".");
    }
    printf("\n");

    bufLen -= count;
    buf += count;
  }
}

static PRUint16
ReadUint16(const PRUint8 *&buf)
{
  PRUint16 x;
#ifdef IS_BIG_ENDIAN
  x = ((PRUint16) buf[1]) | ((PRUint16) buf[0] << 8);
#else
  x = ((PRUint16) buf[0]) | ((PRUint16) buf[1] << 8);
#endif
  buf += sizeof(x);
  return x;
}

static PRUint32
ReadUint32(const PRUint8 *&buf)
{
  PRUint32 x;
#ifdef IS_BIG_ENDIAN
  x = ( (PRUint32) buf[3])        |
      (((PRUint32) buf[2]) << 8)  |
      (((PRUint32) buf[1]) << 16) |
      (((PRUint32) buf[0]) << 24);
#else
  x = ( (PRUint32) buf[0])        |
      (((PRUint32) buf[1]) << 8)  |
      (((PRUint32) buf[2]) << 16) |
      (((PRUint32) buf[3]) << 24);
#endif
  buf += sizeof(x);
  return x;
}

typedef struct {
  PRUint16 length;
  PRUint16 capacity;
  PRUint32 offset;
} SecBuf;

static void
ReadSecBuf(SecBuf *s, const PRUint8 *&buf)
{
  s->length = ReadUint16(buf);
  s->capacity = ReadUint16(buf);
  s->offset = ReadUint32(buf);
}

static void
ReadType1MsgBody(const PRUint8 *inBuf, PRUint32 start)
{
  const PRUint8 *cursor = inBuf + start;
  PRUint32 flags;

  PrintBuf("flags", cursor, 4);
  // read flags
  flags = ReadUint32(cursor);
  PrintFlags(flags);

  // type 1 message may not include trailing security buffers
  if ((flags & kNegotiateDomainSupplied) | 
      (flags & kNegotiateWorkstationSupplied))
  {
    SecBuf secbuf;
    ReadSecBuf(&secbuf, cursor);
    PrintBuf("supplied domain", inBuf + secbuf.offset, secbuf.length);

    ReadSecBuf(&secbuf, cursor);
    PrintBuf("supplied workstation", inBuf + secbuf.offset, secbuf.length);
  }
}

static void
ReadType2MsgBody(const PRUint8 *inBuf, PRUint32 start)
{
  PRUint16 targetLen, offset;
  PRUint32 flags;
  const PRUint8 *target;
  const PRUint8 *cursor = inBuf + start;

  // read target name security buffer
  targetLen = ReadUint16(cursor);
  ReadUint16(cursor); // discard next 16-bit value
  offset = ReadUint32(cursor); // get offset from inBuf
  target = inBuf + offset;

  PrintBuf("target", target, targetLen);

  PrintBuf("flags", cursor, 4);
  // read flags
  flags = ReadUint32(cursor);
  PrintFlags(flags);

  // read challenge
  PrintBuf("challenge", cursor, 8);
  cursor += 8;

  PrintBuf("context", cursor, 8);
  cursor += 8;

  SecBuf secbuf;
  ReadSecBuf(&secbuf, cursor);
  PrintBuf("target information", inBuf + secbuf.offset, secbuf.length);
}

static void
ReadType3MsgBody(const PRUint8 *inBuf, PRUint32 start)
{
  const PRUint8 *cursor = inBuf + start;

  SecBuf secbuf;

  ReadSecBuf(&secbuf, cursor); // LM response
  PrintBuf("LM response", inBuf + secbuf.offset, secbuf.length);

  ReadSecBuf(&secbuf, cursor); // NTLM response
  PrintBuf("NTLM response", inBuf + secbuf.offset, secbuf.length);

  ReadSecBuf(&secbuf, cursor); // domain name
  PrintBuf("domain name", inBuf + secbuf.offset, secbuf.length);

  ReadSecBuf(&secbuf, cursor); // user name
  PrintBuf("user name", inBuf + secbuf.offset, secbuf.length);

  ReadSecBuf(&secbuf, cursor); // workstation name
  PrintBuf("workstation name", inBuf + secbuf.offset, secbuf.length);

  ReadSecBuf(&secbuf, cursor); // session key
  PrintBuf("session key", inBuf + secbuf.offset, secbuf.length);

  PRUint32 flags = ReadUint32(cursor);
  PrintBuf("flags", (const PRUint8 *) &flags, sizeof(flags));
  PrintFlags(flags);
}

static void
ReadMsg(const char *base64buf, PRUint32 bufLen)
{
  PRUint8 *inBuf = (PRUint8 *) PL_Base64Decode(base64buf, bufLen, NULL);
  if (!inBuf)
  {
    printf("PL_Base64Decode failed\n");
    return;
  }

  const PRUint8 *cursor = inBuf;

  PrintBuf("signature", cursor, 8);

  // verify NTLMSSP signature
  if (memcmp(cursor, NTLM_SIGNATURE, sizeof(NTLM_SIGNATURE)) != 0)
  {
    printf("### invalid or corrupt NTLM signature\n");
  }
  cursor += sizeof(NTLM_SIGNATURE);

  PrintBuf("message type", cursor, 4);

  if (memcmp(cursor, NTLM_TYPE1_MARKER, sizeof(NTLM_MARKER_LEN)) == 0)
    ReadType1MsgBody(inBuf, 12);
  else if (memcmp(cursor, NTLM_TYPE2_MARKER, sizeof(NTLM_MARKER_LEN)) == 0)
    ReadType2MsgBody(inBuf, 12);
  else if (memcmp(cursor, NTLM_TYPE3_MARKER, sizeof(NTLM_MARKER_LEN)) == 0)
    ReadType3MsgBody(inBuf, 12);
  else
    printf("### invalid or unknown message type\n"); 

  PR_Free(inBuf);
}

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    printf("usage: ntlmread <msg>\n");
    return -1;
  }
  ReadMsg(argv[1], (PRUint32) strlen(argv[1]));
  return 0;
}
