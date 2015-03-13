#pragma once
#ifndef IMTOOLS_OMP_GUARD_HXX
#define IMTOOLS_OMP_GUARD_HXX

#include "threads.hxx"
#include <omp.h>

namespace imtools { namespace threads
{

class OmpGuard
{
  public:
    OmpGuard(omp_lock_t& lock);
    ~OmpGuard();

    OmpGuard& operator=(const OmpGuard&) = delete;
    OmpGuard(const OmpGuard&) = delete;

  private:
    omp_lock_t& mLock;
};

}} // namespace imtools::threads
#endif // IMTOOLS_OMP_GUARD_HXX
// vim: et ts=2 sts=2 sw=2
