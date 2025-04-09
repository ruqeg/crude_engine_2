#include <core/time.h>

#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <time.h>
#endif

#if defined(_MSC_VER)
static LARGE_INTEGER s_frequency;
#endif


void
crude_initialize_time_service
(
)
{
#if defined(_MSC_VER)
  QueryPerformanceFrequency( &s_frequency );
#endif
}

void  
crude_deinitialize_time_service
(
)
{
}

static int64
int64_mul_div
(
  _In_ int64 value,
  _In_ int64 numer,
  _In_ int64 denom
)
{
  const int64 q = value / denom;
  const int64 r = value % denom;
  return q * numer + r * numer / denom;
}

int64
crude_time_now
(
)
{
#if defined(_MSC_VER)
  LARGE_INTEGER time;
  QueryPerformanceCounter( &time );
  const int64 microseconds = int64_mul_div( time.QuadPart, 1000000LL, s_frequency.QuadPart );
#else
  timespec tp;
  clock_gettime( CLOCK_MONOTONIC, &tp );
  
  const uint64 now = tp.tv_sec * 1000000000 + tp.tv_nsec;
  const int64 microseconds = now / 1000;
#endif
  return microseconds;
}

float64
crude_time_seconds
(
  _In_ int64 time
)
{
  return time / 1000000.0;
}

float64
crude_time_delta_seconds
(
  _In_ int64 starting_time,
  _In_ int64 ending_time
)
{
  return crude_time_seconds( ending_time - starting_time );
}