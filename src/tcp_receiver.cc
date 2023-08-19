#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.

  if(message.SYN){
    isn_ =  Wrap32(message.seqno);
    set_syn_ = true;
  }
  uint64_t index = message.seqno.unwrap(isn_,inbound_stream.bytes_pushed()+1) + message.SYN - 1;
  reassembler.insert(index,message.payload.release(),message.FIN,inbound_stream);

}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage msg;
  uint16_t window_size = inbound_stream.available_capacity() > UINT16_MAX ? UINT16_MAX : inbound_stream.available_capacity();
  if(!set_syn_){
    return {std::optional<Wrap32>{},window_size};
  }
  

  uint64_t abs_ackno = inbound_stream.bytes_pushed()+1;
  if(inbound_stream.is_closed()){
    abs_ackno++;
  }
  msg.ackno = isn_ + abs_ackno;
  msg.window_size = window_size;
  return {msg};
}
