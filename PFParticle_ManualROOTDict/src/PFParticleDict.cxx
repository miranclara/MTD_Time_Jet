// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME srcdIPFParticleDict
#define R__NO_DEPRECATION

/*******************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define G__DICTIONARY
#include "ROOT/RConfig.hxx"
#include "TClass.h"
#include "TDictAttributeMap.h"
#include "TInterpreter.h"
#include "TROOT.h"
#include "TBuffer.h"
#include "TMemberInspector.h"
#include "TInterpreter.h"
#include "TVirtualMutex.h"
#include "TError.h"

#ifndef G__ROOT
#define G__ROOT
#endif

#include "RtypesImp.h"
#include "TIsAProxy.h"
#include "TFileMergeInfo.h"
#include <algorithm>
#include "TCollectionProxyInfo.h"
/*******************************************************************/

#include "TDataMember.h"

// Header files passed as explicit arguments
#include "PFParticle.h"
#include "PFParticle.h"

// Header files passed via #pragma extra_include

// The generated code does not explicitly qualify STL entities
namespace std {} using namespace std;

namespace ROOT {
   static void *new_PFParticle(void *p = nullptr);
   static void *newArray_PFParticle(Long_t size, void *p);
   static void delete_PFParticle(void *p);
   static void deleteArray_PFParticle(void *p);
   static void destruct_PFParticle(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::PFParticle*)
   {
      ::PFParticle *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TInstrumentedIsAProxy< ::PFParticle >(nullptr);
      static ::ROOT::TGenericClassInfo 
         instance("PFParticle", ::PFParticle::Class_Version(), "PFParticle.h", 9,
                  typeid(::PFParticle), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &::PFParticle::Dictionary, isa_proxy, 4,
                  sizeof(::PFParticle) );
      instance.SetNew(&new_PFParticle);
      instance.SetNewArray(&newArray_PFParticle);
      instance.SetDelete(&delete_PFParticle);
      instance.SetDeleteArray(&deleteArray_PFParticle);
      instance.SetDestructor(&destruct_PFParticle);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::PFParticle*)
   {
      return GenerateInitInstanceLocal(static_cast<::PFParticle*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::PFParticle*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));
} // end of namespace ROOT

//______________________________________________________________________________
atomic_TClass_ptr PFParticle::fgIsA(nullptr);  // static to hold class pointer

//______________________________________________________________________________
const char *PFParticle::Class_Name()
{
   return "PFParticle";
}

//______________________________________________________________________________
const char *PFParticle::ImplFileName()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::PFParticle*)nullptr)->GetImplFileName();
}

//______________________________________________________________________________
int PFParticle::ImplFileLine()
{
   return ::ROOT::GenerateInitInstanceLocal((const ::PFParticle*)nullptr)->GetImplFileLine();
}

//______________________________________________________________________________
TClass *PFParticle::Dictionary()
{
   fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::PFParticle*)nullptr)->GetClass();
   return fgIsA;
}

//______________________________________________________________________________
TClass *PFParticle::Class()
{
   if (!fgIsA.load()) { R__LOCKGUARD(gInterpreterMutex); fgIsA = ::ROOT::GenerateInitInstanceLocal((const ::PFParticle*)nullptr)->GetClass(); }
   return fgIsA;
}

//______________________________________________________________________________
void PFParticle::Streamer(TBuffer &R__b)
{
   // Stream an object of class PFParticle.

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(PFParticle::Class(),this);
   } else {
      R__b.WriteClassBuffer(PFParticle::Class(),this);
   }
}

namespace ROOT {
   // Wrappers around operator new
   static void *new_PFParticle(void *p) {
      return  p ? new(p) ::PFParticle : new ::PFParticle;
   }
   static void *newArray_PFParticle(Long_t nElements, void *p) {
      return p ? new(p) ::PFParticle[nElements] : new ::PFParticle[nElements];
   }
   // Wrapper around operator delete
   static void delete_PFParticle(void *p) {
      delete (static_cast<::PFParticle*>(p));
   }
   static void deleteArray_PFParticle(void *p) {
      delete [] (static_cast<::PFParticle*>(p));
   }
   static void destruct_PFParticle(void *p) {
      typedef ::PFParticle current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::PFParticle

namespace ROOT {
   static TClass *vectorlEPFParticlegR_Dictionary();
   static void vectorlEPFParticlegR_TClassManip(TClass*);
   static void *new_vectorlEPFParticlegR(void *p = nullptr);
   static void *newArray_vectorlEPFParticlegR(Long_t size, void *p);
   static void delete_vectorlEPFParticlegR(void *p);
   static void deleteArray_vectorlEPFParticlegR(void *p);
   static void destruct_vectorlEPFParticlegR(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const vector<PFParticle>*)
   {
      vector<PFParticle> *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(vector<PFParticle>));
      static ::ROOT::TGenericClassInfo 
         instance("vector<PFParticle>", -2, "vector", 423,
                  typeid(vector<PFParticle>), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &vectorlEPFParticlegR_Dictionary, isa_proxy, 4,
                  sizeof(vector<PFParticle>) );
      instance.SetNew(&new_vectorlEPFParticlegR);
      instance.SetNewArray(&newArray_vectorlEPFParticlegR);
      instance.SetDelete(&delete_vectorlEPFParticlegR);
      instance.SetDeleteArray(&deleteArray_vectorlEPFParticlegR);
      instance.SetDestructor(&destruct_vectorlEPFParticlegR);
      instance.AdoptCollectionProxyInfo(TCollectionProxyInfo::Generate(TCollectionProxyInfo::Pushback< vector<PFParticle> >()));

      instance.AdoptAlternate(::ROOT::AddClassAlternate("vector<PFParticle>","std::vector<PFParticle, std::allocator<PFParticle> >"));
      return &instance;
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const vector<PFParticle>*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *vectorlEPFParticlegR_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const vector<PFParticle>*>(nullptr))->GetClass();
      vectorlEPFParticlegR_TClassManip(theClass);
   return theClass;
   }

   static void vectorlEPFParticlegR_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   // Wrappers around operator new
   static void *new_vectorlEPFParticlegR(void *p) {
      return  p ? ::new(static_cast<::ROOT::Internal::TOperatorNewHelper*>(p)) vector<PFParticle> : new vector<PFParticle>;
   }
   static void *newArray_vectorlEPFParticlegR(Long_t nElements, void *p) {
      return p ? ::new(static_cast<::ROOT::Internal::TOperatorNewHelper*>(p)) vector<PFParticle>[nElements] : new vector<PFParticle>[nElements];
   }
   // Wrapper around operator delete
   static void delete_vectorlEPFParticlegR(void *p) {
      delete (static_cast<vector<PFParticle>*>(p));
   }
   static void deleteArray_vectorlEPFParticlegR(void *p) {
      delete [] (static_cast<vector<PFParticle>*>(p));
   }
   static void destruct_vectorlEPFParticlegR(void *p) {
      typedef vector<PFParticle> current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class vector<PFParticle>

namespace {
  void TriggerDictionaryInitialization_libPFParticle_Impl() {
    static const char* headers[] = {
"include/PFParticle.h",
nullptr
    };
    static const char* includePaths[] = {
"/cvmfs/cms.cern.ch/el9_amd64_gcc12/lcg/root/6.32.11-a1d23f0c91e1aeff81bd016882a5fb71/include/",
"/afs/cern.ch/work/m/mrkim/2025Mar26_PUPPITEST/CMSSW_15_0_1/src/PFParticle_ManualROOTDict/",
nullptr
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "libPFParticle dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_AutoLoading_Map;
class __attribute__((annotate("$clingAutoload$PFParticle.h")))  PFParticle;
namespace std{template <typename _Tp> class __attribute__((annotate("$clingAutoload$bits/allocator.h")))  __attribute__((annotate("$clingAutoload$string")))  allocator;
}
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "libPFParticle dictionary payload"


#define _BACKWARD_BACKWARD_WARNING_H
// Inline headers
#include "PFParticle.h"
#pragma once

#include "PFParticle.h"

#ifdef __ROOTCLING__
#pragma link C++ class PFParticle+;
#pragma link C++ class std::vector<PFParticle>+;
#endif


#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[] = {
"PFParticle", payloadCode, "@",
nullptr
};
    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("libPFParticle",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_libPFParticle_Impl, {}, classesHeaders, /*hasCxxModule*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_libPFParticle_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_libPFParticle() {
  TriggerDictionaryInitialization_libPFParticle_Impl();
}
