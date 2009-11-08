#ifndef _CORE_SINGLETON_H_
#define _CORE_SINGLETON_H_

namespace TNL {

/**
 * Derive from thsi class if you want a simple singelton object.
 */
template <typename T>
class Singleton : public NonCopyable
{
public:
   Singleton()
   {
      AssertV(!sm_pSingleton, ("%s: instance already exists.", __FUNCTION__));
      sm_pSingleton = static_cast<T*>(this);
   }

   static bool exists()
   {
      return sm_pSingleton != 0;
   }

   static T & get()
   {
      AssertV(sm_pSingleton, ("%s: no instance exists.", __FUNCTION__));
      return *sm_pSingleton;
   }

   static void destroy()
   {
      delete sm_pSingleton;
      sm_pSingleton = 0;
   }

protected:
   ~Singleton()
   {
      AssertV(sm_pSingleton, ("%s: no instance exists.", __FUNCTION__));
      sm_pSingleton = 0;
   }

   static T * sm_pSingleton;
};

template <typename T> T * Singleton<T>::sm_pSingleton = 0;

} // namespace TNL

#endif // _CORE_SINGLETON_H_
