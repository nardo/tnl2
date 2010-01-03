//-----------------------------------------------------------------------------------
//
//   Torque Core Library
//   Copyright (C) 2005 GarageGames.com, Inc.
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

#ifndef _CORE_METHODDISPATCH_H_
#define _CORE_METHODDISPATCH_H_

#ifndef _CORE_TYPES_H_
#include "core/coreTypes.h"
#endif

#ifndef _CORE_VECTOR_H_
#include "core/coreVector.h"
#endif

#ifndef _CORE_BITSTREAM_H_
#include "core/coreBitStream.h"
#endif

#ifndef _CORE_STRINGTABLE_H_
#include "core/coreStringTable.h"
#endif

#ifndef _CORE_STRING_H_
#include "core/coreString.h"
#endif

namespace TNL
{
   enum {
      VectorSizeBitSize = 8,
      ByteBufferSizeBitSize = 10,
   };

   /// Reads a string from a BitStream.
   extern void read(BitStream &s, StringPtr *val);
   /// Rrites a string into a BitStream.
   extern void write(BitStream &s, const StringPtr &val);
   /// Reads a ByteBuffer from a BitStream.
   extern void read(BitStream &s, ByteBufferPtr *val);
   /// Writes a ByteBuffer into a BitStream.
   extern void write(BitStream &s, const ByteBufferPtr &val);

   /// Reads a StringTableEntry from a BitStream.
   inline void read(BitStream &s, StringTableEntry *val)
   {
      s.readStringTableEntry(val);
   }
   /// Writes a StringTableEntry into a BitStream.
   inline void write(BitStream &s, const StringTableEntry &val)
   {
      s.writeStringTableEntry(val);
   }

   /// Reads a generic object from a BitStream.  This can be used for any
   /// type supported by BitStreamTNL::read.
   template <typename T> inline void read(BitStream &s, T *val)
   { 
      s.read(val);
   }
   /// Writes a generic object into a BitStream.  This can be used for any
   /// type supported by BitStreamTNL::write.
   template <typename T> inline void write(BitStream &s, const T &val)
   { 
      s.write(val);
   }
   /// Reads a Vector of objects from a BitStream.
   template <typename T> inline void read(BitStream &s, Vector<T> *val)
   {
      U32 size = s.readInt(VectorSizeBitSize);
      val->setSize(size);
      for(S32 i = 0; i < val->size(); i++)
         read(s, &((*val)[i]));
   }
   /// Writes a Vector of objects into a BitStream.
   template <typename T> void write(BitStream &s, const Vector<T> &val)
   {
      s.writeInt(val.size(), VectorSizeBitSize);
      for(S32 i = 0; i < val.size(); i++)
         write(s, val[i]);
   }
   /// Reads a bit-compressed integer from a BitStream.
   template <U32 BitCount> inline void read(BitStream &s, Int<BitCount> *val)
   {
      val->value = s.readInt(BitCount);
   }
   /// Writes a bit-compressed integer into a BitStream.
   template <U32 BitCount> inline void write(BitStream &s, const Int<BitCount> &val)
   {
      s.writeInt(val.value, BitCount);
   }

   /// Reads a bit-compressed signed integer from a BitStream.
   template <U32 BitCount> inline void read(BitStream &s, SignedInt<BitCount> *val)
   {
      val->value = s.readSignedInt(BitCount);
   }
   /// Writes a bit-compressed signed integer into a BitStream.
   template <U32 BitCount> inline void write(BitStream &s,const SignedInt<BitCount> &val)
   {
      s.writeSignedInt(val.value, BitCount);
   }

   /// Reads a bit-compressed RangedU32 from a BitStream.
   template <U32 MinValue, U32 MaxValue> inline void read(BitStream &s, RangedU32<MinValue,MaxValue> *val)
   {
      val->value = s.readRangedU32(MinValue, MaxValue);
   }

   /// Writes a bit-compressed RangedU32 into a BitStream.
   template <U32 MinValue, U32 MaxValue> inline void write(BitStream &s,const RangedU32<MinValue,MaxValue> &val)
   {
      s.writeRangedU32(val.value, MinValue, MaxValue);
   }

   /// Reads a bit-compressed SignedFloat (-1 to 1) from a BitStream.
   template <U32 BitCount> inline void read(BitStream &s, Float<BitCount> *val)
   {
      val->value = s.readFloat(BitCount);
   }
   /// Writes a bit-compressed SignedFloat (-1 to 1) into a BitStream.
   template <U32 BitCount> inline void write(BitStream &s,const Float<BitCount> &val)
   {
      s.writeFloat(val.value, BitCount);
   }
   /// Reads a bit-compressed Float (0 to 1) from a BitStream.
   template <U32 BitCount> inline void read(BitStream &s, SignedFloat<BitCount> *val)
   {
      val->value = s.readSignedFloat(BitCount);
   }
   /// Writes a bit-compressed Float (0 to 1) into a BitStream.
   template <U32 BitCount> inline void write(BitStream &s,const SignedFloat<BitCount> &val)
   {
      s.writeSignedFloat(val.value, BitCount);
   }
   /// Reads a generic object from a BitStream.  This can be used for any
   /// type supported by BitStreamTNL::read.
   template <typename T> inline void read(BitStream *s, T *val)
   { 
      read(*s, val);
   }
   /// Writes a generic object into a BitStream.  This can be used for any
   /// type supported by BitStreamTNL::write.
   template <typename T> inline void write(BitStream *s, const T &val)
   { 
      write(*s, val);
   }

};

namespace TNL {

/// Base class for FunctorDecl template classes.  The Functor objects
/// store the parameters and member function pointer for the invocation
/// of some class member function.  Functor is used in TNL by the
/// RPC mechanism, the journaling system and the ThreadQueue to store
/// a function for later transmission and dispatch, either to a remote
/// host, a journal file, or another thread in the process.
template<class S> struct Functor {
   /// Construct the Functor.
   Functor() {}
   /// Destruct the Functor.
   virtual ~Functor() {}
   /// Reads this Functor from a BitStream.
   virtual void read(S &stream) = 0;
   /// Writes this Functor to a BitStream.
   virtual void write(S &stream) = 0;
   /// Dispatch the function represented by the Functor.
   virtual void dispatch(void *t) = 0;
};

/// FunctorDecl template class.  This class is specialized based on the
/// member function call signature of the method it represents.  Other
/// specializations hold specific member function pointers and slots
/// for each of the function arguments.
template <class S, class T> 
struct FunctorDecl : public Functor<S> {
   FunctorDecl() {}
   void set() {}
   void read(S &stream) {}
   void write(S &stream) {}
   void dispatch(void *t) { }
};
template <class S, class T> 
struct FunctorDecl<S, void (T::*)()> : public Functor<S> {
   typedef void (T::*FuncPtr)();
   FuncPtr ptr;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set() {}
   void read(S &stream) {}
   void write(S &stream) {}
   void dispatch(void *t) { ((T *)t->*ptr)(); }
};
template <class S, class T, class A> 
struct FunctorDecl<S, void (T::*)(A)> : public Functor<S> {
   typedef void (T::*FuncPtr)(A);
   FuncPtr ptr; A a;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a) { a = _a; }
   void read(S &stream) { TNL::read(stream, &a); }
   void write(S &stream) { TNL::write(stream, a); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a); }
};
template <class S, class T, class A, class B>
struct FunctorDecl<S, void (T::*)(A,B)>: public Functor<S> {
   typedef void (T::*FuncPtr)(A,B);
   FuncPtr ptr; A a; B b;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b) { a = _a; b = _b;}
   void read(S &stream) { TNL::read(stream, &a); TNL::read(stream, &b); }
   void write(S &stream) { TNL::write(stream, a); TNL::write(stream, b); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a, b); }
};

template <class S, class T, class A, class B, class C>
struct FunctorDecl<S, void (T::*)(A,B,C)>: public Functor<S> {
   typedef void (T::*FuncPtr)(A,B,C);
   FuncPtr ptr; A a; B b; C c;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c) { a = _a; b = _b; c = _c;}
   void read(S &stream) { TNL::read(stream, &a); TNL::read(stream, &b); TNL::read(stream, &c); }
   void write(S &stream) { TNL::write(stream, a); TNL::write(stream, b); TNL::write(stream, c); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c); }
};

template <class S, class T, class A, class B, class C, class D>
struct FunctorDecl<S, void (T::*)(A,B,C,D)>: public Functor<S> {
   typedef void (T::*FuncPtr)(A,B,C,D);
   FuncPtr ptr; A a; B b; C c; D d;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d) { a = _a; b = _b; c = _c; d = _d; }
   void read(S &stream) { TNL::read(stream, &a); TNL::read(stream, &b); TNL::read(stream, &c); TNL::read(stream, &d); }
   void write(S &stream) { TNL::write(stream, a); TNL::write(stream, b); TNL::write(stream, c); TNL::write(stream, d); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d); }
};

template <class S, class T, class A, class B, class C, class D, class E>
struct FunctorDecl<S, void (T::*)(A,B,C,D,E)>: public Functor<S> {
   typedef void (T::*FuncPtr)(A,B,C,D,E);
   FuncPtr ptr; A a; B b; C c; D d; E e;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e) { a = _a; b = _b; c = _c; d = _d; e = _e; }
   void read(S &stream) { TNL::read(stream, &a); TNL::read(stream, &b); TNL::read(stream, &c); TNL::read(stream, &d); TNL::read(stream, &e); }
   void write(S &stream) { TNL::write(stream, a); TNL::write(stream, b); TNL::write(stream, c); TNL::write(stream, d); TNL::write(stream, e); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e); }
};

template <class S, class T, class A, class B, class C, class D, class E, class F>
struct FunctorDecl<S, void (T::*)(A,B,C,D,E,F)>: public Functor<S> {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; }
   void read(S &stream) { TNL::read(stream, &a); TNL::read(stream, &b); TNL::read(stream, &c); TNL::read(stream, &d); TNL::read(stream, &e); TNL::read(stream, &f); }
   void write(S &stream) { TNL::write(stream, a); TNL::write(stream, b); TNL::write(stream, c); TNL::write(stream, d); TNL::write(stream, e); TNL::write(stream, f); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f); }
};

template <class S, class T, class A, class B, class C, class D, class E, class F, class G>
struct FunctorDecl<S, void (T::*)(A,B,C,D,E,F,G)>: public Functor<S> {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; }
   void read(S &stream) { TNL::read(stream, &a); TNL::read(stream, &b); TNL::read(stream, &c); TNL::read(stream, &d); TNL::read(stream, &e); TNL::read(stream, &f); TNL::read(stream, &g); }
   void write(S &stream) { TNL::write(stream, a); TNL::write(stream, b); TNL::write(stream, c); TNL::write(stream, d); TNL::write(stream, e); TNL::write(stream, f); TNL::write(stream, g); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g); }
};

template <class S, class T, class A, class B, class C, class D, class E, class F, class G, class H>
struct FunctorDecl<S, void (T::*)(A,B,C,D,E,F,G,H)>: public Functor<S> {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; }
   void read(S &stream) { TNL::read(stream, &a); TNL::read(stream, &b); TNL::read(stream, &c); TNL::read(stream, &d); TNL::read(stream, &e); TNL::read(stream, &f); TNL::read(stream, &g); TNL::read(stream, &h); }
   void write(S &stream) { TNL::write(stream, a); TNL::write(stream, b); TNL::write(stream, c); TNL::write(stream, d); TNL::write(stream, e); TNL::write(stream, f); TNL::write(stream, g); TNL::write(stream, h); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h); }
};

template <class S, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I>
struct FunctorDecl<S, void (T::*)(A,B,C,D,E,F,G,H,I)>: public Functor<S> {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; }
   void read(S &stream) { TNL::read(stream, &a); TNL::read(stream, &b); TNL::read(stream, &c); TNL::read(stream, &d); TNL::read(stream, &e); TNL::read(stream, &f); TNL::read(stream, &g); TNL::read(stream, &h); TNL::read(stream, &i); }
   void write(S &stream) { TNL::write(stream, a); TNL::write(stream, b); TNL::write(stream, c); TNL::write(stream, d); TNL::write(stream, e); TNL::write(stream, f); TNL::write(stream, g); TNL::write(stream, h); TNL::write(stream, i); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i); }
};

template <class S, class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
struct FunctorDecl<S, void (T::*)(A,B,C,D,E,F,G,H,I,J)>: public Functor<S> {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; }
   void read(S &stream) { TNL::read(stream, &a); TNL::read(stream, &b); TNL::read(stream, &c); TNL::read(stream, &d); TNL::read(stream, &e); TNL::read(stream, &f); TNL::read(stream, &g); TNL::read(stream, &h); TNL::read(stream, &i); TNL::read(stream, &j); }
   void write(S &stream) { TNL::write(stream, a); TNL::write(stream, b); TNL::write(stream, c); TNL::write(stream, d); TNL::write(stream, e); TNL::write(stream, f); TNL::write(stream, g); TNL::write(stream, h); TNL::write(stream, i); TNL::write(stream, j); }
   void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j); }
};

};

#endif

