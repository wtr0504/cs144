#include "wrapping_integers.hh"

using namespace std;



Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  const constexpr uint64_t INT32_RANGE = 1L << 32;
  uint32_t offset = raw_value_ - zero_point.raw_value_;

  if(checkpoint > offset){
    uint64_t temp_checkpoint = (checkpoint - offset) + (INT32_RANGE>>1);
    uint64_t wrap_num = temp_checkpoint / INT32_RANGE;
    return wrap_num * INT32_RANGE + offset;
  }
  else
    return offset;
}
