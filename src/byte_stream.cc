#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  if(error||data.empty()||is_closed())
    return;
  // Your code here.
  // static_assert(is_end_input==true,"is end");
  uint64_t sould_write  = min(data.size(),capacity_-buf.size());
  // data = data.substr(0,sould_write);
  // for( const auto &c : data){
  //   buf.push_back(c);
  // }
  for(uint64_t i = 0;i<sould_write;i++){
    buf.push_back(data[i]);
  }
  written_byte += sould_write;
}

void Writer::close()
{
  // Your code here.
  is_end_input = true;
}

void Writer::set_error()
{
  // Your code here.
  error = true;
}

bool Writer::is_closed() const
{
  return is_end_input;
  // Your code here.
    
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buf.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return {written_byte};
}

string_view Reader::peek() const
{ 
   return {std::string_view(&buf.front(), 1)};
}

bool Reader::is_finished() const
{
  if(is_end_input==true&&buf.size()==0){return true;}
  else 
    return false;
}

bool Reader::has_error() const
{
  // Your code here.
  return error;
}

void Reader::pop( uint64_t len )
{
  uint64_t popsize = min(capacity_,len);
  // buf.erase(buf.begin(),buf.begin()+ popsize);
  // buf.erase(buf.begin(),buf.begin()+popsize);
  for(uint64_t i =0;i<popsize;i++){
    buf.pop_front();
  }
  read_byte += popsize;
  // buf.shrink_to_fit();
}

uint64_t Reader::bytes_buffered() const
{
  
  return written_byte - read_byte;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return {read_byte};
}
