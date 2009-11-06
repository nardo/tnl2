//-----------------------------------------------------------------------------------
//
//   Torque Network Library - TNLTest dedicated server and unit tests
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "testGame.h"
#include <stdio.h>
#include "../tnl/tnlSymmetricCipher.h"
#include "../tnl/tnlAsymmetricKey.h"
#include "../tnl/tnlCertificate.h"
#include "../tnl/tnlBitStream.h"

using namespace TNL;

//#define TNLTest_SERVER
#define TNL_TEST_RPC
//#define TNL_TEST_PROTOCOL
//#define TNL_TEST_RPC
//#define TNL_TEST_NETINTERFACE
//#define TNL_TEST_CERTIFICATE

// server.cpp
//
// This file contains several different test programs, one of which
// can be enabled by uncommenting one of the previous #define'd macros.
//
// All of the test programs use the following LogConsumer to write
// output from the TNL to the standard output console.

class DedicatedServerLogConsumer : public LogConsumer
{
public:
   void logString(const char *string)
   {
      printf("%s\n", string);
   }
} gDedicatedServerLogConsumer;


#if defined(TNLTest_SERVER)

// The TNLTest_SERVER test is an example dedicated server for the
// TNLTest application.

int main(int argc, const char **argv)
{
   const char *localBroadcastAddress = "IP:broadcast:28999";

   S32 port = 28999;
   if(argc == 2)
      port = atoi(argv[1]);
   serverGame = new TestGame(true, Address(IPProtocol, Address::Any, port), Address(localBroadcastAddress));
   for(;;)
      serverGame->tick();
   return 0;
}

#elif defined(TNL_TEST_RPC)

// The TNL_TEST_RPC exercises the RPC functionality of TNL.  The RPC
// code is the only code that is challenging to port to new platforms
// because of its reliance on platform specific stack structure and
// function invocation guidelines.
//
// All RPC methods are declared to be virtual and are invoked using a
// virtual member function pointer.  The assembly language output of the
// VFTester calls below can be used to determine how to invoke virtual member
// functions on different platforms and compilers.

class RPCTestConnection : public EventConnection
{
public:

   /// Test RPC function for long argument lists, especially useful in debugging architectures that pass some arguments in registers, and some on the stack.
   TNL_DECLARE_RPC(rpcLongArgChainTest, (Float<7> i1, Float<6> i2, F32 i3, F32 i4, RangedU32<0,40> i5, F32 i6, U32 i7, F32 i8, U32 i9));

   TNL_DECLARE_RPC(rpcVectorTest, (IPAddress theIP, ByteBufferPtr buf, Vector<IPAddress> v0, Vector<StringPtr> v1, Vector<F32> v2));
   TNL_DECLARE_RPC(rpcVectorTest2, (Vector<F32> v, F32 v2));
   TNL_DECLARE_RPC(rpcSTETest, (StringTableEntry e1, Vector<StringTableEntry> v2));

};

TNL_IMPLEMENT_RPC(RPCTestConnection, rpcLongArgChainTest, (Float<7> i1, Float<6> i2, F32 i3, F32 i4, RangedU32<0,40> i5, F32 i6, U32 i7, F32 i8, U32 i9), 
   (i1, i2, i3, i4, i5, i6, i7, i8, i9),
      NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   F32 fi1 = i1;
   F32 fi2 = i2;
   logprintf( "%g %g %g %g %d %g %d %g %d", 
             fi1, fi2, i3, i4, U32(i5), i6, i7, i8, i9);
}

TNL_IMPLEMENT_RPC(RPCTestConnection, rpcVectorTest2, (Vector<F32> floatVec, F32 floatVal), (floatVec, floatVal),
      NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   logprintf("val = %g", floatVal);
   for(S32 i = 0; i < floatVec.size(); i++)
      logprintf("floatVec[%d] = %g", i, floatVec[i]);
}

TNL_IMPLEMENT_RPC(RPCTestConnection, rpcVectorTest,
    (IPAddress theIP, ByteBufferPtr buf, Vector<IPAddress> v0, Vector<StringPtr> v1, Vector<F32> v2),
    (theIP, buf, v0, v1, v2),
      NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   logprintf("Vector 0 size: %d, Vector 1 size: %d, Vector 2 size: %d", v0.size(), v1.size(), v2.size());
   Address anAddress(theIP);
   logprintf("BB: %d - %s", buf->getBufferSize(), buf->getBuffer());
   logprintf("IPAddress = %s", anAddress.toString());
   for(S32 i = 0; i < v0.size(); i++)
      logprintf("Addr: %s", Address(v0[i]).toString());
   for(S32 i = 0; i < v1.size(); i++)
      logprintf("%s", v1[i].getString());
   for(S32 i = 0; i < v2.size(); i++)
      logprintf("%g", v2[i]);
}

TNL_IMPLEMENT_RPC(RPCTestConnection, rpcSTETest,
    (StringTableEntry e1, Vector<StringTableEntry> v2),
    (e1, v2),
      NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   logprintf("e1 = %s", e1.getString());
   for(S32 i = 0; i < v2.size(); i++) 
   {
      logprintf("%d - %s", i, v2[i].getString());
   }
}

class VFTester
{
public:
   U32 someValue;
   VFTester() { someValue = 100; }
   virtual void doSomething(U32 newValue);
};

void VFTester::doSomething(U32 newValue)
{
   someValue = newValue;
}

typedef void (VFTester::*fptr)(U32 newValue);

struct U32Struct
{
  U32 value;
};

int main(int argc, const char **argv)
{
   // The assembly output of the following lines is used to determine
   // how the RPC executor should dispatch RPC calls.
   VFTester *newTester = new VFTester;
   fptr thePtr = &VFTester::doSomething;
   (newTester->*thePtr)(50);

   RPCTestConnection *tc = new RPCTestConnection;

   printf("sizeof RangedU32 = %d\n", sizeof(RangedU32<0,U32_MAX>));
   printf("sizeof Float<> = %d\nSizeof Int<>=%d\n",sizeof(Float<12>),sizeof(Int<12>));
   printf("sizeof U32Struct = %d\n", sizeof(U32Struct));
   fflush(stdout);

   tc->rpcLongArgChainTest_test(0.25, 0.5, 3, 4, 5, 6, 7, 8, 9);
   Vector<StringPtr> v1;
   Vector<IPAddress> v0;
   v0.push_back(Address("IP:192.168.0.1:1").toIPAddress());
   v0.push_back(Address("IP:192.168.0.2:3").toIPAddress());
   v0.push_back(Address("IP:192.168.0.4:5").toIPAddress());
   v0.push_back(Address("IP:192.168.0.6:7").toIPAddress());
   v1.push_back("Hello World");
   v1.push_back("Foo bar");
   v1.push_back("what's up dood?");
   v1.push_back("whoziwhatsit");
   Vector<F32> v2;
   v2.push_back(1.5);
   v2.push_back(2.5);
   v2.push_back(3.5);
   v2.push_back(4.5);
   v2.push_back(5.5);

   tc->rpcVectorTest2_test(v2, 50);
   Address theAddress("IP:192.168.0.23:6969");
   ByteBufferPtr buf = new ByteBuffer((U8 *) "Simple test buffer.", 20);

   Vector<StringTableEntry> v3;
   v3.push_back("Foo Bar!");
   v3.push_back("Testing 1 2 3");

   tc->rpcSTETest_test("Hello World!!!", v3);
   tc->rpcVectorTest_test(theAddress.toIPAddress(), buf, v0, v1, v2);
   delete tc;
}

#elif defined(TNL_TEST_PROTOCOL)

// The TNL_TEST_PROTOCOL section is used to test the underlying NetConnection
// Notify Protocol.  This section is not generally useful, but can help
// debug changes to the underlying protocol.

const U32 testValue = 0xF0F1BAAD;

class ProtocolTestConnection : public NetConnection 
{
   U32 sequence;
   U32 mConNum;
public:
   ProtocolTestConnection(U32 conNum = 0)
   {
      mConNum = conNum; sequence = 0; 
      U8 symmetricKey[16] = { 127, 31, 79, 202, 251, 81, 179, 67, 
                                 90, 151, 216, 10, 171, 243, 1, 0 };

      U8 initVector[16] = { 117, 42, 81, 222, 211, 85, 189, 17, 
                                 92, 159, 6, 102, 179, 2, 100, 0 };

      setSymmetricCipher(new SymmetricCipher(symmetricKey, initVector));
   }

   void writePacket(BitStream *bstream, PacketNotify *note)
   {
      logprintf("%d: sending packet - sequence = %d", mConNum, sequence);
      bstream->write(testValue);
      bstream->write(sequence++);
   }
   void readPacket(BitStream *bstream)
   {
      U32 val;
      bstream->read(&val);
      U32 seq;
      bstream->read(&seq);
      if(val != testValue)
         logprintf("%d: ERROR - read bad test value.", mConNum);
      //logprintf("%d: received packet - sequence = %d", mConNum, seq);
   }
};

int main(int argc, const char **argv)
{
   TNLLogEnable(LogConnectionProtocol, true);

   ProtocolTestConnection *t1 = new ProtocolTestConnection(1);
   ProtocolTestConnection *t2 = new ProtocolTestConnection(2);
   t1->setRemoteConnectionObject(t2);
   t2->setRemoteConnectionObject(t1);

   t1->setInitialRecvSequence(t2->getInitialSendSequence());
   t2->setInitialRecvSequence(t1->getInitialSendSequence());

   t1->checkPacketSend(true,0);
   t2->checkPacketSend(true,0);

   for(U32 i = 0; i < 15; i++)
      t1->checkPacketSend(true,0);
   for(U32 i = 0; i < 3; i++)
      t2->checkPacketSend(true,0);
   for(U32 i = 0; i < 72; i++)
      t1->checkPacketSend(true,0);
   for(U32 i = 0; i < 41; i++)
      t2->checkPacketSend(true,0);
   for(U32 i = 0; i < 15; i++)
      t1->checkPacketSend(true,0);
   for(U32 i = 0; i < 3; i++)
      t2->checkPacketSend(true,0);
   for(U32 i = 0; i < 15; i++)
      t1->checkPacketSend(true,0);
   for(U32 i = 0; i < 3; i++)
      t2->checkPacketSend(true,0);
}

#elif defined(TNL_TEST_NETINTERFACE)

// The TNL_TEST_NETINTERFACE section is used to test the connection
// handshaking process in NetInterface, including key exchange and
// encryption.

class NetInterfaceTestConnection : public NetConnection 
{
   public:
      TNL_DECLARE_NETCONNECTION(NetInterfaceTestConnection);
};

TNL_IMPLEMENT_NETCONNECTION(NetInterfaceTestConnection, NetClassGroupGame, true);

int main(int argc, const char **argv)
{
   TNLLogEnable(LogNetInterface, true);

   NetInterface *clientInterface = new NetInterface(25000);
   NetInterface *serverInterface = new NetInterface(25001);

   serverInterface->setAllowsConnections(true);
   serverInterface->setRequiresKeyExchange(true);

   serverInterface->setPrivateKey(new AsymmetricKey(32));
   NetInterfaceTestConnection *clientConnection = new NetInterfaceTestConnection;

   Address addr("IP:127.0.0.1:25001");
   clientConnection->connect(clientInterface, &addr, true, false);

   for(;;)
   {
      serverInterface->checkIncomingPackets();
      serverInterface->processConnections();
      clientInterface->checkIncomingPackets();
      clientInterface->processConnections();
   }
}

#elif defined(TNL_TEST_CERTIFICATE)

int main(int argc, const char **argv)
{
   RefPtr<AsymmetricKey> signatoryKey = new AsymmetricKey(32);
   RefPtr<AsymmetricKey> certificateKey = new AsymmetricKey(32);

   U8 *certificateString = (U8 *) "This is a sample certificate payload.";
   ByteBuffer thePayload(certificateString, strlen((const char *) certificateString) + 1);

   RefPtr<Certificate> theCertificate = new Certificate(thePayload, certificateKey, signatoryKey);

   PacketStream streamTest;

   streamTest.write(theCertificate);
   streamTest.setBytePosition(0);
   
   RefPtr<Certificate> readCert = new Certificate(&streamTest);

   logprintf("Read certificate: %s", readCert->getPayload()->getBuffer());
   
   logprintf("Checking certificate signature validity...");
   bool isvalid = readCert->validate(signatoryKey);

   logprintf("Certificate %s valid.", isvalid ? "IS" : "IS NOT");
}

#endif
